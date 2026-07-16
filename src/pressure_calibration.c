/*
 * pressure_calibration.c
 *
 * Two-point linear calibration for TPMS piezoresistive pressure sensors.
 *
 * Each sensor channel has slightly different ADC-to-pressure characteristics
 * due to manufacturing tolerance in the sense element. Rather than assuming
 * an ideal transfer function, each channel is calibrated at two known
 * reference pressures during production test, and the resulting gain/offset
 * pair is used here to linearly map raw ADC counts to kPa. This is the same
 * two-point calibration approach used on the production TPMS validation
 * platform this project is modelled on, and is what allows the system to
 * meet its +/-2% accuracy specification across the sensor's operating range.
 */

#include "pressure_calibration.h"

static calibration_coeffs_t calibration_table[NUM_SENSOR_CHANNELS];

/*
 * Reference calibration points per channel, as would be captured during
 * production test:
 *   raw_low  -> pressure_low_kPa   (e.g. sensor at atmospheric reference)
 *   raw_high -> pressure_high_kPa  (e.g. sensor at regulated test rig pressure)
 *
 * gain   = (pressure_high - pressure_low) / (raw_high - raw_low)
 * offset = pressure_low - gain * raw_low
 */
typedef struct
{
    uint16_t raw_low;
    uint16_t raw_high;
    float    pressure_low_kpa;
    float    pressure_high_kpa;
} factory_reference_t;

static const factory_reference_t factory_references[NUM_SENSOR_CHANNELS] =
{
    /* channel 0 */ { .raw_low = 620,  .raw_high = 3450, .pressure_low_kpa = 100.0f, .pressure_high_kpa = 300.0f },
    /* channel 1 */ { .raw_low = 615,  .raw_high = 3440, .pressure_low_kpa = 100.0f, .pressure_high_kpa = 300.0f },
    /* channel 2 */ { .raw_low = 625,  .raw_high = 3460, .pressure_low_kpa = 100.0f, .pressure_high_kpa = 300.0f },
    /* channel 3 */ { .raw_low = 618,  .raw_high = 3445, .pressure_low_kpa = 100.0f, .pressure_high_kpa = 300.0f },
};

void calibration_init(void)
{
    for (uint8_t ch = 0; ch < NUM_SENSOR_CHANNELS; ch++)
    {
        const factory_reference_t *ref = &factory_references[ch];
        float raw_span = (float)(ref->raw_high - ref->raw_low);

        calibration_table[ch].gain   = (ref->pressure_high_kpa - ref->pressure_low_kpa) / raw_span;
        calibration_table[ch].offset = ref->pressure_low_kpa - (calibration_table[ch].gain * (float)ref->raw_low);
    }
}

float calibration_apply(uint8_t channel, uint16_t raw_adc_sample)
{
    if (channel >= NUM_SENSOR_CHANNELS)
    {
        return 0.0f;
    }

    const calibration_coeffs_t *coeffs = &calibration_table[channel];
    return (coeffs->gain * (float)raw_adc_sample) + coeffs->offset;
}
