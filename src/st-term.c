#include <stdio.h>
/* According to POSIX.1-2001 */
#include <sys/select.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <signal.h>
#include "stlink-common.h"

#define STLINKY_MAGIC 0xDEADF00D

struct stlinky {
	stlink_t *sl;
	uint32_t off;
	size_t bufsize;
};


/* Detects stlinky in RAM, returns handler */
struct stlinky*  stlinky_detect(stlink_t* sl)
{
	static const uint32_t sram_base = 0x20000000;
	struct stlinky* st = malloc(sizeof(struct stlinky));
	st->sl = sl;
	printf("sram: 0x%x bytes @ 0x%x\n", sl->sram_base, sl->sram_size);
	uint32_t off;
	for (off = 0; off < sl->sram_size; off += 4) {
		stlink_read_mem32(sl, sram_base + off, 4);
		if ( STLINKY_MAGIC== *(uint32_t*) sl->q_buf)
		{
			printf("stlinky detected at 0x%x\n", sram_base + off);
			st->off = sram_base + off;
			stlink_read_mem32(sl, st->off + 4, 4);
			st->bufsize = (size_t) *(unsigned char*) sl->q_buf;
			printf("stlinky buffer size 0x%zu \n", st->bufsize);
			return st;
		}
	}
	return NULL;
}

int stlinky_canrx(struct stlinky *st)
{
	stlink_read_mem32(st->sl, st->off+4, 4);
	unsigned char tx = (unsigned char) st->sl->q_buf[1];
	return (int) tx;
}

size_t stlinky_rx(struct stlinky *st, char* buffer)
{
	unsigned char tx = 0;
	while(tx == 0) {
		stlink_read_mem32(st->sl, st->off+4, 4);
		tx = (unsigned char) st->sl->q_buf[1];
	}
	size_t rs = tx + (4 - (tx % 4)); /* voodoo */
	stlink_read_mem32(st->sl, st->off+8, rs);
	memcpy(buffer, st->sl->q_buf, (size_t) tx);
	*st->sl->q_buf=0x0;
	stlink_write_mem8(st->sl, st->off+5, 1);
	return (size_t) tx;
}

size_t stlinky_tx(struct stlinky *st, char* buffer, size_t sz)
{
	unsigned char rx = 1;
	while(rx != 0) {
		stlink_read_mem32(st->sl, st->off+4, 4);
		rx = (unsigned char) st->sl->q_buf[2];
	}
	memcpy(st->sl->q_buf, buffer, sz);
	size_t rs = sz + (4 - (sz % 4)); /* voodoo */
	stlink_write_mem32(st->sl, st->off+8+st->bufsize, rs);
	*st->sl->q_buf=(unsigned char) sz;
	stlink_write_mem8(st->sl, st->off+6, 1);
	return (size_t) rx;
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

static int keep_running = 1;
static int sigcount=0;
void cleanup(int dummy)
{
	sigcount++;
	keep_running = 0;
	printf("\n\nGot a signal\n");
	if (sigcount==2) {
		printf("\n\nGot a second signal - bailing out\n");
		exit(1);
	}
}


int main(int ac, char** av) {
	stlink_t* sl;

	/* unused */
	ac = ac;
	av = av;
	sl = stlink_open_usb(10);
	if (sl != NULL) {
		printf("ST-Linky proof-of-concept terminal :: Created by Necromant for lulz\n");
		stlink_version(sl);
		stlink_enter_swd_mode(sl);
		printf("chip id: %#x\n", sl->chip_id);
		printf("core_id: %#x\n", sl->core_id);

		cortex_m3_cpuid_t cpuid;
		stlink_cpu_id(sl, &cpuid);
		printf("cpuid:impl_id = %0#x, variant = %#x\n", cpuid.implementer_id, cpuid.variant);
		printf("cpuid:part = %#x, rev = %#x\n", cpuid.part, cpuid.revision);

		stlink_reset(sl);
		stlink_force_debug(sl);
		stlink_run(sl);
		stlink_status(sl);

		/* wait for device to boot */
		/* TODO: Make timeout adjustable via command line */
		sleep(1);

		struct stlinky *st = stlinky_detect(sl);
		if (st == NULL)
		{
			printf("stlinky magic not found in sram :(\n");
			goto bailout;
		}
		char* rxbuf = malloc(st->bufsize);
		char* txbuf = malloc(st->bufsize);
		size_t tmp;
		nonblock(1);
		int fd = fileno(stdin);
		int saved_flags = fcntl(fd, F_GETFL);
		fcntl(fd, F_SETFL, saved_flags & ~O_NONBLOCK);
		signal(SIGINT, cleanup);
		printf("Entering interactive terminal. CTRL+C to exit\n\n\n");
		while(1) {
			if (stlinky_canrx(st)) {
				tmp = stlinky_rx(st, rxbuf);
				fwrite(rxbuf,tmp,1,stdout);
				fflush(stdout);
			}
			if (kbhit()) {
				tmp = read(fd, txbuf, st->bufsize);
				stlinky_tx(st,txbuf,tmp);
			}
			if (!keep_running)
				break;
		}
	bailout:
		nonblock(0);
		stlink_exit_debug_mode(sl);
		stlink_close(sl);
	}
	return 0;
}
