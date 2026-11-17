(*
    IEC 61131-3 Structured Text reference implementation.

    This is an educational reference. Type declaration placement and TON
    invocation syntax may require adaptation for the target PLC vendor.

    Timers are invoked exactly once per PLC scan below. MinOffTimer runs only
    while the state is OFF_COOLDOWN, so time spent in DEFROST or FAULT_LOCKOUT
    does not count toward the post-condition minimum OFF interval.
*)

TYPE E_HVAC_STATE :
(
    OFF_COOLDOWN,
    RUNNING_MIN_ON,
    RUNNING,
    DEFROST,
    FAULT_LOCKOUT
);
END_TYPE

PROGRAM HVAC_Controller
VAR
    ReturnAirTemp_C : REAL := 8.0;
    Setpoint_C : REAL := 4.0;
    Hysteresis_C : REAL := 1.0;

    SensorValid : BOOL := TRUE;
    HighPressureOK : BOOL := TRUE;
    LowPressureOK : BOOL := TRUE;
    DefrostRequest : BOOL := FALSE; (* Treat as a one-shot request/pulse. *)
    ResetFault : BOOL := FALSE;

    CompressorOutput : BOOL := FALSE;
    EvaporatorFanOutput : BOOL := TRUE;
    AlarmOutput : BOOL := FALSE;

    State : E_HVAC_STATE := OFF_COOLDOWN;

    MinOnTimer : TON;
    MinOffTimer : TON;
    DefrostTimer : TON;

    FaultText : STRING(80) := '';
END_VAR

(* Call each timer once per scan. *)
MinOnTimer(IN := CompressorOutput, PT := T#180s);
MinOffTimer(
    IN := (State = OFF_COOLDOWN) AND NOT CompressorOutput,
    PT := T#300s
);
DefrostTimer(IN := (State = DEFROST), PT := T#30s);

(* Safety interlocks have priority over all normal sequencing. *)
IF NOT SensorValid THEN
    CompressorOutput := FALSE;
    EvaporatorFanOutput := FALSE;
    AlarmOutput := TRUE;
    FaultText := 'Temperature sensor fault';
    State := FAULT_LOCKOUT;

ELSIF NOT HighPressureOK THEN
    CompressorOutput := FALSE;
    EvaporatorFanOutput := FALSE;
    AlarmOutput := TRUE;
    FaultText := 'High pressure safety trip';
    State := FAULT_LOCKOUT;

ELSIF NOT LowPressureOK THEN
    CompressorOutput := FALSE;
    EvaporatorFanOutput := FALSE;
    AlarmOutput := TRUE;
    FaultText := 'Low pressure safety trip';
    State := FAULT_LOCKOUT;

ELSIF State = FAULT_LOCKOUT THEN
    CompressorOutput := FALSE;
    EvaporatorFanOutput := FALSE;

    IF ResetFault AND SensorValid AND HighPressureOK AND LowPressureOK THEN
        AlarmOutput := FALSE;
        FaultText := '';
        State := OFF_COOLDOWN;
    END_IF;

ELSIF DefrostRequest AND (State <> DEFROST) THEN
    CompressorOutput := FALSE;
    EvaporatorFanOutput := FALSE;
    State := DEFROST;

ELSIF State = DEFROST THEN
    CompressorOutput := FALSE;
    EvaporatorFanOutput := FALSE;

    IF DefrostTimer.Q THEN
        EvaporatorFanOutput := TRUE;
        State := OFF_COOLDOWN;
    END_IF;

ELSE
    CASE State OF

        OFF_COOLDOWN:
            CompressorOutput := FALSE;
            EvaporatorFanOutput := TRUE;

            IF (ReturnAirTemp_C >= (Setpoint_C + Hysteresis_C / 2.0))
               AND MinOffTimer.Q THEN
                CompressorOutput := TRUE;
                State := RUNNING_MIN_ON;
            END_IF;

        RUNNING_MIN_ON:
            CompressorOutput := TRUE;
            EvaporatorFanOutput := TRUE;

            IF MinOnTimer.Q THEN
                IF ReturnAirTemp_C <= (Setpoint_C - Hysteresis_C / 2.0) THEN
                    CompressorOutput := FALSE;
                    State := OFF_COOLDOWN;
                ELSE
                    State := RUNNING;
                END_IF;
            END_IF;

        RUNNING:
            CompressorOutput := TRUE;
            EvaporatorFanOutput := TRUE;

            IF ReturnAirTemp_C <= (Setpoint_C - Hysteresis_C / 2.0) THEN
                CompressorOutput := FALSE;
                State := OFF_COOLDOWN;
            END_IF;

        ELSE
            CompressorOutput := FALSE;
            EvaporatorFanOutput := FALSE;
            AlarmOutput := TRUE;
            FaultText := 'Unexpected state';
            State := FAULT_LOCKOUT;

    END_CASE;
END_IF;

END_PROGRAM
