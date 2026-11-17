#include "../src/hvac_controller.h"

#include <assert.h>
#include <stdio.h>

static HvacInputs normal_inputs(float temp)
{
    HvacInputs in = {
        .return_air_temp_c = temp,
        .sensor_valid = true,
        .high_pressure_ok = true,
        .low_pressure_ok = true,
        .defrost_request = false,
        .reset_fault = false
    };
    return in;
}

static void test_minimum_off_timer(void)
{
    HvacController c;
    HvacInputs in = normal_inputs(8.0f);
    unsigned i;

    hvac_init(&c);
    hvac_set_timing(&c, 3u, 5u, 2u);

    for (i = 0; i < 4; ++i) {
        hvac_tick(&c, &in);
        assert(!c.compressor_on);
    }

    hvac_tick(&c, &in);
    assert(c.compressor_on);
    assert(c.state == HVAC_STATE_RUNNING_MIN_ON);
    puts("[PASS] Compressor cannot start before minimum OFF timer expires");
}

static void test_minimum_on_timer(void)
{
    HvacController c;
    HvacInputs in = normal_inputs(8.0f);

    hvac_init(&c);
    hvac_set_timing(&c, 3u, 0u, 2u);

    hvac_tick(&c, &in); /* Start compressor immediately. */
    assert(c.compressor_on);

    in.return_air_temp_c = 3.0f; /* Satisfied temperature demand. */
    hvac_tick(&c, &in);
    hvac_tick(&c, &in);
    assert(c.compressor_on);

    hvac_tick(&c, &in);
    assert(!c.compressor_on);
    assert(c.state == HVAC_STATE_OFF_COOLDOWN);
    puts("[PASS] Normal stop is delayed until minimum ON timer expires");
}

static void test_high_pressure_trip_overrides_minimum_on(void)
{
    HvacController c;
    HvacInputs in = normal_inputs(8.0f);

    hvac_init(&c);
    hvac_set_timing(&c, 100u, 0u, 2u);
    hvac_tick(&c, &in);
    assert(c.compressor_on);

    in.high_pressure_ok = false;
    hvac_tick(&c, &in);

    assert(!c.compressor_on);
    assert(c.state == HVAC_STATE_FAULT_LOCKOUT);
    assert(c.alarm_active);
    puts("[PASS] High-pressure safety trip immediately stops compressor");
}

static void test_fault_reset(void)
{
    HvacController c;
    HvacInputs in = normal_inputs(8.0f);

    hvac_init(&c);
    in.sensor_valid = false;
    hvac_tick(&c, &in);
    assert(c.state == HVAC_STATE_FAULT_LOCKOUT);

    in.sensor_valid = true;
    in.reset_fault = true;
    hvac_tick(&c, &in);

    assert(c.state == HVAC_STATE_OFF_COOLDOWN);
    assert(!c.alarm_active);
    assert(!c.compressor_on);
    puts("[PASS] Healthy interlocks plus reset clear fault into OFF_COOLDOWN");
}

static void test_defrost(void)
{
    unsigned i;
    HvacController c;
    HvacInputs in = normal_inputs(8.0f);

    hvac_init(&c);
    hvac_set_timing(&c, 3u, 0u, 2u);
    in.defrost_request = true;
    hvac_tick(&c, &in);
    assert(c.state == HVAC_STATE_DEFROST);
    assert(!c.compressor_on);
    assert(!c.evaporator_fan_on);

    in.defrost_request = false;
    for (i = 0; i < 2; ++i) {
        hvac_tick(&c, &in);
    }

    assert(c.state == HVAC_STATE_OFF_COOLDOWN);
    puts("[PASS] Defrost stops compressor/fan and returns to OFF_COOLDOWN");
}

int main(void)
{
    test_minimum_off_timer();
    test_minimum_on_timer();
    test_high_pressure_trip_overrides_minimum_on();
    test_fault_reset();
    test_defrost();

    puts("\nAll controller tests passed.");
    return 0;
}
