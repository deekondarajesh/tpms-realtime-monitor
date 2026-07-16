#ifndef TASK_SCHEDULER_H
#define TASK_SCHEDULER_H

/*
 * Creates the FreeRTOS tasks and queue that make up the TPMS monitoring
 * system, and initialises the ADC peripheral used to sample the four
 * pressure sensor channels. Must be called before vTaskStartScheduler().
 */
void tpms_scheduler_init(void);

#endif /* TASK_SCHEDULER_H */
