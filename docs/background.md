# Background

Where CIM comes from, what it has to do, and where to read more.

## Why CIM

A Formula Student car has a central VCU and dozens of small peripherals in the wire harness: pedal sensors, fans, displays, valves. Each of them needs a microcontroller and a bus connection, and the data they exchange keeps growing. The classic CAN 2.0 bus with 8-byte frames at 1 Mbit/s reached its limit in AMZ's cars, and the previous Mini CAN Module had no way to be updated in the car. [^thesis-intro]

CIM is the answer: one small board with CAN FD, a capable MCU and the common sensor and actuator interfaces, which can be flashed over the bus. Everything application-specific goes on a carrier board under it.

CAN FD keeps the CAN bus but allows up to 64 bytes per frame and a faster bit rate for the data phase, up to 8 Mbit/s with the TCAN4551. For the protocol itself see [CAN in Automation](https://www.can-cia.org/can-knowledge/can-fd) and the [RP2040 datasheet](https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf) for the MCU; this documentation does not repeat them.

## Requirements

The thesis fixed eight requirements. They still describe what a CIM must do, and the table says where each one lives today. [^thesis-req]

| | Requirement | Where it is met |
|---|---|---|
| 1 | CAN FD at up to 8 Mbit/s | [TCAN4x5x driver](../firmware/drivers/tcan4x5x/README.md), [hardware overview](hardware/overview.md) |
| 2 | supply from 5 V to 30 V | TLVM23615 regulator, [hardware overview](hardware/overview.md) |
| 3 | GPIO, SPI, I2C, ADC, PWM, UART for peripherals | eight GPIOs on the [mezzanine connector](hardware/pinout.md) |
| 4 | small enough for the wire harness, guideline 15 x 30 mm | board design, [design history](hardware/overview.md#design-history) |
| 5 | software updates over CAN FD | [bootloader concept](protocol/bootloader.md), [CIM protocol](protocol/cim-protocol.md) |
| 6 | diagnostics over a serial interface for non-technical users | [commissioning](../firmware/commission/README.md) and [logging](../firmware/log/README.md) over USB serial |
| 7 | I/O expander for applications with many pins | on the carrier board, not part of this repository |
| 8 | automotive environment: temperature, vibration, EMI, dirt | board design; tested in the car |

## The master's thesis

CIM was designed and built as the master's thesis *CAN FD Interface Module* by Lukas Betschart at the Lucerne University of Applied Sciences and Arts (HSLU), 2024, supervised by Prof. Erich Styger. From its abstract: [^thesis-abstract]

> The CAN FD Interface Module is a modular and compact printed circuit board with open-source hardware and software. It improves data communication in automotive and robotics systems by supporting the CAN FD protocol, the successor of the CAN 2.0 protocol. The module uses the RP2040 microcontroller and TCAN4551 CAN controller and forms the interface between CAN FD and various sensors and actuators. To simplify the software deployment, it supports software updates over a CAN FD bootloader. Testing results show that the module reliably handles data communication at a data bit rate of 8 Mbit/s, can be easily updated via CAN FD bootloader and is universally applicable in customized systems.

> [!NOTE]
> The thesis PDF will be added to this documentation. Until then, the pages on this site cite it by chapter and section title.

The thesis is the background for this site, not a part of it. The table says which chapters this site distils and which stay in the thesis only.

| Thesis chapter | On this site | Only in the thesis |
|---|---|---|
| 1 Introduction | [Why CIM](#why-cim) | Formula Student and AMZ background, nomenclature of the board family |
| 2 Theoretical Fundamentals | nothing; see the official sources linked above | CAN and CAN FD basics, RP2040 boot sequence, CMake, FreeRTOS |
| 3 Methodology | [Requirements](#requirements) | V-model, user stories, project plan and retrospective |
| 4 Research | | the previous AMZ system, existing RP2040 CAN boards, bootloader research |
| 5 Architecture & Design | [Hardware overview](hardware/overview.md), [Firmware architecture](firmware/architecture.md), [Bootloader concept](protocol/bootloader.md) | the McuLib-based software layering |
| 6 Hardware Implementation | [Hardware overview](hardware/overview.md), [Pinout](hardware/pinout.md) | Altium workflow, PCB stack-up and assembly, BOM, breakout, carrier and HAT boards |
| 7 Software Implementation | [Bootloader concept](protocol/bootloader.md) (memory layout) | the old firmware on McuLib, CAN Shell, the old bridge and Python host tool, all replaced |
| 8 Testing and Validation | | all measurements: power consumption, regulator heat and ripple, SPI throughput optimisation, bit timing, in-car tests |
| 9 Results and Discussion | | requirements review, weaknesses, future work |

The firmware described in chapters 5 and 7 was built on McuLib and has been rewritten in this repository without it, see [Firmware architecture](firmware/architecture.md#relation-to-the-thesis). The protocol of chapter 5 is version 1; [ADR 0002](adr/0002-device-protocol.md) defines version 2.

[^thesis-intro]: L. Betschart, *CAN FD Interface Module*, master's thesis, HSLU, 2024. Chapter 1 (Introduction), *Low-Level Control and Data Acquisition*, and chapter 3 (Methodology), *Problem Statement*.
[^thesis-req]: Thesis, chapter 3, *Requirements List*.
[^thesis-abstract]: Thesis, *Abstract*, shortened.
