#include "hvac_controller.h"

#include <stdio.h>
#include <string.h>

static void set_state(HvacController *c, HvacState state)
{
    if (c->state != state) {
        c->state = state;
        c->state_seconds = 0u;
    }
}

static void stop_compressor(HvacController *c)
{
    if (c->compressor_on) {
        c->compressor_on = false;
        c->compressor_off_seconds = 0u;
    }
}

static void start_compressor(HvacController *c)
{
    if (!c->compressor_on) {
        c->compressor_on = true;
        c->compressor_on_seconds = 0u;
    }
}

static void enter_fault(HvacController *c, const char *message)
{
    if (c->state != HVAC_STATE_FAULT_LOCKOUT) {
        c->fault_count++;
    }

    stop_compressor(c);
    c->evaporator_fan_on = false;
    c->alarm_active = true;
    (void)snprintf(c->fault_message, sizeof(c->fault_message), "%s", message);
    set_state(c, HVAC_STATE_FAULT_LOCKOUT);
}

void hvac_init(HvacController *c)
{
    if (c == NULL) {
        return;
    }

    memset(c, 0, sizeof(*c));
    c->setpoint_c = HVAC_DEFAULT_SETPOINT_C;
    c->hysteresis_c = HVAC_DEFAULT_HYSTERESIS_C;
    c->min_on_seconds = HVAC_MIN_COMPRESSOR_ON_SECONDS;
    c->min_off_seconds = HVAC_MIN_COMPRESSOR_OFF_SECONDS;
    c->defrost_seconds = HVAC_DEFROST_SECONDS;

    /*
     * Conservative startup: require the minimum OFF interval before
     * the first compressor start.
     */
    c->compressor_off_seconds = 0u;
    c->state = HVAC_STATE_OFF_COOLDOWN;
    c->compressor_on = false;
    c->evaporator_fan_on = true;
}

void hvac_set_timing(HvacController *c,
                     uint32_t min_on_seconds,
                     uint32_t min_off_seconds,
                     uint32_t defrost_seconds)
{
    if (c == NULL) {
        return;
    }

    c->min_on_seconds = min_on_seconds;
    c->min_off_seconds = min_off_seconds;
    c->defrost_seconds = defrost_seconds;
}

void hvac_tick(HvacController *c, const HvacInputs *in)
{
    bool cooling_demand;

    if (c == NULL || in == NULL) {
        return;
    }

    c->state_seconds++;

    if (c->compressor_on) {
        c->compressor_on_seconds++;
        c->total_runtime_seconds++;
    } else {
        c->compressor_off_seconds++;
    }

    /* Safety interlocks always take priority over normal sequencing. */
    if (!in->sensor_valid) {
        enter_fault(c, "Temperature sensor fault");
        return;
    }
    if (!in->high_pressure_ok) {
        enter_fault(c, "High pressure safety trip");
        return;
    }
    if (!in->low_pressure_ok) {
        enter_fault(c, "Low pressure safety trip");
        return;
    }

    if (c->state == HVAC_STATE_FAULT_LOCKOUT) {
        stop_compressor(c);
        c->evaporator_fan_on = false;

        /*
         * Reset is accepted only after all input interlocks are healthy.
         * The controller then returns to OFF_COOLDOWN; the minimum OFF
         * timer still has to expire before a restart.
         */
        if (in->reset_fault) {
            c->alarm_active = false;
            c->fault_message[0] = '\0';
            c->compressor_off_seconds = 0u;
            c->evaporator_fan_on = true;
            set_state(c, HVAC_STATE_OFF_COOLDOWN);
        }
        return;
    }

    if (in->defrost_request && c->state != HVAC_STATE_DEFROST) {
        stop_compressor(c);
        c->evaporator_fan_on = false;
        set_state(c, HVAC_STATE_DEFROST);
        return;
    }

    if (c->state == HVAC_STATE_DEFROST) {
        stop_compressor(c);
        c->evaporator_fan_on = false;

        if (c->state_seconds >= c->defrost_seconds) {
            c->evaporator_fan_on = true;
            c->compressor_off_seconds = 0u;
            set_state(c, HVAC_STATE_OFF_COOLDOWN);
        }
        return;
    }

    cooling_demand =
        in->return_air_temp_c >= (c->setpoint_c + (c->hysteresis_c / 2.0f));

    switch (c->state) {
    case HVAC_STATE_OFF_COOLDOWN:
        c->compressor_on = false;
        c->evaporator_fan_on = true;

        if (cooling_demand &&
            c->compressor_off_seconds >= c->min_off_seconds) {
            start_compressor(c);
            set_state(c, HVAC_STATE_RUNNING_MIN_ON);
        }
        break;

    case HVAC_STATE_RUNNING_MIN_ON:
        c->compressor_on = true;
        c->evaporator_fan_on = true;

        /*
         * Minimum ON protection: ignore a normal temperature stop request
         * until the configured minimum run time has elapsed.
         */
        if (c->compressor_on_seconds >= c->min_on_seconds) {
            if (in->return_air_temp_c <=
                (c->setpoint_c - (c->hysteresis_c / 2.0f))) {
                stop_compressor(c);
                set_state(c, HVAC_STATE_OFF_COOLDOWN);
            } else {
                set_state(c, HVAC_STATE_RUNNING);
            }
        }
        break;

    case HVAC_STATE_RUNNING:
        c->compressor_on = true;
        c->evaporator_fan_on = true;

        if (in->return_air_temp_c <=
            (c->setpoint_c - (c->hysteresis_c / 2.0f))) {
            stop_compressor(c);
            set_state(c, HVAC_STATE_OFF_COOLDOWN);
        }
        break;

    case HVAC_STATE_DEFROST:
    case HVAC_STATE_FAULT_LOCKOUT:
    default:
        /* Defensive fallback: fail safe. */
        enter_fault(c, "Unexpected controller state");
        break;
    }
}

HvacOutputs hvac_get_outputs(const HvacController *c)
{
    HvacOutputs out = {0};

    if (c == NULL) {
        return out;
    }

    out.state = c->state;
    out.compressor_on = c->compressor_on;
    out.evaporator_fan_on = c->evaporator_fan_on;
    out.alarm_active = c->alarm_active;
    out.compressor_on_seconds = c->compressor_on_seconds;
    out.compressor_off_seconds = c->compressor_off_seconds;
    out.state_seconds = c->state_seconds;
    out.total_runtime_seconds = c->total_runtime_seconds;
    out.fault_count = c->fault_count;
    (void)snprintf(out.fault_message, sizeof(out.fault_message), "%s",
                   c->fault_message);
    return out;
}

const char *hvac_state_name(HvacState state)
{
    switch (state) {
    case HVAC_STATE_OFF_COOLDOWN: return "OFF_COOLDOWN";
    case HVAC_STATE_RUNNING_MIN_ON: return "RUNNING_MIN_ON";
    case HVAC_STATE_RUNNING: return "RUNNING";
    case HVAC_STATE_DEFROST: return "DEFROST";
    case HVAC_STATE_FAULT_LOCKOUT: return "FAULT_LOCKOUT";
    default: return "UNKNOWN";
    }
}
