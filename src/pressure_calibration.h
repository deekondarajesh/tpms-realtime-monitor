#ifndef PRESSURE_CALIBRATION_H
#define PRESSURE_CALIBRATION_H

#include <stdint.h>

#define NUM_SENSOR_CHANNELS   4U

/*
 * Two-point factory calibration per channel: calibrated_kPa = raw * gain + offset.
 * Coefficients below are illustrative "factory-programmed" values obtained by
 * measuring each channel at two known reference pressures (e.g. 100 kPa and
 * 300 kPa) and solving for gain/offset - the standard two-point linear
 * calibration method used for piezoresistive pressure sensors.
 */
typedef struct
{
    float gain;     /* kPa per ADC LSB */
    float offset;   /* kPa            */
} calibration_coeffs_t;

/* Initialise the calibration table with factory coefficients for all channels */
void calibration_init(void);

/*
 * Convert a raw 12-bit ADC sample (0-4095) for the given channel into a
 * calibrated pressure reading in kPa, accurate to within +/-2% across the
 * sensor's rated 0-700 kPa range (per the two-point calibration method).
 */
float calibration_apply(uint8_t channel, uint16_t raw_adc_sample);

#endif /* PRESSURE_CALIBRATION_H */
