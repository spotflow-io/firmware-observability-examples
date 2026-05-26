#ifndef ESP32_INDUSTRIAL_SENSOR_NODE_H
#define ESP32_INDUSTRIAL_SENSOR_NODE_H

int sensor_node_init(void);

void sensor_node_step(void);

void sensor_node_trigger_repro_crash(void);

#endif /* ESP32_INDUSTRIAL_SENSOR_NODE_H */
