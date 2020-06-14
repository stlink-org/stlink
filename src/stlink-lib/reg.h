#ifndef STLINK_REG_H_
#define STLINK_REG_H_

#define STLINK_REG_CM3_CPUID           0xE000ED00
#define STLINK_REG_CM3_FP_CTRL         0xE0002000
#define STLINK_REG_CM3_FP_COMP0        0xE0002008

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
