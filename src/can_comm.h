#ifndef CAN_COMM_H
#define CAN_COMM_H

#include <stdint.h>
#include <stdbool.h>

/* CAN identifiers used on the bus by this project */
#define CAN_ID_SENSOR_DATA_BASE   0x200U   /* 0x200 + channel = periodic data  */
#define CAN_ID_ALERT_BASE         0x100U   /* 0x100 + channel = alert frame    */

typedef struct
{
    uint8_t  channel;
    float    pressure_kpa;
} can_sensor_frame_t;

/* Initialise GPIO (PA11/PA12) and the bxCAN peripheral for 500 kbit/s */
void can_init(void);

/* Transmit a periodic sensor-data frame for the given channel */
bool can_send_sensor_data(uint8_t channel, float pressure_kpa);

/* Transmit a high-priority alert frame for the given channel (lower CAN ID = higher bus priority) */
bool can_send_alert(uint8_t channel, float pressure_kpa);

#endif /* CAN_COMM_H */
