#ifndef ALERT_DETECTION_H
#define ALERT_DETECTION_H

#include <stdint.h>
#include <stdbool.h>

/* TPMS pressure alert thresholds (kPa) - typical passenger tyre range */
#define PRESSURE_LOW_THRESHOLD_KPA    180.0f
#define PRESSURE_HIGH_THRESHOLD_KPA   280.0f

/* Initialise the alert output GPIO (PB0, drives an indicator LED/buzzer) */
void alert_detection_init(void);

/* Pure threshold logic - no hardware access, safe to call from host-side unit tests */
bool alert_is_out_of_range(float pressure_kpa);

/*
 * Evaluates a calibrated pressure reading against the configured thresholds
 * AND drives the hardware alert output / CAN alert frame if out of range.
 * Returns true if the reading is outside the safe range (an alert condition).
 * This check runs inline within each sensor task (not queued), which is what
 * keeps the alert response time under 100ms even while other tasks are busy -
 * see README for the reasoning behind this design choice.
 */
bool alert_check(uint8_t channel, float pressure_kpa);

/* Drives the alert GPIO output based on whether any channel is currently in alert */
void alert_set_output(bool active);

#endif /* ALERT_DETECTION_H */
