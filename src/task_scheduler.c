/*
 * task_scheduler.c
 *
 * Task architecture for the TPMS Real-Time Sensor Monitoring System.
 *
 * WHY FreeRTOS (not a bare-metal super-loop):
 *   1. Deterministic timing per channel - each of the four sensor channels
 *      must be sampled on a fixed period regardless of how long calibration
 *      math or CAN transmission takes elsewhere in the system. A single
 *      round-robin while(1) loop makes that timing guarantee fragile as
 *      more work gets added; a preemptive scheduler makes it structural.
 *   2. Priority separation for safety-relevant work - alert detection must
 *      never be delayed behind routine data logging. FreeRTOS lets the
 *      CAN-comms task and sensor tasks be scheduled independently by
 *      priority, and vTaskDelay() means a busy task yields the CPU instead
 *      of blocking everything else.
 *   3. Clean decoupling via a queue - acquisition/calibration/alert-check
 *      (fast, per-channel) is separated from CAN transmission (potentially
 *      momentarily blocked waiting for a free mailbox) via xQueueSend /
 *      xQueueReceive, so a slow bus never stalls sensor sampling.
 *
 * Architecture:
 *   4x vSensorChannelTask  - one per wheel, priority 2. Each samples its
 *                            ADC channel every 20ms, calibrates the reading,
 *                            runs the alert check inline (see
 *                            alert_detection.c for why), then pushes the
 *                            result onto a shared queue for transmission.
 *   1x vCanTxTask           - priority 3 (higher than sensor tasks - once
 *                            data is queued, getting it onto the bus
 *                            promptly matters more than starting the next
 *                            sample early). Blocks on the queue and
 *                            transmits each reading as it arrives.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "task_scheduler.h"
#include "pressure_calibration.h"
#include "can_comm.h"
#include "alert_detection.h"
#include "stm32f103xb.h"

#define SENSOR_TASK_STACK_WORDS   192
#define CANTX_TASK_STACK_WORDS    192
#define SENSOR_TASK_PRIORITY      (tskIDLE_PRIORITY + 2)
#define CANTX_TASK_PRIORITY       (tskIDLE_PRIORITY + 3)
#define SENSOR_SAMPLE_PERIOD_MS   20   /* 50Hz per channel */
#define SENSOR_QUEUE_LENGTH       8

static QueueHandle_t xSensorDataQueue = NULL;

/* ---------------- ADC driver (register-level, polled single-conversion) ---------------- */

static void adc_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_ADC1EN;

    /* PA0-PA3 -> analog input mode: CNF=00, MODE=00 (all zero) for each pin's 4-bit field */
    GPIOA->CRL &= ~(0xFFFFUL); /* clears CRL bits for pins 0-3 -> analog input */

    ADC1->CR2 |= ADC_CR2_ADON;
    for (volatile uint32_t i = 0; i < 1000; i++) { /* t_STAB power-up delay */ }

    /* Self-calibration, per RM0008 recommended start-up sequence */
    ADC1->CR2 |= ADC_CR2_CAL;
    while (ADC1->CR2 & ADC_CR2_CAL) { /* wait for calibration to complete */ }
}

static uint16_t adc_read_channel(uint8_t channel)
{
    ADC1->SQR3 = (uint32_t)(channel & 0x1FUL); /* single conversion, this channel only */
    ADC1->CR2 |= ADC_CR2_SWSTART;

    while ((ADC1->SR & ADC_SR_EOC) == 0)
    {
        /* poll for end-of-conversion - acceptable here since each sensor task
           already yields the CPU via vTaskDelay between samples */
    }

    return (uint16_t)(ADC1->DR & 0x0FFFUL); /* 12-bit result */
}

/* ---------------- Tasks ---------------- */

static void vSensorChannelTask(void *pvParameters)
{
    uint8_t channel = (uint8_t)(uintptr_t)pvParameters;
    TickType_t last_wake_time = xTaskGetTickCount();

    for (;;)
    {
        uint16_t raw_sample   = adc_read_channel(channel);
        float    pressure_kpa = calibration_apply(channel, raw_sample);

        /* Inline, synchronous alert check - see alert_detection.c for why
           this isn't done via the queue/CAN-tx task instead. */
        alert_check(channel, pressure_kpa);

        can_sensor_frame_t frame = { .channel = channel, .pressure_kpa = pressure_kpa };
        xQueueSend(xSensorDataQueue, &frame, 0); /* non-blocking: a full queue means
                                                      CAN-tx is behind, don't stall sampling */

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(SENSOR_SAMPLE_PERIOD_MS));
    }
}

static void vCanTxTask(void *pvParameters)
{
    (void)pvParameters;
    can_sensor_frame_t frame;

    for (;;)
    {
        if (xQueueReceive(xSensorDataQueue, &frame, portMAX_DELAY) == pdTRUE)
        {
            can_send_sensor_data(frame.channel, frame.pressure_kpa);
        }
    }
}

void tpms_scheduler_init(void)
{
    adc_init();
    calibration_init();
    can_init();
    alert_detection_init();

    xSensorDataQueue = xQueueCreate(SENSOR_QUEUE_LENGTH, sizeof(can_sensor_frame_t));
    configASSERT(xSensorDataQueue != NULL);

    static const char *task_names[NUM_SENSOR_CHANNELS] =
    {
        "SensorCh0", "SensorCh1", "SensorCh2", "SensorCh3"
    };

    for (uint8_t ch = 0; ch < NUM_SENSOR_CHANNELS; ch++)
    {
        xTaskCreate(
            vSensorChannelTask,
            task_names[ch],
            SENSOR_TASK_STACK_WORDS,
            (void *)(uintptr_t)ch,
            SENSOR_TASK_PRIORITY,
            NULL
        );
    }

    xTaskCreate(
        vCanTxTask,
        "CanTx",
        CANTX_TASK_STACK_WORDS,
        NULL,
        CANTX_TASK_PRIORITY,
        NULL
    );
}
