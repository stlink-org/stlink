#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stlink.h>

#if defined(_MSC_VER)
#define __attribute__(x)
#endif

static void __attribute__((unused)) mark_buf(stlink_t *sl) {
    memset(sl->q_buf, 0, sizeof(sl->q_buf));
    sl->q_buf[0] = 0xaa;
    sl->q_buf[1] = 0xbb;
    sl->q_buf[2] = 0xcc;
    sl->q_buf[3] = 0xdd;
    sl->q_buf[4] = 0x11;
    sl->q_buf[15] = 0x22;
    sl->q_buf[16] = 0x33;
    sl->q_buf[63] = 0x44;
    sl->q_buf[64] = 0x69;
    sl->q_buf[1024 * 6 - 1] = 0x42; // 6kB
    sl->q_buf[1024 * 8 - 1] = 0x42; // 8kB
}


int main(void) { // main() ripped out of old stlink-hw.c
    /* Avoid unused parameter warning */
    // set scpi lib debug level: 0 for no debug info, 10 for lots
    fputs(
        "\nUsage: stlink-access-test [anything at all] ...\n"
        "\n*** Notice: The stlink firmware violates the USB standard.\n"
        "*** Because we just use libusb, we can just tell the kernel's\n"
        "*** driver to simply ignore the device...\n"
        "*** Unplug the stlink and execute once as root:\n"
        "modprobe -r usb-storage && modprobe usb-storage quirks=483:3744:i\n\n",
        stderr);

    stlink_t *sl = stlink_v1_open(99, 1);

    if (sl == NULL) return(0);

    // we are in mass mode, go to swd
    stlink_enter_swd_mode(sl);
    stlink_current_mode(sl);
    stlink_core_id(sl);
    stlink_status(sl);
    // stlink_force_debug(sl);
    stlink_reset(sl);
    stlink_status(sl);
    // core system control block
    stlink_read_mem32(sl, 0xe000ed00, 4);
    DLOG("cpu id base register: SCB_CPUID = got 0x%08x expect 0x411fc231\n", read_uint32(sl->q_buf, 0));
    // no MPU
    stlink_read_mem32(sl, 0xe000ed90, 4);
    DLOG("mpu type register: MPU_TYPER = got 0x%08x expect 0x0\n", read_uint32(sl->q_buf, 0));

#if 0
    stlink_read_mem32(sl, 0xe000edf0, 4);
    DD(sl, "DHCSR = 0x%08x", read_uint32(sl->q_buf, 0));
    stlink_read_mem32(sl, 0x4001100c, 4);
    DD(sl, "GPIOC_ODR = 0x%08x", read_uint32(sl->q_buf, 0));
#endif

#if 0
    // let blink all the leds
    // see "RM0041 Reference manual - STM32F100xx advanced ARM-based 32-bit MCUs"

    #define GPIOC     0x40011000 // port C
    #define GPIOC_CRH (GPIOC + 0x04) // port configuration register high
    #define GPIOC_ODR (GPIOC + 0x0c) // port output data register
    #define LED_BLUE  (1 << 8) // pin 8
    #define LED_GREEN (1 << 9) // pin 9
    stlink_read_mem32(sl, GPIOC_CRH, 4);
    uint32_t io_conf = read_uint32(sl->q_buf, 0);
    DLOG("GPIOC_CRH = 0x%08x\n", io_conf);

    // set: general purpose output push-pull, output mode, max speed 10 MHz.
    write_uint32(sl->q_buf, 0x44444411);
    stlink_write_mem32(sl, GPIOC_CRH, 4);

    memset(sl->q_buf, 0, sizeof(sl->q_buf));

    for (int i = 0; i < 100; i++) {
        write_uint32(sl->q_buf, LED_BLUE | LED_GREEN);
        stlink_write_mem32(sl, GPIOC_ODR, 4);
        // stlink_read_mem32(sl, 0x4001100c, 4);
        // DD(sl, "GPIOC_ODR = 0x%08x", read_uint32(sl->q_buf, 0));
        usleep(100 * 1000);

        memset(sl->q_buf, 0, sizeof(sl->q_buf));
        stlink_write_mem32(sl, GPIOC_ODR, 4); // PC lo
        usleep(100 * 1000);
    }

    write_uint32(sl->q_buf, io_conf); // set old state
#endif

#if 0
    // TODO rtfm: stlink doesn't have flash write routines
    // writing to the flash area confuses the fw for the next read access

    // stlink_read_mem32(sl, 0, 1024*6);
    // flash 0x08000000 128kB
    fputs("++++++++++ read a flash at 0x0800 0000\n", stderr);
    stlink_read_mem32(sl, 0x08000000, 1024 * 6); // max 6kB
    clear_buf(sl);
    stlink_read_mem32(sl, 0x08000c00, 5);
    stlink_read_mem32(sl, 0x08000c00, 4);
    mark_buf(sl);
    stlink_write_mem32(sl, 0x08000c00, 4);
    stlink_read_mem32(sl, 0x08000c00, 256);
    stlink_read_mem32(sl, 0x08000c00, 256);
#endif

#if 0
    // sram 0x20000000 8kB
    fputs("\n++++++++++ read/write 8bit, sram at 0x2000 0000 ++++++++++++++++\n\n", stderr);
    memset(sl->q_buf, 0, sizeof(sl->q_buf));
    mark_buf(sl);
    // stlink_write_mem8(sl, 0x20000000, 16);
    // stlink_write_mem8(sl, 0x20000000, 1);
    // stlink_write_mem8(sl, 0x20000001, 1);
    stlink_write_mem8(sl, 0x2000000b, 3);
    stlink_read_mem32(sl, 0x20000000, 16);
#endif

#if 0
    // a not aligned mem32 access doesn't work indeed
    fputs("\n++++++++++ read/write 32bit, sram at 0x2000 0000 ++++++++++++++++\n\n", stderr);
    memset(sl->q_buf, 0, sizeof(sl->q_buf));
    mark_buf(sl);
    stlink_write_mem32(sl, 0x20000000, 1);
    stlink_read_mem32(sl, 0x20000000, 16);
    mark_buf(sl);
    stlink_write_mem32(sl, 0x20000001, 1);
    stlink_read_mem32(sl, 0x20000000, 16);
    mark_buf(sl);
    stlink_write_mem32(sl, 0x2000000b, 3);
    stlink_read_mem32(sl, 0x20000000, 16);

    mark_buf(sl);
    stlink_write_mem32(sl, 0x20000000, 17);
    stlink_read_mem32(sl, 0x20000000, 32);
#endif

#if 0
    // sram 0x20000000 8kB
    fputs("++++++++++ read/write 32bit, sram at 0x2000 0000 ++++++++++++\n", stderr);
    memset(sl->q_buf, 0, sizeof(sl->q_buf));
    mark_buf(sl);
    stlink_write_mem8(sl, 0x20000000, 64);
    stlink_read_mem32(sl, 0x20000000, 64);

    mark_buf(sl);
    stlink_write_mem32(sl, 0x20000000, 1024 * 8); // 8kB
    stlink_read_mem32(sl, 0x20000000, 1024 * 6);
    stlink_read_mem32(sl, 0x20000000 + 1024 * 6, 1024 * 2);
#endif

#if 0
    stlink_run(sl);
    stlink_status(sl);
    stlink_force_debug(sl);
    stlink_status(sl);
#endif

#if 0 /* read the system bootloader */
    fputs("\n++++++++++ reading bootloader ++++++++++++++++\n\n", stderr);
    stlink_fread(sl, "/tmp/barfoo", sl->sys_base, sl->sys_size);
#endif

#if 0 /* read the flash memory */
    fputs("\n+++++++ read flash memory\n\n", stderr);
    /* mark_buf(sl); */
    stlink_read_mem32(sl, 0x08000000, 4);
#endif

#if 0 /* flash programming */
    fputs("\n+++++++ program flash memory\n\n", stderr);
    stlink_fwrite_flash(sl, "/tmp/foobar", 0x08000000);
#endif

#if 0 /* check file contents */
    fputs("\n+++++++ check flash memory\n\n", stderr);
    {
        const int res = stlink_fcheck_flash(sl, "/tmp/foobar", 0x08000000);
        printf("_____ stlink_fcheck_flash() == %d\n", res);
    }
#endif

#if 0
    fputs("\n+++++++ sram write and execute\n\n", stderr);
    stlink_fwrite_sram(sl, "/tmp/foobar", sl->sram_base);
    stlink_run_at(sl, sl->sram_base);
#endif

#if 0
    stlink_run(sl);
    stlink_status(sl);
    // back to mass mode, just in case ...
    stlink_exit_debug_mode(sl);
    stlink_current_mode(sl);
    stlink_close(sl);
#endif

    // fflush(stderr);
    // fflush(stdout);
    return(EXIT_SUCCESS);
}
