/**
  ******************************************************************************
  * @file           : stm32_register.h
  * @brief          : STM32 specific registers
  * @copyright      : Copyright (c) 2026 stlink-org. All rights reserved.
  * @date           : 2026-07-27
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */

#ifndef STM32_REGISTER_H
#define STM32_REGISTER_H

#define STM32_REG_CM3_CPUID                   ((uint32_t) 0xe000ed00)

#define STM32_REG_CMx_CPUID_PARTNO_CM0        0xc20
#define STM32_REG_CMx_CPUID_PARTNO_CM0P       0xc60
#define STM32_REG_CMx_CPUID_PARTNO_CM3        0xc23
#define STM32_REG_CMx_CPUID_PARTNO_CM4        0xc24
#define STM32_REG_CMx_CPUID_PARTNO_CM7        0xc27
#define STM32_REG_CMx_CPUID_PARTNO_CM33       0xd21
#define STM32_REG_CMx_CPUID_IMPL_ARM          0x41


#define STM32_REG_CM3_FP_CTRL                 ((uint32_t) 0xe0002000)  // Flash Patch Control Register
#define STM32_REG_CM3_FP_COMPn(n)             ((uint32_t) (0xe0002008 + n * 4))
#define STM32_REG_CM3_FP_CTRL_KEY             (1U << 1)
#define STM32_REG_CM7_FP_LAR                  ((uint32_t) 0xe0000fb0)
#define STM32_REG_CM7_FP_LAR_KEY              ((uint32_t) 0xc5acce55)

#define STM32_REG_CM3_DEMCR                   ((uint32_t) 0xe000edfc)
#define STM32_REG_CM3_DEMCR_TRCENA            (1U << 24)
#define STM32_REG_CM3_DEMCR_VC_HARDERR        (1U << 10)
#define STM32_REG_CM3_DEMCR_VC_BUSERR         (1U << 8)
#define STM32_REG_CM3_DEMCR_VC_CORERESET      (1U << 0)
#define STM32_REG_CM3_DWT_COMPn(n)            ((uint32_t) (0xe0001020 + n * 16))
#define STM32_REG_CM3_DWT_MASKn(n)            ((uint32_t) (0xe0001024 + n * 16))
#define STM32_REG_CM3_DWT_FUNn(n)             ((uint32_t) (0xe0001028 + n * 16))

/* Cortex™-M3 Technical Reference Manual */
/* Configurable Fault Status Register */
#define STM32_REG_CFSR                        ((uint32_t) 0xe000ed28)

/* Hard Fault Status Register */
#define STM32_REG_HFSR                        ((uint32_t) 0xe000ed2c)

/* Debug Halting Control and Status Register */
#define STM32_REG_DFSR                        ((uint32_t) 0xe000ed30)
#define STM32_REG_DFSR_HALT                   (1U << 0)
#define STM32_REG_DFSR_BKPT                   (1U << 1)
#define STM32_REG_DFSR_VCATCH                 (1U << 3)
#define STM32_REG_DFSR_EXTERNAL               (1U << 4)
#define STM32_REG_DFSR_CLEAR                  ((uint32_t) 0x0000001f)
#define STM32_REG_DHCSR                       ((uint32_t) 0xe000edf0)
#define STM32_REG_DHCSR_DBGKEY                ((uint32_t) (0xa05f << 16))
#define STM32_REG_DHCSR_C_DEBUGEN             (1U << 0)
#define STM32_REG_DHCSR_C_HALT                (1U << 1)
#define STM32_REG_DHCSR_C_STEP                (1U << 2)
#define STM32_REG_DHCSR_C_MASKINTS            (1U << 3)
#define STM32_REG_DHCSR_S_REGRDY              (1U << 16)
#define STM32_REG_DHCSR_S_HALT                (1U << 17)
#define STM32_REG_DHCSR_S_SLEEP               (1U << 18)
#define STM32_REG_DHCSR_S_LOCKUP              (1U << 19)
#define STM32_REG_DHCSR_S_RETIRE_ST           (1U << 24)
#define STM32_REG_DHCSR_S_RESET_ST            (1U << 25)
#define STM32_REG_DCRSR                       ((uint32_t) 0xe000edf4)
#define STM32_REG_DCRDR                       ((uint32_t) 0xe000edf8)
#define STM32_REG_DEMCR                       ((uint32_t) 0xe000edfc)
#define STM32_REG_DEMCR_TRCENA                (1U << 24)

/* MCU Debug Component Registers */

/*
 * DBGMCU's address and trace-enable bit layout are family-specific.
 * Below: classic parts (F0/F1/F3/F4/G0/G4/L4...)
 * --> One bit does both pin routing and TPIU clocking.
 */
#define STM32_REG_DBGMCU_CR                   ((uint32_t) 0xe0042004)  // Debug MCU Configuration Register
#define STM32_REG_DBGMCU_CR_DBG_SLEEP         (1U << 0)
#define STM32_REG_DBGMCU_CR_DBG_STOP          (1U << 1)
#define STM32_REG_DBGMCU_CR_DBG_STANDBY       (1U << 2)
#define STM32_REG_DBGMCU_CR_TRACE_IOEN        (1U << 5)
#define STM32_REG_DBGMCU_CR_TRACE_MODE_ASYNC  (0U << 6)

/* STM32H5 (RM0481):
 * Pin routing (TRACE_IOEN) and TPIU clocking (TRACE_CLKEN) are separate bits;
 * both are required for SWO to output anything.
 */
#define STM32_REG_DBGMCU_CR_H5                ((uint32_t) 0x44024004)  // Debug MCU Configuration Register
#define STM32_REG_DBGMCU_CR_TRACE_IOEN_H5     (1U << 4)
#define STM32_REG_DBGMCU_CR_TRACE_CLKEN_H5    (1U << 5)

/* STM32L5/U5:
 * Same two-bit layout as H5, different DBGMCU address.
 */
#define STM32_REG_DBGMCU_CR_L5_U5             ((uint32_t) 0xe0044004)  // Debug MCU Configuration Register
#define STM32_REG_DBGMCU_CR_TRACE_IOEN_L5_U5  (1U << 4)
#define STM32_REG_DBGMCU_CR_TRACE_CLKEN_L5_U5 (1U << 5)

/* Data Watchpoint and Trace (DWT) Registers */
#define STM32_REG_DWT_CTRL                    ((uint32_t) 0xe0001000)  // DWT Control Register
#define STM32_REG_DWT_CTRL_NUM_COMP           (1U << 28)
#define STM32_REG_DWT_CTRL_CYC_TAP            (1U << 9)
#define STM32_REG_DWT_CTRL_POST_INIT          (1U << 5)
#define STM32_REG_DWT_CTRL_POST_PRESET        (1U << 1)
#define STM32_REG_DWT_CTRL_CYCCNT_ENA         (1U << 0)
#define STM32_REG_DWT_FUNCTION0               ((uint32_t) 0xe0001028)  // DWT Function Register 0
#define STM32_REG_DWT_FUNCTION1               ((uint32_t) 0xe0001038)  // DWT Function Register 1
#define STM32_REG_DWT_FUNCTION2               ((uint32_t) 0xe0001048)  // DWT Function Register 2
#define STM32_REG_DWT_FUNCTION3               ((uint32_t) 0xe0001058)  // DWT Function Register 3

/* Instrumentation Trace Macrocell (ITM) Registers */
#define STM32_REG_ITM_TER                     ((uint32_t) 0xe0000e00)  // ITM Trace Enable Register
#define STM32_REG_ITM_TER_PORTS_ALL           ((uint32_t) 0xffffffff)
#define STM32_REG_ITM_TPR                     ((uint32_t) 0xe0000e40)  // ITM Trace Privilege Register
#define STM32_REG_ITM_TPR_PORTS_ALL           ((uint32_t) 0x0000000f)
#define STM32_REG_ITM_TCR                     ((uint32_t) 0xe0000e80)  // ITM Trace Control Register
#define STM32_REG_ITM_TCR_TRACE_BUS_ID_1      (1U << 16)
#define STM32_REG_ITM_TCR_SWO_ENA             (1U << 4)
#define STM32_REG_ITM_TCR_DWT_ENA             (1U << 3)
#define STM32_REG_ITM_TCR_SYNC_ENA            (1U << 2)
#define STM32_REG_ITM_TCR_TS_ENA              (1U << 1)
#define STM32_REG_ITM_TCR_ITM_ENA             (1U << 0)
#define STM32_REG_ITM_TCC                     ((uint32_t) 0xe0000e90)  // ITM Trace Cycle Count
#define STM32_REG_ITM_LAR                     ((uint32_t) 0xe0000fb0)  // ITM Lock Access Register
#define STM32_REG_ITM_LAR_KEY                 ((uint32_t) 0xc5acce55)

/* Trace Port Interface (TPI) Registers */
#define STM32_REG_TPI_LAR                     ((uint32_t) 0xe0040fb0)  // TPI Lock Access Register
#define STM32_REG_TPI_LAR_KEY                 ((uint32_t) 0xc5acce55)
#define STM32_REG_TPI_CSPSR                   ((uint32_t) 0xe0040004)  // TPI Current Parallel Port Size Reg
#define STM32_REG_TPI_CSPSR_PORT_SIZE_1       (1U << 0)
#define STM32_REG_TPI_ACPR                    ((uint32_t) 0xe0040010)  // TPI Async Clock Prescaler Register
#define STM32_REG_TPI_ACPR_MAX                ((uint32_t) 0x00001fff)
#define STM32_REG_TPI_SPPR                    ((uint32_t) 0xe00400f0)  // TPI Selected Pin Protocol Register
#define STM32_REG_TPI_SPPR_SWO_MANCHESTER     (1U << 0)
#define STM32_REG_TPI_SPPR_SWO_NRZ            (2U << 0)
#define STM32_REG_TPI_FFCR                    ((uint32_t) 0xe0040304)  // TPI Formatter and Flush Control Register
#define STM32_REG_TPI_FFCR_TRIG_IN            (1U << 8)

/* Application Interrupt and Reset Control Register */
#define STM32_REG_AIRCR                       ((uint32_t) 0xe000ed0c)
#define STM32_REG_AIRCR_VECTKEY               ((uint32_t) 0x05fa0000)
#define STM32_REG_AIRCR_SYSRESETREQ           ((uint32_t) 0x00000004)
#define STM32_REG_AIRCR_VECTRESET             ((uint32_t) 0x00000001)

/* ARM Cortex-M7 Processor Technical Reference Manual */
/* Cache Control and Status Register */
#define STM32_REG_CM7_CTR                     ((uint32_t) 0xe000ed7c)
#define STM32_REG_CM7_CLIDR                   ((uint32_t) 0xe000ed78)
#define STM32_REG_CM7_CCR                     ((uint32_t) 0xe000ed14)
#define STM32_REG_CM7_CCR_DC                  (1U << 16)
#define STM32_REG_CM7_CCR_IC                  (1U << 17)
#define STM32_REG_CM7_CSSELR                  ((uint32_t) 0xe000ed84)
#define STM32_REG_CM7_DCCSW                   ((uint32_t) 0xe000ef6c)
#define STM32_REG_CM7_ICIALLU                 ((uint32_t) 0xe000ef50)
#define STM32_REG_CM7_CCSIDR                  ((uint32_t) 0xe000ed80)

#endif // STM32_REGISTER_H
