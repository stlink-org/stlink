# MCUs supported by the STlink toolset

A list of devices supported by the stlink toolset can be found in */inc/stm32.h*.
More commonly these are:

| Product-Family | ARM Cortex Core | Product Line                                               |
| -------------- | --------------- | ---------------------------------------------------------- |
| STM32F0        | M0              |                                                            |
| STM32C0        | M0+             |                                                            |
| STM32G0        | M0+             |                                                            |
| STM32L0        | M0+             |                                                            |
| STM32F10**0**  | M3              | Value line                                                 |
| STM32F10**1**  | M3              | Access line                                                |
| STM32F10**2**  | M3              | USB Access line                                            |
| STM32F10**3**  | M3              | Performance line                                           |
| STM32F10**5**  | M3              | Connectivity line                                          |
| STM32F10**7**  | M3              | Connectivity line                                          |
| STM32L1        | M3              |                                                            |
| STM32F2        | M3              |                                                            |
| STM32F3**01**  | M4F             | Access line                                                |
| STM32F3**02**  | M4F             | USB & CAN line                                             |
| STM32F3**03**  | M4F             | Performance line                                           |
| STM32F3**34**  | M4F             | Digital Power line                                         |
| STM32F3**73**  | M4F             | Precision Measurement line (64k/16k / 128k/24k / 265k/32k) |
| STM32F3**18**  | M4F             | General Purpose line (64k/16k)                             |
| STM32F3**28**  | M4F             | General Purpose line (64k/16k)                             |
| STM32F3**58**  | M4F             | General Purpose line (265k/48k)                            |
| STM32F3**78**  | M4F             | Precision Measurement line (265k/32k)                      |
| STM32F3**98**  | M4F             | General Purpose line (512k/80k)                            |
| STM32F4        | M4F             |                                                            |
| STM32G4        | M4F             |                                                            |
| STM32L4        | M4F             |                                                            |
| STM32F7        | M7F             |                                                            |
| STM32H7        | M7F             |                                                            |
| STM32WB        | M4F             |                                                            |
| STM32WL        | M4              |                                                            |
| STM32L5        | M33             |                                                            |
| STM32H5        | M33             |                                                            |
| STM32U5        | M33             |                                                            |


# Chinese Clone-Chips [may work, but without support!]

## STM32F1 Clone / ARM Cortex M3 (Core-ID: 0x2ba01477) (mostly on Bluepill-Boards)

**(!) Attention:** Some MCUs may come with with _**Fake-STM32-Marking !**_

**(!) Attention:** The Core-ID of these MCUs is in conflict with the one of the original STM32F1-devices.

| Product-Code  | Chip-ID | Comment                                                                   |
| ------------- | ------- | ------------------------------------------------------------------------- |
| CKS32F103C8T6 | 0x410   | STM32F103C8T6 clone from China Key Systems (CKS)                          |
| CH32F103C8T6  | 0x410   | STM32F103C8T6 clone from Nanjing Qinheng Microelectronics Co., Ltd. (WCH) |

## STM32F3 Clone / ARM Cortex M4F (Core-ID: 0x2ba01477)

**(!) Attention:** The Chip-IDs of these MCUs are in conflict with such of original STM32F1-devices.

| Product-Code | Chip-ID | Comment                              |
| ------------ | ------- | ------------------------------------ |
| GD32F303VET6 | 0x414   | STM32F303 clone from GigaDevice (GD) |
| GD32F303CGT6 | 0x430   | STM32F303 clone from GigaDevice (GD) |
| GD32F303VGT6 | 0x430   | STM32F303 clone from GigaDevice (GD) |
