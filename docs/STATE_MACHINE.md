# State machine design

## States

| State | Compressor | Evaporator fan | Purpose |
|---|---|---|---|
| `OFF_COOLDOWN` | OFF | ON | Enforces compressor minimum OFF time |
| `RUNNING_MIN_ON` | ON | ON | Prevents normal temperature stop before minimum ON time |
| `RUNNING` | ON | ON | Normal cooling operation |
| `DEFROST` | OFF | OFF | Simulated defrost interval |
| `FAULT_LOCKOUT` | OFF | OFF | Safety shutdown until healthy inputs and manual reset |

## Temperature control

Default setpoint is 4.0 °C and total hysteresis band is 1.0 °C.

- Cooling demand starts at or above 4.5 °C.
- Normal cooling stop is requested at or below 3.5 °C.
- Between 3.5 °C and 4.5 °C, the controller retains its current demand state.

## Compressor protection

- Minimum ON: 180 seconds.
- Minimum OFF: 300 seconds.
- The OFF timer begins when the compressor stops.
- The ON timer begins when the compressor starts.
- Safety interlocks override minimum ON time and stop the compressor immediately.

## Safety behaviour

Invalid temperature sensor, high-pressure trip, or low-pressure trip enters
`FAULT_LOCKOUT`. The compressor and evaporator fan are de-energised in the
simulation. A reset is accepted only after all interlock inputs are healthy.

## Simulation limitation

This is an educational software simulation. It is not a certified HVAC controller
and must not be connected directly to compressor hardware.
