#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <signal.h>

#include <stlink.h>

/* STLinky structure on STM chip

struct stlinky {
    uint32_t magic;
    uint32_t bufsize;
    uint32_t up_tail;
    uint32_t up_head;
    uint32_t dw_tail;
    uint32_t dw_head;
    char upbuf[CONFIG_LIB_STLINKY_BSIZE];
    char dwbuf[CONFIG_LIB_STLINKY_BSIZE];
} __attribute__ ((packed));
*/


#define STLINKY_MAGIC 0xDEADF00D

#define ST_TERM_MAX_BUFF_SIZE               (1024*1024)   //1Mb

#define RX_Q_OFFSET                         8
#define RX_BUFF_OFFSET                      24
#define TX_Q_OFFSET                         16
#define TX_BUFF_OFFSET(bufsize)             (24 + bufsize)

#define READ_UINT32_LE(buf)  ((uint32_t) (  buf[0]         \
                                          | buf[1] <<  8   \
                                          | buf[2] << 16   \
                                          | buf[3] << 24))

static stlink_t* gsl;
static sigset_t sig_mask;

struct stlinky {
    stlink_t *sl;
    uint32_t off;
    size_t bufsize;
};

void nonblock(int state);

static void cleanup(int signum) {
	(void)signum;

    if (gsl) {
        /* Switch back to mass storage mode before closing. */
        stlink_run(gsl);
        stlink_exit_debug_mode(gsl);
        stlink_close(gsl);
    }

    printf("\n");
    nonblock(0);
    exit(1);
}

void sig_init() {
    sigemptyset(&sig_mask);
    sigaddset(&sig_mask, SIGINT);
    sigaddset(&sig_mask, SIGTERM);
    signal(SIGINT, &cleanup);
    signal(SIGTERM, &cleanup);
    sigprocmask(SIG_BLOCK, &sig_mask, NULL);
}

void sig_process() {
    sigset_t pending;
    sigpending(&pending);
    if (sigismember(&pending, SIGINT) || sigismember(&pending, SIGTERM)) {
        sigprocmask(SIG_UNBLOCK, &sig_mask, NULL);
        sigsuspend(&pending);
        sigprocmask(SIG_BLOCK, &sig_mask, NULL);
    }
}

/* Detects stlinky in RAM, returns handler */
struct stlinky*  stlinky_detect(stlink_t* sl)
{
    static const uint32_t sram_base = 0x20000000;
    struct stlinky* st = malloc(sizeof(struct stlinky));
    int multiple=0;
    st->sl = sl;
    printf("sram: 0x%x bytes @ 0x%zx\n", sl->sram_base, sl->sram_size);
    uint32_t off;
    for (off = 0; off < sl->sram_size; off += 4) {
        if (off % 1024 == 0) sig_process();
        stlink_read_mem32(sl, sram_base + off, 4);
        if (STLINKY_MAGIC == READ_UINT32_LE(sl->q_buf))
        {
            if (multiple > 0) printf("WARNING: another ");
            printf("stlinky detected at 0x%x\n", sram_base + off);
            st->off = sram_base + off;
            stlink_read_mem32(sl, st->off + 4, 4);
            st->bufsize = READ_UINT32_LE(sl->q_buf);
            printf("stlinky buffer size 0x%u \n", (unsigned int)st->bufsize);
            multiple++;
        }
    }
    if (multiple > 0) {
        if (multiple > 1) {
            printf("Using last stlinky structure detected\n");
        }
        return st;
    }
    return NULL;
}

static void stlinky_read_buff(struct stlinky *st, uint32_t off, uint32_t size, char *buffer)
{
    int aligned_size;

    if (size == 0)
        return;

    //Read from device with 4-byte alignment
    aligned_size = (size & 0xFFFFFFFC) + 8;
    stlink_read_mem32(st->sl, off & 0xFFFFFFFC, aligned_size);

    //copy to local buffer
    memcpy(buffer, st->sl->q_buf + (off & 0x3), size);

    return;
}

static void stlinky_write_buf(struct stlinky *st, uint32_t off, uint32_t size, char *buffer)
{
    memcpy(st->sl->q_buf, buffer, size);
    stlink_write_mem8(st->sl, off, size);
    return;
}

size_t stlinky_rx(struct stlinky *st, char* buffer)
{
    //read head and tail values
    uint32_t tail, head;
    stlink_read_mem32(st->sl, st->off + RX_Q_OFFSET, sizeof(tail) + sizeof(head));
    memcpy(&tail, &st->sl->q_buf[0], sizeof(tail));
    memcpy(&head, &st->sl->q_buf[sizeof(tail)], sizeof(head));

    //return if empty
    if(head == tail)
        return 0;

    //read data
    int size_read = 0;
    if(head > tail){
        stlinky_read_buff(st, st->off + RX_BUFF_OFFSET + tail, head - tail, buffer);
        size_read += head - tail;
    } else if(head < tail){
        stlinky_read_buff(st, st->off + RX_BUFF_OFFSET + tail, (uint32_t) st->bufsize - tail, buffer);
        size_read += st->bufsize - tail;

        stlinky_read_buff(st, st->off + RX_BUFF_OFFSET, head, buffer + size_read);
        size_read += head;
    }

    //move tail
    tail = (tail + size_read) % st->bufsize;

    //write tail
    memcpy(st->sl->q_buf, &tail, sizeof(tail));
    stlink_write_mem32(st->sl, st->off + RX_Q_OFFSET, sizeof(tail));

    return size_read;
}

size_t stlinky_tx(struct stlinky *st, char* buffer, size_t siz)
{
    //read head and tail values
    uint32_t tail, head;
    stlink_read_mem32(st->sl, st->off + TX_Q_OFFSET, sizeof(tail) + sizeof(head));
    memcpy(&tail, &st->sl->q_buf[0], sizeof(tail));
    memcpy(&head, &st->sl->q_buf[sizeof(tail)], sizeof(head));

    //Figure out buffer usage
    int usage = head - tail;
    if (usage < 0)
        usage += st->bufsize;

    //check if new data will fit
    if (usage + siz >= st->bufsize)
        return 0;

    //copy in data (take care of possible split)
    int first_chunk = (head + siz >= st->bufsize) ? (int) st->bufsize - (int) head : (int) siz;
    int second_chunk = (int) siz - first_chunk;

    //copy data
    stlinky_write_buf(st, st->off + (uint32_t) TX_BUFF_OFFSET(st->bufsize) + head, first_chunk, buffer);
    if (second_chunk > 0)
        stlinky_write_buf(st, st->off + (uint32_t) TX_BUFF_OFFSET(st->bufsize),
                                        second_chunk, buffer + first_chunk);

    //increment head pointer
    head = (head + siz) % (uint32_t) st->bufsize;
    memcpy(st->sl->q_buf, &head, sizeof(head));
    stlink_write_mem32(st->sl, st->off + TX_Q_OFFSET + sizeof(tail), sizeof(head));

    return siz;
}

int kbhit()
{
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds); //STDIN_FILENO is 0
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    return FD_ISSET(STDIN_FILENO, &fds);
}

void nonblock(int state)
{
    struct termios ttystate;

    //get the terminal state
    tcgetattr(STDIN_FILENO, &ttystate);

    if (state==1)
    {
        //turn off canonical mode
        ttystate.c_lflag &= ~ICANON;
        ttystate.c_lflag &= ~ECHO;
        //minimum of number input read.
        ttystate.c_cc[VMIN] = 1;
    }
    else if (state==0)
    {
        //turn on canonical mode
        ttystate.c_lflag |= ICANON | ECHO;
    }
    //set the terminal attributes.
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);

}

int main(int ac, char** av) {
    struct stlinky *st=NULL;

    sig_init();

    gsl = stlink_open_usb(10, 1, NULL);
    if (gsl != NULL) {
        printf("ST-Linky proof-of-concept terminal :: Created by Necromant for lulz\n");
        stlink_version(gsl);
        stlink_enter_swd_mode(gsl);
        printf("chip id: %#x\n", gsl->chip_id);
        printf("core_id: %#x\n", gsl->core_id);

        cortex_m3_cpuid_t cpuid;
        stlink_cpu_id(gsl, &cpuid);
        printf("cpuid:impl_id = %0#x, variant = %#x\n", cpuid.implementer_id, cpuid.variant);
        printf("cpuid:part = %#x, rev = %#x\n", cpuid.part, cpuid.revision);

        stlink_reset(gsl);
        stlink_force_debug(gsl);
        stlink_run(gsl);
        stlink_status(gsl);

        /* wait for device to boot */
        /* TODO: Make timeout adjustable via command line */
        sleep(1);

        if(ac == 1){
            st = stlinky_detect(gsl);
        }else if(ac == 2){
            st = malloc(sizeof(struct stlinky));
            st->sl = gsl;
            st->off = (int)strtol(av[1], NULL, 16);
            printf("using stlinky at 0x%x\n", st->off);
            stlink_read_mem32(gsl, st->off + 4, 4);
            st->bufsize = READ_UINT32_LE(gsl->q_buf);
            printf("stlinky buffer size 0x%u \n", (unsigned int)st->bufsize);
        }else{
            cleanup(0);
        }
        if (st == NULL)
        {
            printf("stlinky magic not found in sram :(\n");
            cleanup(0);
        }
        if (st->bufsize > ST_TERM_MAX_BUFF_SIZE){
            printf("stlinky buffer size too big\n");
            cleanup(0);
        }
        char* rxbuf = malloc(st->bufsize);
        char* txbuf = malloc(st->bufsize);
        size_t tmp;
        nonblock(1);
        int fd = fileno(stdin);
        int saved_flags = fcntl(fd, F_GETFL);
        fcntl(fd, F_SETFL, saved_flags & ~O_NONBLOCK);
        printf("Entering interactive terminal. CTRL+C to exit\n\n\n");
        while(1) {
            sig_process();
            tmp = stlinky_rx(st, rxbuf);
            if(tmp > 0)
            {
                fwrite(rxbuf,tmp,1,stdout);
                fflush(stdout);
            }
            if (kbhit()) {
                tmp = read(fd, txbuf, st->bufsize);
                stlinky_tx(st,txbuf,tmp);
            }
        }
    }
    return 0;
}
