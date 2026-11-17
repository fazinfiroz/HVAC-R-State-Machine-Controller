#ifndef HVAC_CONTROLLER_H
#define HVAC_CONTROLLER_H

#include <stdbool.h>
#include <stdint.h>

#define HVAC_MIN_COMPRESSOR_ON_SECONDS  180u
#define HVAC_MIN_COMPRESSOR_OFF_SECONDS 300u
#define HVAC_DEFROST_SECONDS            30u
#define HVAC_DEFAULT_SETPOINT_C         4.0f
#define HVAC_DEFAULT_HYSTERESIS_C       1.0f

typedef enum {
    HVAC_STATE_OFF_COOLDOWN = 0,
    HVAC_STATE_RUNNING_MIN_ON,
    HVAC_STATE_RUNNING,
    HVAC_STATE_DEFROST,
    HVAC_STATE_FAULT_LOCKOUT
} HvacState;

typedef struct {
    float return_air_temp_c;
    bool sensor_valid;
    bool high_pressure_ok;
    bool low_pressure_ok;
    bool defrost_request;
    bool reset_fault;
} HvacInputs;

typedef struct {
    HvacState state;
    bool compressor_on;
    bool evaporator_fan_on;
    bool alarm_active;
    uint32_t compressor_on_seconds;
    uint32_t compressor_off_seconds;
    uint32_t state_seconds;
    uint32_t total_runtime_seconds;
    uint32_t fault_count;
    char fault_message[64];
} HvacOutputs;

typedef struct {
    float setpoint_c;
    float hysteresis_c;
    uint32_t min_on_seconds;
    uint32_t min_off_seconds;
    uint32_t defrost_seconds;

    HvacState state;
    bool compressor_on;
    bool evaporator_fan_on;
    bool alarm_active;

    uint32_t compressor_on_seconds;
    uint32_t compressor_off_seconds;
    uint32_t state_seconds;
    uint32_t total_runtime_seconds;
    uint32_t fault_count;
    char fault_message[64];
} HvacController;

void hvac_init(HvacController *controller);
void hvac_set_timing(HvacController *controller,
                     uint32_t min_on_seconds,
                     uint32_t min_off_seconds,
                     uint32_t defrost_seconds);
void hvac_tick(HvacController *controller, const HvacInputs *inputs);
HvacOutputs hvac_get_outputs(const HvacController *controller);
const char *hvac_state_name(HvacState state);

#endif
