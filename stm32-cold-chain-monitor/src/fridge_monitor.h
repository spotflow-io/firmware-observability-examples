#ifndef STM32_COLD_CHAIN_FRIDGE_MONITOR_H
#define STM32_COLD_CHAIN_FRIDGE_MONITOR_H

#include <stdbool.h>
#include <stdint.h>

int  fridge_monitor_init(void);
void fridge_monitor_step(void);

/* Simulate a temperature excursion (a door left open or a compressor fault)
 * that drives the cabinet past its safe limit. Wired to the user button and the
 * on-screen button. */
void fridge_monitor_simulate_excursion(void);

/* Shared state, read by the local LVGL display. */
float    fridge_monitor_last_temp(void);
uint32_t fridge_monitor_read_errors(void);
bool     fridge_monitor_in_excursion(void);

#endif /* STM32_COLD_CHAIN_FRIDGE_MONITOR_H */
