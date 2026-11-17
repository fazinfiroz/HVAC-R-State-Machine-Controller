#include "hvac_controller.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_status(uint32_t elapsed, float temp,
                         const HvacOutputs *out)
{
    printf("%5u s | Temp: %5.1f C | State: %-16s | Compressor: %-3s"
           " | Fan: %-3s | ON timer: %3u | OFF timer: %3u",
           elapsed, temp, hvac_state_name(out->state),
           out->compressor_on ? "ON" : "OFF",
           out->evaporator_fan_on ? "ON" : "OFF",
           out->compressor_on_seconds,
           out->compressor_off_seconds);

    if (out->alarm_active) {
        printf(" | ALARM: %s", out->fault_message);
    }
    putchar('\n');
}

int main(int argc, char **argv)
{
    HvacController controller;
    HvacInputs inputs = {
        .return_air_temp_c = 8.0f,
        .sensor_valid = true,
        .high_pressure_ok = true,
        .low_pressure_ok = true,
        .defrost_request = false,
        .reset_fault = false
    };
    uint32_t duration = 900u;
    uint32_t t;

    if (argc > 1) {
        char *end = NULL;
        unsigned long parsed = strtoul(argv[1], &end, 10);
        if (end == argv[1] || *end != '\0' || parsed == 0ul) {
            fprintf(stderr, "Usage: %s [simulation_seconds]\n", argv[0]);
            return EXIT_FAILURE;
        }
        duration = (uint32_t)parsed;
    }

    hvac_init(&controller);

    /*
     * Shorter timers make the demo observable in a quick terminal run.
     * The production/default configuration remains 180 s ON / 300 s OFF.
     * Run with --production-timers to use the actual configured values.
     */
    if (argc < 3 || strcmp(argv[2], "--production-timers") != 0) {
        hvac_set_timing(&controller, 15u, 20u, 10u);
        puts("DEMO MODE: minimum ON=15s, minimum OFF=20s, defrost=10s");
        puts("Use: hvac_sim 900 --production-timers for 180s/300s timers.");
    } else {
        puts("PRODUCTION CONFIGURATION: minimum ON=180s, minimum OFF=300s");
    }

    puts("Scenario: warm start, cooling demand, temperature recovery, defrost.");
    puts("Time  | Temperature | Controller status");
    puts("---------------------------------------------------------------");

    for (t = 1u; t <= duration; ++t) {
        /* Warm initially; then cool down after compressor starts. */
        if (t < 40u) {
            inputs.return_air_temp_c = 8.0f;
        } else if (t < 75u) {
            inputs.return_air_temp_c = 3.0f;
        } else {
            inputs.return_air_temp_c = 4.0f;
        }

        /* Request a brief defrost cycle in the demo. */
        inputs.defrost_request = (t == 100u);

        hvac_tick(&controller, &inputs);

        if (t == 1u || t % 5u == 0u || t == 100u || t == 110u) {
            HvacOutputs out = hvac_get_outputs(&controller);
            print_status(t, inputs.return_air_temp_c, &out);
        }
    }

    {
        HvacOutputs out = hvac_get_outputs(&controller);
        puts("\nSimulation summary");
        printf("Final state: %s\n", hvac_state_name(out.state));
        printf("Total compressor runtime: %u seconds\n",
               out.total_runtime_seconds);
        printf("Fault count: %u\n", out.fault_count);
    }

    return EXIT_SUCCESS;
}
