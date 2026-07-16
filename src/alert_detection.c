/*
 * alert_detection.c
 *
 * Threshold-based alert detection for TPMS pressure readings.
 *
 * Design note: alert_check() is called directly inside each sensor task
 * immediately after calibration, rather than being queued to a separate
 * "alert task". Queuing would add latency equal to however long the
 * consumer task takes to get scheduled - acceptable for routine data
 * logging, but not for a safety-relevant under/over-pressure warning.
 * Checking inline, synchronously, is what lets this system guarantee
 * <100ms alert response latency regardless of what the CAN-comms task
 * is doing at that moment.
 */

#include "alert_detection.h"
#include "can_comm.h"
#include "stm32f103xb.h"

#define ALERT_GPIO_PIN_MASK   (1UL << 0)  /* PB0 */

void alert_detection_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

    /* PB0: general-purpose output, push-pull, 2MHz -> bits [3:0] of CRL */
    GPIOB->CRL &= ~(0xFUL << 0);
    GPIOB->CRL |=  ((GPIO_CNF_OUTPUT_PP << 2) | GPIO_MODE_OUTPUT_2MHZ) << 0;

    alert_set_output(false);
}

bool alert_is_out_of_range(float pressure_kpa)
{
    return (pressure_kpa < PRESSURE_LOW_THRESHOLD_KPA) ||
           (pressure_kpa > PRESSURE_HIGH_THRESHOLD_KPA);
}

bool alert_check(uint8_t channel, float pressure_kpa)
{
    bool in_alert = alert_is_out_of_range(pressure_kpa);

    if (in_alert)
    {
        alert_set_output(true);
        can_send_alert(channel, pressure_kpa);
    }

    return in_alert;
}

void alert_set_output(bool active)
{
    if (active)
    {
        GPIOB->BSRR = ALERT_GPIO_PIN_MASK;        /* set pin high  */
    }
    else
    {
        GPIOB->BRR  = ALERT_GPIO_PIN_MASK;        /* set pin low   */
    }
}
