#ifndef STLINK_REG_H_
#define STLINK_REG_H_

#define STLINK_REG_CM3_CPUID                0xE000ED00

#define STLINK_REG_CM3_FP_CTRL              0xE0002000
#define STLINK_REG_CM3_FP_COMPn(n)          (0xE0002008 + n*4)
#define STLINK_REG_CM7_FP_LAR               0xE0000FB0
#define STLINK_REG_CM7_FP_LAR_KEY           0xC5ACCE55

#define STLINK_REG_CM3_DEMCR                0xE000EDFC
#define STLINK_REG_CM3_DEMCR_TRCENA         (1 << 24)
#define STLINK_REG_CM3_DEMCR_VC_HARDERR     (1 << 10)
#define STLINK_REG_CM3_DEMCR_VC_BUSERR      (1 << 8)
#define STLINK_REG_CM3_DEMCR_VC_CORERESET   (1 << 0)
#define STLINK_REG_CM3_DWT_COMPn(n)         (0xE0001020 + n*16)
#define STLINK_REG_CM3_DWT_MASKn(n)         (0xE0001024 + n*16)
#define STLINK_REG_CM3_DWT_FUNn(n)          (0xE0001028 + n*16)

/* Cortex™-M3 Technical Reference Manual */
/* Debug Halting Control and Status Register */
#define STLINK_REG_DFSR                     0xE000ED30
#define STLINK_REG_DFSR_HALT                (1 << 0)
#define STLINK_REG_DFSR_BKPT                (1 << 1)
#define STLINK_REG_DFSR_VCATCH              (1 << 3)
#define STLINK_REG_DFSR_EXTERNAL            (1 << 4)
#define STLINK_REG_DFSR_CLEAR               0x0000001F
#define STLINK_REG_DHCSR                    0xe000edf0
#define STLINK_REG_DHCSR_DBGKEY             (0xA05F << 16)
#define STLINK_REG_DHCSR_C_DEBUGEN          (1 << 0)
#define STLINK_REG_DHCSR_C_HALT             (1 << 1)
#define STLINK_REG_DHCSR_C_STEP             (1 << 2)
#define STLINK_REG_DHCSR_C_MASKINTS         (1 << 3)
#define STLINK_REG_DHCSR_S_REGRDY           (1 << 16)
#define STLINK_REG_DHCSR_S_HALT             (1 << 17)
#define STLINK_REG_DHCSR_S_SLEEP            (1 << 18)
#define STLINK_REG_DHCSR_S_LOCKUP           (1 << 19)
#define STLINK_REG_DHCSR_S_RETIRE_ST        (1 << 24)
#define STLINK_REG_DHCSR_S_RESET_ST         (1 << 25)
#define STLINK_REG_DCRSR                    0xe000edf4
#define STLINK_REG_DCRDR                    0xe000edf8

/* Application Interrupt and Reset Control Register */
#define STLINK_REG_AIRCR                    0xe000ed0c
#define STLINK_REG_AIRCR_VECTKEY            0x05fa0000
#define STLINK_REG_AIRCR_SYSRESETREQ        0x00000004
#define STLINK_REG_AIRCR_VECTRESET          0x00000001

/* ARM Cortex-M7 Processor Technical Reference Manual */
/* Cache Control and Status Register */
#define STLINK_REG_CM7_CTR                  0xE000ED7C
#define STLINK_REG_CM7_CLIDR                0xE000ED78
#define STLINK_REG_CM7_CCR                  0xE000ED14
#define STLINK_REG_CM7_CCR_DC               (1 << 16)
#define STLINK_REG_CM7_CCR_IC               (1 << 17)
#define STLINK_REG_CM7_CSSELR               0xE000ED84
#define STLINK_REG_CM7_DCCSW                0xE000EF6C
#define STLINK_REG_CM7_ICIALLU              0xE000EF50
#define STLINK_REG_CM7_CCSIDR               0xE000ED80

#endif // STLINK_REG_H_
