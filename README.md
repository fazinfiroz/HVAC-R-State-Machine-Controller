# HVAC/R State Machine Controller & Compressor Protection Simulation

A C11 educational simulation of a refrigeration controller using a deterministic
finite state machine. It demonstrates compressor anti-short-cycle timing,
temperature hysteresis, safety interlocks, defrost sequencing, and unit tests.

## Features

- Compressor minimum ON time: 180 seconds (default configuration)
- Compressor minimum OFF time: 300 seconds (default configuration)
- Temperature setpoint and hysteresis control
- High-pressure, low-pressure, and sensor-fault lockout
- Defrost state simulation
- C11 implementation and IEC 61131-3 Structured Text reference
- Automated unit tests

## Requirements

- GCC (MinGW-w64 or MSYS2) or another C11-compatible compiler
- Windows PowerShell, Command Prompt, Linux, or macOS terminal

## Build on Windows (GCC)

Open PowerShell in the project root:

```powershell
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -Isrc src/main.c src/hvac_controller.c -o hvac_sim.exe
```

Run the demonstration (short timers so transitions are visible):

```powershell
.\hvac_sim.exe 150
```

Run with the configured 180-second minimum ON and 300-second minimum OFF timers:

```powershell
.\hvac_sim.exe 900 --production-timers
```

## Build and run tests

```powershell
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -Isrc tests/test_controller.c src/hvac_controller.c -o test_controller.exe
.\test_controller.exe
```

Expected test cases:

1. Compressor cannot start before minimum OFF timer expires.
2. Normal compressor stop is delayed until minimum ON timer expires.
3. High-pressure safety trip overrides minimum ON protection.
4. Fault reset requires healthy interlocks.
5. Defrost stops the compressor and fan and returns to OFF_COOLDOWN.

## Important implementation notes

The demo executable uses shorter timer values (15 s ON / 20 s OFF / 10 s
defrost) so the behaviour can be observed quickly. The controller defaults and
the `--production-timers` mode use the requested 180 s ON / 300 s OFF values.

The C simulation advances exactly one logical second each time `hvac_tick()`
is called. A real embedded or PLC implementation must call its control task
from a reliable periodic scheduler and use appropriate hardware safety design.

## Repository layout

```text
src/                 C controller and simulation entry point
tests/               Unit tests
structured-text/     IEC 61131-3 Structured Text reference
docs/                State-machine design notes
```

## Safety disclaimer

This repository is a learning and simulation project. It is not a certified
safety system, and it must not be used to control real refrigeration equipment
without engineering review, suitable hardware protections, and commissioning.
