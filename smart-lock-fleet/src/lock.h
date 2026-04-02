#ifndef SMART_LOCK_LOCK_H
#define SMART_LOCK_LOCK_H

/**
 * @brief Register all lock-related custom metrics with the Spotflow SDK.
 *
 * Must be called once from main() after the network is up, before any
 * lock operations are simulated or reported.
 *
 * Registers:
 *   - lock_operation_duration_ms  (float, 1MIN, labels: operation, method)
 *   - door_opened                 (int,   NONE, no labels)
 *   - auth_failure                (int,   NONE, label: method)
 *
 * @return 0 on success, negative errno on failure.
 */
int init_lock_metrics(void);

/**
 * @brief Simulate one lock or unlock operation and report metrics.
 *
 * Randomly picks an operation (lock/unlock) and a method (nfc/keypad/bluetooth),
 * simulates an operation duration, and reports it via Spotflow. Occasionally
 * injects a simulated authentication failure before a successful operation.
 *
 * Also fires a door_opened event when the lock operation succeeds.
 */
void simulate_lock_operation(void);

#endif /* SMART_LOCK_LOCK_H */
