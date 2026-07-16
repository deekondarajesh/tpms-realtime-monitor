# Real-Time TPMS Sensor Monitoring System

[![CI](https://github.com/deekondarajesh/tpms-realtime-monitor/actions/workflows/ci.yml/badge.svg)](https://github.com/deekondarajesh/tpms-realtime-monitor/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-STM32F103%20%28Cortex--M3%29-blue)](#hardware-target)

Real-time firmware for a 4-channel Tyre Pressure Monitoring System (TPMS),
built on FreeRTOS for the STM32F103C8T6 (Cortex-M3 @ 72MHz). Continuously
samples four pressure sensor channels, applies factory two-point
calibration, checks each reading against safe operating thresholds, and
reports data over CAN bus at 500 kbit/s.

## Why FreeRTOS?

A bare-metal `while(1)` super-loop can handle one job at a time reasonably
well, but this system has several timing-sensitive jobs running
concurrently, with different urgency levels:

- **Four sensor channels need sampling on a fixed period** (50Hz each),
  regardless of how long calibration math or CAN transmission takes
  elsewhere in the system.
- **Alert detection is safety-relevant** and must never be delayed behind
  routine data logging - if a tyre is under/over-pressure, that has to be
  flagged within milliseconds, not whenever the main loop gets around to it.
- **CAN transmission can momentarily block** (e.g. if a transmit mailbox is
  still busy), and that stall must never hold up sensor sampling.

FreeRTOS's preemptive scheduler and priority levels turn these from
"hopefully fine" timing assumptions into structural guarantees: each sensor
channel runs as its own task, alert checks happen inline (not queued) so
they're never delayed by a busy CAN task, and a queue decouples acquisition
from transmission so a slow bus never stalls sampling. See the top comment
in `src/task_scheduler.c` for the full reasoning.

## Architecture

```
 4x vSensorChannelTask (priority 2)          1x vCanTxTask (priority 3)
 ┌─────────────────────────────┐             ┌───────────────────┐
 │ ADC read (per channel)      │             │                   │
 │ → calibration_apply()       │  queue      │ xQueueReceive()   │
 │ → alert_check() [inline]    │ ──────────► │ → can_send_data() │
 │ → xQueueSend() [non-block]  │             │                   │
 └─────────────────────────────┘             └───────────────────┘
```

| Module | Responsibility |
|---|---|
| `main.c` | System clock config (72MHz PLL from 8MHz HSE), scheduler start |
| `task_scheduler.c` | Task/queue creation, ADC driver, task architecture |
| `pressure_calibration.c` | Two-point per-channel linear calibration (±2% accuracy) |
| `alert_detection.c` | Threshold checking, alert GPIO output |
| `can_comm.c` | bxCAN peripheral init (500 kbit/s) and frame transmission |

## Hardware target

- **MCU:** STM32F103C8T6 ("Blue Pill"), Cortex-M3, 72MHz, 64KB flash, 20KB RAM
- **Sensors:** 4x analogue pressure sensor channels on PA0-PA3 (ADC1)
- **CAN bus:** PA11 (RX) / PA12 (TX), 500 kbit/s, requires an external CAN
  transceiver (e.g. TJA1050) between the MCU and the bus
- **Alert output:** PB0 (indicator LED/buzzer driver)

## Results

- **±2% calibration accuracy** across the sensor's operating range, verified
  by unit test against known reference points
- **<100ms alert response latency**, guaranteed by checking thresholds
  inline within each sensor task rather than via a queued/lower-priority path
- **9/9 unit tests passing** on the pure calibration and threshold logic
- Firmware footprint: **~8.2KB flash, ~10KB RAM** (fits comfortably within
  the target's 64KB/20KB budget, confirmed by a real cross-compiled build)

## Building

### Firmware (STM32 target)

Requires the `arm-none-eabi-gcc` toolchain.

```bash
make firmware
```

Produces `build/tpms_monitor.elf` and `build/tpms_monitor.bin`. Flash the
`.bin` to the target with your programmer of choice (e.g. ST-Link:
`st-flash write build/tpms_monitor.bin 0x8000000`).

### Unit tests (native, no hardware required)

```bash
make test
```

Compiles and runs the calibration/alert-threshold logic natively on your PC.
This works without any STM32 hardware because that logic is deliberately
kept separate from anything touching real registers (see the comment at the
top of `tests/unit_tests.c`).

## Continuous integration

Every push and pull request runs two GitHub Actions jobs (see
`.github/workflows/ci.yml`):

1. **Unit tests** - builds and runs the host-side test suite (`make test`)
2. **Firmware build** - installs the ARM GCC toolchain and cross-compiles
   the real STM32 firmware (`make firmware`), uploading the resulting
   `.elf`/`.bin` as a build artifact

This means every commit is verified to both pass its logic tests and
actually compile for the target hardware before it's considered good -
the same principle behind any production release gate.

## Coding standards

`.clang-format` (LLVM-based, 4-space indentation, Allman braces) defines
the formatting standard for this project's own code under `src/` and
`tests/`. Vendored code under `Middlewares/` and `Drivers/CMSIS/` keeps its
upstream formatting rather than being reformatted to match.

## License

This project's own code is MIT licensed - see [LICENSE](LICENSE). It vendors
the FreeRTOS kernel under its own MIT license - see
[Middlewares/FreeRTOS/LICENSE.md](Middlewares/FreeRTOS/LICENSE.md).

## Third-party code

This project vendors the [FreeRTOS kernel](https://github.com/FreeRTOS/FreeRTOS-Kernel)
(MIT licensed) under `Middlewares/FreeRTOS/`, configured for the Cortex-M3
GCC port. `Drivers/CMSIS/stm32f103xb.h` is a deliberately trimmed-down,
hand-written register header covering only the peripherals this project
uses (RCC, GPIO, ADC1, bxCAN), rather than the full vendor CMSIS pack, so
the register map stays easy to read and audit.
