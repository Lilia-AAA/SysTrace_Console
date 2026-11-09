# SysTrace Console

A device-level logging and diagnostics terminal running on an STM32F103 under FreeRTOS.

SysTrace Console turns a small STM32 board into a self-contained "black box": it accepts
commands over a serial console, reports live system state, drives the board's peripherals,
and records device events into an external SPI NOR flash so they survive a power cycle.

The whole firmware fits in **64 KB of flash and 20 KB of RAM** — it currently uses about
**49 KB of flash (77%)** and **12.3 KB of RAM (62%)**, the latter including an 8.2 KB
FreeRTOS heap.

---

## Features

- **Command console over UART** — 11 commands covering peripheral control, system status,
  log retrieval and RTC configuration. The command table is an array of function pointers,
  so adding a command is one table entry plus one parser function.
- **Two-stage logging** — events are buffered in a 16-entry RAM ring and then flushed
  asynchronously to SPI flash. The flash side is a four-sector ring (4 × 4 KB) that erases
  and wraps as it fills; the boot scan recovers the write cursor by finding the highest
  record ID, so logs stay continuous across resets and power cycles.
- **Live system snapshot** — per-task stack high-water marks, heap usage, queue occupancy,
  RTC time and device state, all in one `STATUS` command.
- **Key interaction** — debouncing, long press and double click handled by an explicit state
  machine; key events enter the same event queue as console commands.
- **Watchdog that detects deadlocks** — the IWDG is refreshed *only* from the idle hook, so any
  task that hangs or monopolises the CPU will starve the idle task and reset the board within
  about four seconds.
- **Persistent RTC** — date and time survive resets. Since the STM32F1 RTC has no date
  register at all, the calendar is mirrored into backup registers `BKP_DR2` / `BKP_DR3`
  and restored at boot.

---

## Hardware and Toolchain

| Item | Detail |
|---|---|
| MCU | STM32F103xB, Cortex-M3, 64 KB flash / 20 KB RAM, 72 MHz |
| RTOS | FreeRTOS V11.3.0, heap_4, preemptive, 1000 Hz tick |
| Peripherals | USART1 @ 115200 8N1 (DMA1_Channel5 + IDLE reception), SPI1 (W25Q NOR flash), IWDG, RTC (LSE), GPIO |
| Toolchain | CMake + Ninja + arm-none-eabi-gcc, STM32Cube HAL |
| IDE | VS Code + Cortex-Debug + ST-Link |

---

## Architecture

Three layers on top of the CubeMX-generated hardware initialisation:

```
Core/        Clock, GPIO, DMA, USART, SPI, IWDG and RTC initialisation (HAL)
App/         uart_dma_rx, uart_console, event_process, key_control,
             log_storage, sys_monitor
Utils/       print_helper (serialised output), w25q_flash (flash driver), get_rtc
FreeRTOS/    Kernel sources
```

### Tasks

| Task | Priority | Stack (words) | Responsibility |
|---|---|---|---|
| `Event_Process_Task` | 3 | 260 | Event dispatch, peripheral actions, log generation |
| `DMA_Task` | 2 | 400 | Line assembly, command parsing and dispatch |
| `Sys_Monitor_Task` | 1 | 240 | 10 ms periodic snapshot collection |
| `Log_Flush_Task` | 1 | 160 | Async log flush to flash |
| `UART_Gatekeeper_Task` | 1 | 100 | Serialises all UART transmission |
| `Key_Control_Task` | 1 | 80 | Key state machine |
| `Beep_Task` | 1 | 70 | Buzzer timing |

### Data Flow

```
key press / UART command
        │
        ▼
  event queue (Log_t, 32 B × 3)
        │
        ▼
 Event_Process_Task ──► LED / buzzer / command response
        │
        ▼
 Log_add ──► RAM ring buffer (16 × 32 B)
        │  threshold of 4 reached → binary semaphore
        ▼
 Log_Flush_Task ──► W25Q SPI flash (sequential append)

any task ──► PrintStr ──► print queue (96 B × 3) ──► Gatekeeper ──► UART TX
```

---

## Modules

### Serial command console

Reception uses DMA with the UART idle interrupt. Because the idle interrupt fires per *burst*
of bytes rather than per *line*, `DMA_Task` assembles complete lines across bursts and only
dispatches on `\r` or `\n`; the terminal must terminate each command with a newline.

The receive callback also filters the HAL half-transfer event: the HAL forwards
`HAL_UART_RXEVENT_HT` to `HAL_UARTEx_RxEventCallback()` with `Size = RxXferSize / 2`, which
would otherwise be mistaken for a complete reception and split every command longer than half
the buffer.

### Two-stage logging

Each log record is a fixed 32-byte structure — event type, parameter, 6-byte RTC timestamp,
monotonic sequence number and a validity magic. In flash the four 4 KB sectors are chained
into a ring: when the current sector fills, the *next* sector is erased before writing, so the
region wraps `sector 0 → 1 → 2 → 3 → 0` and old records are overwritten in order.

A linear "stop at the first empty slot" scan is not enough once the ring has wrapped, because
the lowest addresses may then hold the newest data. `Flash_Log_Init` therefore walks all 512
slots, keeps every record with a valid magic and selects the highest ID via
`(int16_t)(id - max_id) > 0`; the write cursor is the slot after it. IDs are 16-bit and wrap,
and the signed difference makes that 65535 → 0 transition compare correctly. `READ FLASHLOG`
prints sector by sector, so after a wrap the output order is not chronological — use the IDs.

RAM ring updates and the "slot consumed" bookkeeping run inside a critical section, and a
record is only marked consumed after the flash write is verified.

### System monitor

The `STATUS` command emits a `Status_snapshot_t`: device state, RTC time, system tick, heap
total / free / minimum-ever, plus stack high-water marks for all seven tasks and occupancy for
all five queues. The task stack sizes in the table above were tuned from these measurements.

### Key control

20 ms debounce, 1 s long press and a 300 ms double-click window, implemented as an explicit
state machine (`KEY_IDLE`, `KEY_DEBOUNCE`, `KEY_DOWN`, `KEY_WAIT_SECOND`, `KEY_DEBOUNCE2`,
`KEY_WAIT_UP`). Key port is PA8, status LED is PB1.

### Reliability

- **Watchdog as a deadlock detector.** The IWDG runs from the LSI (about 4 s timeout) and is
  refreshed exclusively in `vApplicationIdleHook()`. A task that blocks forever or hogs the
  CPU prevents the idle task from running, so the board resets instead of silently hanging.
  `__HAL_DBGMCU_FREEZE_IWDG()` keeps the watchdog quiet while debugging.
- **Stack overflow detection** via `configCHECK_FOR_STACK_OVERFLOW = 2`; `configASSERT`
  records the file name and line number of the failing assertion.
- **Reset cause reporting** at startup, decoded from the RCC reset flags.

### RTC persistence

The RTC is clocked from a 32.768 kHz crystal (LSE). On the STM32F1 the RTC is only a 32-bit
seconds counter — there is no date register — so `HAL_RTC_GetTime()` advances a RAM calendar
(`hrtc.DateToUpdate`) by whole days whenever the counter passes a 24-hour boundary.

A magic value (`0x2005`) in `BKP_DR1` prevents the initial time from being rewritten on every
reset. Because `HAL_RTC_Init()` clears the RAM calendar to 00-01-01 on each boot, the date is
mirrored into `BKP_DR2` (year, month) and `BKP_DR3` (weekday, day) whenever the day changes,
and restored by writing the fields back into `hrtc.DateToUpdate` — not via `HAL_RTC_SetDate()`,
which would truncate the whole days accumulated in the counter. The backup domain keeps the
counter running while the MCU is off, so the next `HAL_RTC_GetTime()` catches the calendar up
on the days that elapsed.

---

## Design Decisions

| Problem | Decision |
|---|---|
| Interleaved output when several tasks print | Funnel all transmission through a single Gatekeeper task instead of adding a lock. Serialisation comes for free and concurrent `HAL_UART_Transmit()` calls never return `HAL_BUSY`. |
| Commands split across several UART events | Assemble lines in the task and dispatch only on CR/LF, rather than assuming one event equals one command. |
| HAL half-transfer event mistaken for a completed reception | Filter `HAL_UART_RXEVENT_HT` in the callback. |
| Log consistency on power loss | Fixed-size records, sequential append, no erase-while-writing. A torn record is discarded by the magic check; existing records are never invalidated. |
| Recovering the write cursor after the sector ring wraps | At boot, find the record with the highest ID instead of stopping at the first erased slot; the signed 16-bit difference compares correctly across the 65535 → 0 ID wrap. |
| Seven tasks in 20 KB of RAM | Dynamic creation from an 8.2 KB heap_4 pool, with stack high-water marks used continuously to trim each task's stack to its real requirement. |

---

## Resource Footprint

| Region | Used | Total | Ratio |
|---|---|---|---|
| Flash (.text + .data) | ~49 KB | 64 KB | 77% |
| RAM (.data + .bss, incl. 8.2 KB RTOS heap) | ~12.3 KB | 20 KB | 62% |

---

## Command Reference

| Command | Description |
|---|---|
| `LED ON` / `LED OFF` | Switch the status LED |
| `BEEP <ms>` | Sound the buzzer for the given duration |
| `STATUS` | Print a full system snapshot |
| `LOCK` / `UNLOCK` | Lock or unlock the device |
| `READ LOG` | Dump the RAM log ring |
| `CLEAR LOG` | Clear the RAM log ring |
| `READ FLASHLOG` | Dump the records stored in flash |
| `SET TIME YYYY/MM/DD HH:MM:SS` | Set the RTC date and time |
| `HELP` | List available commands |

Input is case-insensitive; `-`, `_` and tabs are normalised to spaces, so use `/` as the date
separator. The buffer holds 40 bytes per burst — keep commands short.

---

## Build

```bash
cmake -B build/Debug -G Ninja \
      -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake
cmake --build build/Debug
```

The resulting image is `build/Debug/SysTrace_Console.elf`; flash it with ST-Link.

---

## Repository Layout

```
Core/         CubeMX-generated initialisation and interrupt handlers
App/          Application modules (console, event processing, logging, monitoring)
Utils/        Reusable helpers (print gatekeeper, W25Q driver, RTC access)
FreeRTOS/     Kernel sources and FreeRTOSConfig.h
Drivers/      STM32F1 HAL and CMSIS
cmake/        Toolchain file and CubeMX-generated build glue
```

---

## Future Work

- Add a CRC to each log record to detect torn writes explicitly.
- Print `READ FLASHLOG` output in ID order so it stays chronological after the sector ring wraps.
- Replace dynamic allocation with `xTaskCreateStatic` to remove heap fragmentation risk.
- Add a binary/RTT channel for debug output to reduce pressure on the console UART.
