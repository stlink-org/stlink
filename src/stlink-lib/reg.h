#ifndef STLINK_REG_H_
#define STLINK_REG_H_

#define STLINK_REG_CM3_CPUID           0xE000ED00

#define STLINK_REG_CM3_FP_CTRL         0xE0002000
#define STLINK_REG_CM3_FP_COMPn(n)    (0xE0002008 + n*4)
#define STLINK_REG_CM7_FP_LAR          0xE0000FB0
#define STLINK_REG_CM7_FP_LAR_KEY      0xC5ACCE55

#define STLINK_REG_CM3_DEMCR           0xE000EDFC
#define STLINK_REG_CM3_DWT_COMPn(n)   (0xE0001020 + n*16)
#define STLINK_REG_CM3_DWT_MASKn(n)   (0xE0001024 + n*16)
#define STLINK_REG_CM3_DWT_FUNn(n)    (0xE0001028 + n*16)

/* Cortex™-M3 Technical Reference Manual */
/* Debug Halting Control and Status Register */
#define STLINK_REG_DHCSR               0xe000edf0
#define STLINK_REG_DHCSR_DBGKEY        0xa05f0000
#define STLINK_REG_DCRSR               0xe000edf4
#define STLINK_REG_DCRDR               0xe000edf8

/* Application Interrupt and Reset Control Register */
#define STLINK_REG_AIRCR               0xe000ed0c
#define STLINK_REG_AIRCR_VECTKEY       0x05fa0000
#define STLINK_REG_AIRCR_SYSRESETREQ   0x00000004

#endif // STLINK_REG_H_
