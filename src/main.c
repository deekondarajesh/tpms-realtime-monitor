/*
 * main.c
 * Real-Time TPMS Sensor Monitoring System
 * Target: STM32F103C8T6 (Cortex-M3 @ 72MHz)
 *
 * Configures the system clock to 72MHz via the internal PLL from an 8MHz
 * external crystal (HSE), then hands off to the FreeRTOS scheduler, which
 * runs four sensor-channel tasks and one CAN transmit task (see
 * task_scheduler.c for the task architecture and reasoning).
 */

#include "FreeRTOS.h"
#include "task.h"
#include "task_scheduler.h"
#include "stm32f103xb.h"

/*
 * Configures SYSCLK = 72MHz from an 8MHz HSE crystal via PLL (x9),
 * with APB1 = 36MHz (PLL/2, required since APB1 max is 36MHz) and
 * APB2 = 72MHz (undivided).
 */
static void system_clock_config(void)
{
    /* 1. Start the external 8MHz crystal oscillator (HSE) */
    RCC->CR |= RCC_CR_HSEON;
    while ((RCC->CR & RCC_CR_HSERDY) == 0)
    {
        /* wait for the crystal to stabilise */
    }

    /* 2. Configure APB1 prescaler (/2) before enabling the PLL */
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;

    /* 3. Configure PLL source = HSE, PLL multiplier = x9 (8MHz x 9 = 72MHz) */
    RCC->CFGR |= (RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLMULL9);

    /* 4. Enable the PLL and wait for it to lock */
    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0)
    {
        /* wait for PLL lock */
    }

    /* 5. Switch the system clock source to the PLL */
    RCC->CFGR = (RCC->CFGR & ~(0x3UL)) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL)
    {
        /* wait for the switch to take effect */
    }
}

int main(void)
{
    system_clock_config();

    tpms_scheduler_init();

    /* Hand control to FreeRTOS - this call does not return under normal operation */
    vTaskStartScheduler();

    /* Only reached if there wasn't enough heap to create the idle/timer tasks */
    for (;;)
    {
    }
}

/* FreeRTOS hook - called if pvPortMalloc() cannot satisfy an allocation */
void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    for (;;)
    {
    }
}

/* FreeRTOS hook - called if a task's stack overflows its allocated bounds */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    taskDISABLE_INTERRUPTS();
    for (;;)
    {
    }
}
