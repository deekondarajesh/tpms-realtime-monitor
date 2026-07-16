/*
 * unit_tests.c
 *
 * Host-side unit tests for the hardware-independent logic in this project:
 * pressure calibration (pressure_calibration.c) and alert threshold
 * evaluation (alert_is_out_of_range() in alert_detection.c).
 *
 * These run natively on a PC (see Makefile 'test' target) rather than on
 * the STM32 target, because they don't touch any peripheral registers -
 * only the pure math/logic. Code that *does* touch hardware (ADC reads,
 * CAN transmission, GPIO) is intentionally kept out of these functions so
 * it can be exercised this way; that separation is deliberate, not
 * incidental, and is worth being able to explain in review.
 */

#include <stdio.h>
#include <math.h>
#include "../src/pressure_calibration.h"
#include "../src/alert_detection.h"

static int tests_run    = 0;
static int tests_failed = 0;

#define EXPECT_TRUE(cond, msg) \
    do { \
        tests_run++; \
        if (!(cond)) { \
            tests_failed++; \
            printf("  FAIL: %s\n", msg); \
        } else { \
            printf("  PASS: %s\n", msg); \
        } \
    } while (0)

static int approx_equal(float a, float b, float tolerance)
{
    return fabsf(a - b) <= tolerance;
}

static void test_calibration_reference_points(void)
{
    printf("test_calibration_reference_points:\n");
    calibration_init();

    /* Channel 0's factory reference points map raw 620 -> 100 kPa,
       raw 3450 -> 300 kPa. Calibration should reproduce those exactly
       (within floating point tolerance) since they ARE the reference points. */
    EXPECT_TRUE(approx_equal(calibration_apply(0, 620), 100.0f, 0.5f),
                "channel 0 low reference point maps to ~100 kPa");
    EXPECT_TRUE(approx_equal(calibration_apply(0, 3450), 300.0f, 0.5f),
                "channel 0 high reference point maps to ~300 kPa");
}

static void test_calibration_accuracy_spec(void)
{
    printf("test_calibration_accuracy_spec:\n");
    calibration_init();

    /* Midpoint check: raw sample halfway between the two reference points
       should land close to the midpoint pressure, within the +/-2% accuracy
       specification (2% of 200 kPa span = 4 kPa tolerance). */
    uint16_t raw_mid = (620 + 3450) / 2;
    float expected_mid_kpa = 200.0f;
    float result = calibration_apply(0, raw_mid);

    EXPECT_TRUE(approx_equal(result, expected_mid_kpa, 4.0f),
                "channel 0 midpoint reading is within +/-2% accuracy spec");
}

static void test_calibration_invalid_channel(void)
{
    printf("test_calibration_invalid_channel:\n");
    calibration_init();

    EXPECT_TRUE(calibration_apply(NUM_SENSOR_CHANNELS, 1000) == 0.0f,
                "out-of-range channel index safely returns 0.0 rather than reading garbage memory");
}

static void test_alert_thresholds(void)
{
    printf("test_alert_thresholds:\n");

    EXPECT_TRUE(alert_is_out_of_range(150.0f) == true,
                "150 kPa (below low threshold) triggers an alert");
    EXPECT_TRUE(alert_is_out_of_range(300.0f) == true,
                "300 kPa (above high threshold) triggers an alert");
    EXPECT_TRUE(alert_is_out_of_range(220.0f) == false,
                "220 kPa (normal operating pressure) does not trigger an alert");
    EXPECT_TRUE(alert_is_out_of_range(PRESSURE_LOW_THRESHOLD_KPA) == false,
                "exactly the low threshold value is still considered in-range");
    EXPECT_TRUE(alert_is_out_of_range(PRESSURE_HIGH_THRESHOLD_KPA) == false,
                "exactly the high threshold value is still considered in-range");
}

static void test_alert_debounce(void)
{
    printf("test_alert_debounce:\n");

    /* Fresh channel: first two consecutive out-of-range readings should NOT
       yet trigger (below ALERT_DEBOUNCE_THRESHOLD of 3) */
    EXPECT_TRUE(alert_debounce_update(0, true) == false,
                "1st consecutive out-of-range reading does not yet trigger");
    EXPECT_TRUE(alert_debounce_update(0, true) == false,
                "2nd consecutive out-of-range reading does not yet trigger");
    EXPECT_TRUE(alert_debounce_update(0, true) == true,
                "3rd consecutive out-of-range reading triggers the alert");

    /* A single good reading should reset the debounce counter */
    EXPECT_TRUE(alert_debounce_update(0, false) == false,
                "an in-range reading resets the debounce count");
    EXPECT_TRUE(alert_debounce_update(0, true) == false,
                "count restarts from 1 after a reset, so this alone does not trigger");

    /* Channels are independent of one another */
    EXPECT_TRUE(alert_debounce_update(1, true) == false,
                "channel 1's debounce count is independent of channel 0's");
}

int main(void)
{
    printf("Running TPMS logic unit tests...\n\n");

    test_calibration_reference_points();
    test_calibration_accuracy_spec();
    test_calibration_invalid_channel();
    test_alert_thresholds();
    test_alert_debounce();

    printf("\n%d/%d tests passed.\n", tests_run - tests_failed, tests_run);
    return (tests_failed == 0) ? 0 : 1;
}
