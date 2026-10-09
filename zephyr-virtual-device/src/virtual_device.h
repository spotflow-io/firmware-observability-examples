#ifndef ZEPHYR_VIRTUAL_DEVICE_VIRTUAL_DEVICE_H
#define ZEPHYR_VIRTUAL_DEVICE_VIRTUAL_DEVICE_H

#include <stdbool.h>

/* Parses the run-time credentials block. Call once, before the network starts. */
void virtual_device_init(void);

/* True on the first boot of this QEMU run, false after a warm reset. */
bool virtual_device_is_first_boot(void);

/* Seconds after the first boot at which the demo crash fires, 0 if disabled. */
int virtual_device_crash_after_seconds(void);

/* True once the user pressed 'c' in the console. */
bool virtual_device_crash_requested(void);

#endif /* ZEPHYR_VIRTUAL_DEVICE_VIRTUAL_DEVICE_H */
