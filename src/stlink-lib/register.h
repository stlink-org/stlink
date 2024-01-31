/*
 * File: register.h
 *
 * Common STM32 registers
 */

#ifndef REGISTER_H
#define REGISTER_H

#define STLINK_REG_CM3_CPUID                0xE000ED00

#define STLINK_REG_CMx_CPUID_PARTNO_CM0     0xC20
#define STLINK_REG_CMx_CPUID_PARTNO_CM0P    0xC60
#define STLINK_REG_CMx_CPUID_PARTNO_CM3     0xC23
#define STLINK_REG_CMx_CPUID_PARTNO_CM4     0xC24
#define STLINK_REG_CMx_CPUID_PARTNO_CM7     0xC27
#define STLINK_REG_CMx_CPUID_PARTNO_CM33    0xD21
#define STLINK_REG_CMx_CPUID_IMPL_ARM       0x41


#define STLINK_REG_CM3_FP_CTRL              0xE0002000 // Flash Patch Control Register
#define STLINK_REG_CM3_FP_COMPn(n)          (0xE0002008 + n*4)
#define STLINK_REG_CM3_FP_CTRL_KEY          (1 << 1)
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
/* Configurable Fault Status Register */
#define STLINK_REG_CFSR                     0xE000ED28

/* Hard Fault Status Register */
#define STLINK_REG_HFSR                     0xE000ED2C

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
#define STLINK_REG_DEMCR                    0xe000edfc
#define STLINK_REG_DEMCR_TRCENA             (1 << 24)

/* MCU Debug Component Registers */
#define STLINK_REG_DBGMCU_CR                0xE0042004 // Debug MCU Configuration Register
#define STLINK_REG_DBGMCU_CR_DBG_SLEEP      (1 << 0)
#define STLINK_REG_DBGMCU_CR_DBG_STOP       (1 << 1)
#define STLINK_REG_DBGMCU_CR_DBG_STANDBY    (1 << 2)
#define STLINK_REG_DBGMCU_CR_TRACE_IOEN     (1 << 5)
#define STLINK_REG_DBGMCU_CR_TRACE_MODE_ASYNC  (0x00 << 6)

/* Data Watchpoint and Trace (DWT) Registers */
#define STLINK_REG_DWT_CTRL                 0xE0001000 // DWT Control Register
#define STLINK_REG_DWT_CTRL_NUM_COMP        (1 << 28)
#define STLINK_REG_DWT_CTRL_CYC_TAP         (1 << 9)
#define STLINK_REG_DWT_CTRL_POST_INIT       (1 << 5)
#define STLINK_REG_DWT_CTRL_POST_PRESET     (1 << 1)
#define STLINK_REG_DWT_CTRL_CYCCNT_ENA      (1 << 0)
#define STLINK_REG_DWT_FUNCTION0            0xE0001028 // DWT Function Register 0
#define STLINK_REG_DWT_FUNCTION1            0xE0001038 // DWT Function Register 1
#define STLINK_REG_DWT_FUNCTION2            0xE0001048 // DWT Function Register 2
#define STLINK_REG_DWT_FUNCTION3            0xE0001058 // DWT Function Register 3

/* Instrumentation Trace Macrocell (ITM) Registers */
#define STLINK_REG_ITM_TER                  0xE0000E00 // ITM Trace Enable Register
#define STLINK_REG_ITM_TER_PORTS_ALL        (0xFFFFFFFF)
#define STLINK_REG_ITM_TPR                  0xE0000E40 // ITM Trace Privilege Register
#define STLINK_REG_ITM_TPR_PORTS_ALL        (0x0F)
#define STLINK_REG_ITM_TCR                  0xE0000E80 // ITM Trace Control Register
#define STLINK_REG_ITM_TCR_TRACE_BUS_ID_1   (0x01 << 16)
#define STLINK_REG_ITM_TCR_SWO_ENA          (1 << 4)
#define STLINK_REG_ITM_TCR_DWT_ENA          (1 << 3)
#define STLINK_REG_ITM_TCR_SYNC_ENA         (1 << 2)
#define STLINK_REG_ITM_TCR_TS_ENA           (1 << 1)
#define STLINK_REG_ITM_TCR_ITM_ENA          (1 << 0)
#define STLINK_REG_ITM_TCC                  0xE0000E90 // ITM Trace Cycle Count
#define STLINK_REG_ITM_LAR                  0xE0000FB0 // ITM Lock Access Register
#define STLINK_REG_ITM_LAR_KEY              0xC5ACCE55

/* Trace Port Interface (TPI) Registers */
#define STLINK_REG_TPI_CSPSR                0xE0040004 // TPI Current Parallel Port Size Reg
#define STLINK_REG_TPI_CSPSR_PORT_SIZE_1    (0x01 << 0)
#define STLINK_REG_TPI_ACPR                 0xE0040010 // TPI Async Clock Prescaler Register
#define STLINK_REG_TPI_ACPR_MAX             (0x1FFF)
#define STLINK_REG_TPI_SPPR  0xE00400F0     // TPI Selected Pin Protocol Register
#define STLINK_REG_TPI_SPPR_SWO_MANCHESTER  (0x01 << 0)
#define STLINK_REG_TPI_SPPR_SWO_NRZ         (0x02 << 0)
#define STLINK_REG_TPI_FFCR  0xE0040304     // TPI Formatter and Flush Control Register
#define STLINK_REG_TPI_FFCR_TRIG_IN         (0x01 << 8)

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

#endif // REGISTER_H
