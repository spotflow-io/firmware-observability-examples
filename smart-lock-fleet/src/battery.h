#ifndef SMART_LOCK_BATTERY_H
#define SMART_LOCK_BATTERY_H

/**
 * @brief Start the battery monitor thread.
 *
 * The thread registers the battery_level_percent metric and reports a
 * simulated battery reading every 5 minutes. Call this once from main()
 * after the network is up.
 *
 * The metric registration happens inside the thread (same pattern as the
 * Spotflow SDK temperature example), so this function returns immediately
 * and the thread runs independently.
 */
void init_battery_monitor(void);

#endif /* SMART_LOCK_BATTERY_H */
