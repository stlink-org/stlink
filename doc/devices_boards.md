Boards supported by the STlink toolset
======================================

The following devices are supported by the STlink tools.

All Boards are expected to work with ST-Link-v2 programmers.


**STM32F0 / ARM Cortex M0 / Core-ID: 0x0bb11477 (STM32F0_CORE_ID)**

| Chip-ID | Product-Code        |
| ---     | ---                 |
| 0x440   | STM32F0**30**x**8** |
| 0x442   | STM32F0**30**x**C** |
| 0x444   | STM32F0**3**xx**4** |
| 0x444   | STM32F0**3**xx**6** |
| 0x445   | STM32F0**4**xxx     |
| 0x440   | STM32F0**5**xxx     |
| 0x445   | STM32F0**70**x**6** |
| 0x448   | STM32F0**70**x**B** |
| 0x448   | STM32F0**71**xx     |
| 0x448   | STM32F0**72**xx     |
| 0x442   | STM32F0**9**xxx     |

Tested boards [incl. STLink programmers]:
* Nucleo-F030R8 [v2-1]
* Nucleo-32 [v2-1]
* STM32F0-Discovery [v2]
* STM320518-EVAL
* Nucleo-F072RB [v2-1]
* Nucleo-F091RC [v2-1]


**STM32F1 / ARM Cortex M3 / Core-ID: 0x1ba01477 (STM32F1_CORE_ID)**

| Product-Code      | Product Line            |
| ---               | ---                     |
| STM32F10**0**yyxx | Value line (V)          |
| STM32F10**1**yyxx | Access line (A)         |
| STM32F10**2**yyxx | USB Access line (USB-A) |
| STM32F10**3**yyxx | Performance line (P)    |
| STM32F10**5**yyxx | Connectivity line (C)   |
| STM32F10**7**yyxx | Connectivity line (C)   |

| Chip-ID | Product Line         | Code (yy) | V    | A    | USB-A | P    | C   |
| ---     | ---                  | ---       | ---  | ---  |  ---  | ---  | --- |
| 0x412   | Low-Density          | x4 x6     | F100 | F101 | F102  | F103 |     |
| 0x410   | Medium Density       | x8 xB     |      | F101 | F102  | F103 |     |
| 0x414   | High density         | xC xD xE  |      | F101 | F103  |      |     |
| 0x418   | STM32F105xx/107xx    | x8 xB xC  |      |      |       |      | F105<br />F107 |
| 0x420   | Medium density value | x8 xB     | F100 |      |       |      |     |
| 0x428   | High density Value   | xC xD xE  | F100 |      |       |      |     |
| 0x430   | XL-Density           | xF XG     |      | F101 |       | F103 |     |

Tested boards [incl. STLink programmers]:
* STM32VL-Discovery (STM32F100RBT6) with STLink-v1 [v1, v2]
* STM32F103-Bluepill: C8Tx & R8xx [v2]
* Nucleo-F103RB [v2-1]
* HY-STM32 (STM32F103VETx) [v1, v2]
* DecaWave EVB1000 (STM32F105RCTx) [v1, v2]


**STM32F2 / ARM Cortex M3 / Core-ID: 0x2ba01477 (STM32F2_CORE_ID)**

| Chip-ID | Product-Code  | Product Line  |
| ---     | ---           | ---           |
| 0x411   | STM32F2yyxx   | (all devices) |


**STM32F1 / ARM Cortex M3 / Core-ID: 0x2ba01477 (STM3F1c_CORE_ID)**

| Product-Code      | Chip-ID | STLink<br />Programmer | Boards |
| ---               | ---     | ---      | --- |
| CKS32F103C8Tx     | 0x410   | v2       | "STM32"-Bluepill ( _**Fake-Marking !**_ )<br />STM32F103C8T6 clone from China Key Systems (CKS) |
| CKS32F103C8Tx     | 0x410   | v2       | CKS32-Bluepill (Clone)<br />STM32F103C8T6 clone from China Key Systems (CKS) |


**STM32F3 / ARM Cortex M4F / Core-ID: 0x2ba01477 (STM32F3_CORE_ID)**

| Product-Code      | Product Line                                                    |
| ---               | ---                                                             |
| STM32F3**01**yyxx | Access line (A)                                                 |
| STM32F3**02**yyxx | USB & CAN line (USB/CAN)                                        |
| STM32F3**03**yyxx | Performance line (P)                                            |
| STM32F3**34**yy   | Digital Power line (DP)                                         |
| STM32F3**73**yy   | Precision Measurement line (PM)  64k/16k / 128k/24k / 265k/32k  |
| STM32F3**18**yy   | General Purpose line (GP)        64k/16k                        |
| STM32F3**28**yy   | General Purpose line (GP)        64k/16k                        |
| STM32F3**58**yy   | General Purpose line (GP)       265k/48k                        |
| STM32F3**78**yy   | Precision Measurement line (PM) 265k/32k                        |
| STM32F3**98**yy   | General Purpose line (GP)       512k/80k                        |

| Chip-ID | Product Line | Code (yy) | A    | USB/CAN | P    | others |
| ---     | ---          | ---       | ---  | ---     | ---  | ---    |
| 0x422   | _N/A_        | xB xC     |      | F302    | F303 |        |
| 0x422   | _N/A_        | -         |      |         |      | F358   |
| 0x432   | _N/A_        | -         |      |         |      | F373<br />F378 |
| 0x438   | _N/A_        | x4 x6 x8  |      |         | F303 |        |
| 0x438   | _N/A_        | -         |      |         |      | F334<br />F328 |
| 0x439   | _N/A_        | x4 x6 x8  | F301 | F302    |      |        |
| 0x439   | _N/A_        | -         |      |         |      | F318   |
| 0x446   | _N/A_        | xD xE     |      | F302    | F303 |        |
| 0x446   | _N/A_        | -         |      |         |      | F398   |

Tested boards [incl. STLink programmers]:
* Nucleo-F302K8 [v2-1]
* Nucleo-F303K8 [v2-1]
* STM32F3348-Discovery [v2-1]
* Nucleo-F334R8 [v2-1]
* STM32F303-Discovery [v2]
* Nucleo-F303RE [v2-1]


**STM32F3 / ARM Cortex M4F / Core-ID: 0x2ba01477 (STM32F3c_CORE_ID)**

| Product-Code   | Chip-ID | STLink<br />Programmer | Boards |
| ---            | ---     | ---      | --- |
| GD32F303VGT6   | 0x430   | _N/A_    | STM32F303 clone from GigaDevice GD)<br />_unsupported_ |


**STM32F4 / ARM Cortex M4F / Core-ID: 0x2ba01477 (STM32F4_CORE_ID)**

| Chip-ID | Product-Code        |
| ---     | ---                 |
| 0x413   | STM32F4**0**xxx     |
| 0x413   | STM32F4**1**xxx     |
| 0x419   | STM32F4**2**xxx     |
| 0x419   | STM32F4**3**xxx     |
| 0x423   | STM32F4**01**x**B** |
| 0x423   | STM32F4**01**x**C** |
| 0x433   | STM32F4**01**x**D** |
| 0x433   | STM32F4**01**x**E** |
| 0x458   | STM32F4**10**xx     |
| 0x431   | STM32F4**11**xx     |
| 0x441   | STM32F4**12**xx     |
| 0x421   | STM32F4**46**xx     |
| 0x434   | STM32F4**69**xx     |
| 0x434   | STM32F4**79**xx     |
| 0x463   | STM32F4**13**xx     |
| 0x463   | STM32F4**23**xx     |

Tested boards [incl. STLink programmers]:
* STM32F407-Discovery [v2]
* 32F411E-Discovery with gyro, audio [v2]
* 32F429I-Discovery with LCD [v2]
* 32F439VIT6-Discovery [v2] (reseated MCU)
* Nucleo-F401RE [v2-1]
* Nucleo-F411RE [v2-1]
* 32F413H-Discovery [v2-1]


**STM32F7 / ARM Cortex M7F / Core-ID: 0x5ba02477 (STM32F7_CORE_ID)**

| Chip-ID | Product-Code    |
| ---     | ---             |
| 0x452   | STM32F7**2**xxx |
| 0x452   | STM32F7**3**xxx |
| 0x449   | STM32F7**4**xxx |
| 0x449   | STM32F7**5**xxx |
| 0x451   | STM32F7**6**xxx |
| 0x451   | STM32F7**7**xxx |

Tested boards [incl. STLink programmers]:
* STM32F756NGHx evaluation board [v2-1]
* 32F769I-Discovery [v2-1]
* Nucleo-F722ZE [v2-1]
* Nucleo-F746ZG [v2-1]


**STM32G0 / ARM Cortex M0+ / Core-ID: 0x0bc11477 (STM32G0_CORE_ID)**

| Chip-ID | Product-Code    |
| ---     | ---             |
| 0x466   | STM32G0**3**xxx |
| 0x466   | STM32G0**4**xxx |
| 0x460   | STM32G0**7**xxx |
| 0x460   | STM32G0**8**xxx |


**STM32G4 / ARM Cortex M4F / Core-ID: 0x2ba01477 (STM32G4_CORE_ID)**

| Chip-ID | Product-Code    |
| ---     | ---             |
| 0x468   | STM32G4**31**xx |
| 0x468   | STM32G4**41**xx |
| 0x469   | STM32G4**7**xxx |
| 0x469   | STM32G4**8**xxx |


**STM32L0 / ARM Cortex M0+ / Core-ID: 0x0bc11477 (STM32L0_CORE_ID)**

| Chip-ID | Product-Code    |
| ---     | ---             |
| 0x457   | STM32L0**1**xxx |
| 0x457   | STM32L0**2**xxx |
| 0x425   | STM32L0**31**xx |
| 0x425   | STM32L0**41**xx |
| 0x417   | STM32L0**5**xxx |
| 0x417   | STM32L0**6**xxx |
| 0x447   | STM32L0**7**xxx |
| 0x447   | STM32L0**8**xxx |

Tested boards [incl. STLink programmers]:
* Nucleo-L053R8 [v2-1]


**STM32L1 / ARM Cortex M3 / Core-ID: 0x2ba01477 (STM32L1_CORE_ID)**

| Chip-ID | Product-Code     |
| ---     | ---              |
| 0x416   | STM32L1xxx**6**  |
| 0x416   | STM32L1xxx**8**  |
| 0x416   | STM32L1xxx**B**  |
| 0x429   | STM32L1xxx**6A** |
| 0x429   | STM32L1xxx**8A** |
| 0x429   | STM32L1xxx**BA** |
| 0x427   | STM32L1xxx**C**  |
| 0x436   | STM32L1xxx**D**  |
| 0x437   | STM32L1xxx**E**  |

Tested boards [incl. STLink programmers]:
* Nucleo-L152RE [v2-1]
* 32L152C-Discovery [v2]


**STM32L4 / ARM Cortex M4F / Core-ID: 0x2ba01477 (STM32L4_CORE_ID)**

| Chip-ID | Product-Code     |
| ---     | ---              |
| 0x464   | STM32L4**12**xx  |
| 0x464   | STM32L4**22**xx  |
| 0x435   | STM32L4**3**xxx  |
| 0x435   | STM32L4**4**xxx  |
| 0x462   | STM32L4**5**xxx  |
| 0x462   | STM32L4**6**xxx  |
| 0x415   | STM32L4**7**xxx  |
| 0x415   | STM32L4**8**xxx  |
| 0x461   | STM32L4**96**xx  |
| 0x461   | STM32L4**A6**xx  |
| 0x470   | STM32L4**R**xx   |
| 0x470   | STM32L4**S**xx   |
| 0x471   | STM32L4**P5**xx  |
| 0x471   | STM32L4**Q5**xx  |

Tested boards [incl. STLink programmers]:
* Nucleo-L432KC [v2-1]
* Nucleo-L452RE [v2-1]
* Nucleo-L476RG [v2-1]
* Nucleo-L496ZG [v2-1]
* 32L4R9I-Discovery [v2-1]


**STM32W / ARM Cortex M3 / Core-ID: 0x2ba01477 (STM32W_CORE_ID)**

| Chip-ID | Product-Code     |
| ---     | ---              |
| 0x495   | STM32WB**50**xx  |
| 0x495   | STM32WB**55**xx  |
| 0x497   | STM32WLE**5**xx  |
