#ifndef SENSOR_H
#define SENSOR_H

#include <stdint.h>

/**
 * @brief Opaque handle to a sensor instance.
 *
 * Obtained from sensor_init() and passed to every subsequent sensor call.
 * The handle becomes NULL after sensor_deinit() is called — any further use
 * triggers a fault that is captured as a Zephyr coredump and sent to Spotflow.
 */
typedef struct sensor_handle sensor_handle_t;

/**
 * @brief Sensor reading containing temperature and humidity.
 */
typedef struct {
	/** Temperature in degrees Celsius. */
	double temperature_c;
	/** Relative humidity in percent. */
	double humidity_pct;
} sensor_reading_t;

/**
 * @brief Initialize the (simulated) I2C sensor.
 *
 * @return Pointer to an allocated sensor handle, or NULL on failure.
 */
sensor_handle_t *sensor_init(void);

/**
 * @brief Read temperature and humidity from the sensor.
 *
 * @param handle  Sensor handle returned by sensor_init().
 *                Passing NULL causes a kernel panic — this is intentional in
 *                the example to demonstrate coredump capture.
 * @param out     Output reading populated on success.
 * @return        0 on success, negative errno on failure.
 */
int sensor_read(sensor_handle_t *handle, sensor_reading_t *out);

/**
 * @brief Release sensor resources.
 *
 * After this call the handle is freed. Callers must not use the handle again.
 *
 * @param handle  Sensor handle to release.
 */
void sensor_deinit(sensor_handle_t *handle);

#endif /* SENSOR_H */
