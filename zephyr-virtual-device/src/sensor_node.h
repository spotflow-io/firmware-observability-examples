#ifndef ZEPHYR_VIRTUAL_DEVICE_SENSOR_NODE_H
#define ZEPHYR_VIRTUAL_DEVICE_SENSOR_NODE_H

int  sensor_node_init(void);
void sensor_node_step(void);
void sensor_node_trigger_repro_crash(void);

#endif /* ZEPHYR_VIRTUAL_DEVICE_SENSOR_NODE_H */
