/*
 * startup_stm32f103xb.s
 * Minimal startup code for STM32F103xB (Cortex-M3).
 * Sets up the vector table, copies .data from flash to RAM,
 * zeroes .bss, then calls main().
 */

    .syntax unified
    .cpu cortex-m3
    .thumb

.global Reset_Handler
.global g_pfnVectors

/* Stack top is defined by the linker script */
.word _estack

/* ---------------- Vector Table ---------------- */
    .section .isr_vector,"a",%progbits
    .type g_pfnVectors, %object
g_pfnVectors:
    .word _estack                  /* Initial stack pointer */
    .word Reset_Handler
    .word NMI_Handler
    .word HardFault_Handler
    .word MemManage_Handler
    .word BusFault_Handler
    .word UsageFault_Handler
    .word 0
    .word 0
    .word 0
    .word 0
    .word SVC_Handler
    .word DebugMon_Handler
    .word 0
    .word PendSV_Handler
    .word SysTick_Handler
    /* External interrupts (subset - enough for CAN1 RX0, used by this project) */
    .word 0            /* WWDG            */
    .word 0            /* PVD             */
    .word 0            /* TAMPER          */
    .word 0            /* RTC             */
    .word 0            /* FLASH           */
    .word 0            /* RCC             */
    .word 0            /* EXTI0           */
    .word 0            /* EXTI1           */
    .word 0            /* EXTI2           */
    .word 0            /* EXTI3           */
    .word 0            /* EXTI4           */
    .word 0            /* DMA1_Channel1   */
    .word 0            /* DMA1_Channel2   */
    .word 0            /* DMA1_Channel3   */
    .word 0            /* DMA1_Channel4   */
    .word 0            /* DMA1_Channel5   */
    .word 0            /* DMA1_Channel6   */
    .word 0            /* DMA1_Channel7   */
    .word 0            /* ADC1_2          */
    .word 0            /* USB_HP_CAN1_TX  */
    .word CAN1_RX0_IRQHandler /* USB_LP_CAN1_RX0 */

    .size g_pfnVectors, . - g_pfnVectors

/* ---------------- Reset Handler ---------------- */
    .section .text.Reset_Handler
    .weak Reset_Handler
    .type Reset_Handler, %function
Reset_Handler:
    ldr r0, =_estack
    mov sp, r0

    /* Copy .data section from flash to RAM */
    ldr r0, =_sdata
    ldr r1, =_edata
    ldr r2, =_sidata
    movs r3, #0
    b LoopCopyDataInit

CopyDataInit:
    ldr r4, [r2, r3]
    str r4, [r0, r3]
    adds r3, r3, #4

LoopCopyDataInit:
    adds r4, r0, r3
    cmp r4, r1
    bcc CopyDataInit

    /* Zero-fill .bss section */
    ldr r2, =_sbss
    ldr r4, =_ebss
    movs r3, #0
    b LoopFillZerobss

FillZerobss:
    str r3, [r2]
    adds r2, r2, #4

LoopFillZerobss:
    cmp r2, r4
    bcc FillZerobss

    bl main
    b .
    .size Reset_Handler, . - Reset_Handler

/* ---------------- Default handlers ---------------- */
    .section .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
    b Infinite_Loop
    .size Default_Handler, . - Default_Handler

.macro def_irq_handler handler_name
    .weak \handler_name
    .set \handler_name, Default_Handler
.endm

    def_irq_handler NMI_Handler
    def_irq_handler HardFault_Handler
    def_irq_handler MemManage_Handler
    def_irq_handler BusFault_Handler
    def_irq_handler UsageFault_Handler
    def_irq_handler SVC_Handler
    def_irq_handler DebugMon_Handler
    def_irq_handler PendSV_Handler
    def_irq_handler SysTick_Handler
    def_irq_handler CAN1_RX0_IRQHandler
