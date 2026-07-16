/*
 * stm32f103xb.h
 *
 * Minimal CMSIS-style peripheral register definitions for the
 * STM32F103xB (Cortex-M3, e.g. STM32F103C8T6 "Blue Pill").
 *
 * Only the peripherals actually used by this project are defined:
 * RCC (clocks), GPIO (A/B), ADC1, bxCAN (CAN1), NVIC/SysTick core
 * peripherals. Register offsets match ST's RM0008 reference manual.
 *
 * This is a deliberately trimmed-down device header (not the full
 * vendor CMSIS pack) so the register map is easy to read and audit.
 */

#ifndef STM32F103XB_H
#define STM32F103XB_H

#include <stdint.h>

#define __IO volatile
#define __I  volatile const
#define __O  volatile

/* ---------------- Core peripheral base addresses ---------------- */
#define PERIPH_BASE           0x40000000UL
#define APB1PERIPH_BASE       (PERIPH_BASE)
#define APB2PERIPH_BASE       (PERIPH_BASE + 0x00010000UL)
#define AHBPERIPH_BASE        (PERIPH_BASE + 0x00020000UL)

/* ---------------- RCC (Reset and Clock Control) ---------------- */
typedef struct
{
    __IO uint32_t CR;        /* Clock control register            */
    __IO uint32_t CFGR;      /* Clock configuration register       */
    __IO uint32_t CIR;       /* Clock interrupt register            */
    __IO uint32_t APB2RSTR;
    __IO uint32_t APB1RSTR;
    __IO uint32_t AHBENR;
    __IO uint32_t APB2ENR;   /* APB2 peripheral clock enable        */
    __IO uint32_t APB1ENR;   /* APB1 peripheral clock enable        */
} RCC_TypeDef;

#define RCC_BASE              (AHBPERIPH_BASE + 0x1000UL)
#define RCC                   ((RCC_TypeDef *)RCC_BASE)

#define RCC_CR_HSEON          (1UL << 16)
#define RCC_CR_HSERDY         (1UL << 17)
#define RCC_CR_PLLON          (1UL << 24)
#define RCC_CR_PLLRDY         (1UL << 25)

#define RCC_CFGR_SW_PLL       (2UL << 0)
#define RCC_CFGR_SWS_PLL      (2UL << 2)
#define RCC_CFGR_SWS_Msk      (3UL << 2)
#define RCC_CFGR_PLLSRC_HSE   (1UL << 16)
#define RCC_CFGR_PLLMULL9     (7UL << 18)   /* 8MHz HSE x9 = 72MHz  */
#define RCC_CFGR_PPRE1_DIV2   (4UL << 8)    /* APB1 max 36MHz       */

#define RCC_APB2ENR_IOPAEN    (1UL << 2)
#define RCC_APB2ENR_IOPBEN    (1UL << 3)
#define RCC_APB2ENR_ADC1EN    (1UL << 9)
#define RCC_APB1ENR_CAN1EN    (1UL << 25)

/* ---------------- GPIO ---------------- */
typedef struct
{
    __IO uint32_t CRL;   /* Config low  (pins 0-7)  */
    __IO uint32_t CRH;   /* Config high (pins 8-15) */
    __IO uint32_t IDR;   /* Input data              */
    __IO uint32_t ODR;   /* Output data             */
    __IO uint32_t BSRR;  /* Bit set/reset           */
    __IO uint32_t BRR;   /* Bit reset               */
    __IO uint32_t LCKR;  /* Lock                    */
} GPIO_TypeDef;

#define GPIOA_BASE            (APB2PERIPH_BASE + 0x0800UL)
#define GPIOB_BASE            (APB2PERIPH_BASE + 0x0C00UL)
#define GPIOA                 ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB                  ((GPIO_TypeDef *)GPIOB_BASE)

/* CRL/CRH mode+cnf: 4 bits per pin, MODE[1:0] + CNF[1:0] */
#define GPIO_MODE_OUTPUT_2MHZ  0x2UL
#define GPIO_CNF_OUTPUT_PP     0x0UL
#define GPIO_CNF_INPUT_ANALOG  0x0UL
#define GPIO_CNF_AF_PP         0x2UL /* alternate function push-pull */
#define GPIO_MODE_OUTPUT_10MHZ 0x1UL

/* ---------------- ADC1 ---------------- */
typedef struct
{
    __IO uint32_t SR;
    __IO uint32_t CR1;
    __IO uint32_t CR2;
    __IO uint32_t SMPR1;
    __IO uint32_t SMPR2;
    __IO uint32_t JOFR[4];
    __IO uint32_t HTR;
    __IO uint32_t LTR;
    __IO uint32_t SQR1;
    __IO uint32_t SQR2;
    __IO uint32_t SQR3;
    __IO uint32_t JSQR;
    __IO uint32_t JDR[4];
    __IO uint32_t DR;
} ADC_TypeDef;

#define ADC1_BASE             (APB2PERIPH_BASE + 0x2400UL)
#define ADC1                  ((ADC_TypeDef *)ADC1_BASE)

#define ADC_CR2_ADON          (1UL << 0)
#define ADC_CR2_CAL           (1UL << 2)
#define ADC_CR2_CONT           (1UL << 1)
#define ADC_CR2_EXTSEL_SWSTART (7UL << 17)
#define ADC_CR2_EXTTRIG        (1UL << 20)
#define ADC_CR2_SWSTART        (1UL << 22)
#define ADC_SR_EOC             (1UL << 1)

/* ---------------- bxCAN (CAN1) ---------------- */
typedef struct
{
    __IO uint32_t TIR;
    __IO uint32_t TDTR;
    __IO uint32_t TDLR;
    __IO uint32_t TDHR;
} CAN_TxMailBox_TypeDef;

typedef struct
{
    __IO uint32_t RIR;
    __IO uint32_t RDTR;
    __IO uint32_t RDLR;
    __IO uint32_t RDHR;
} CAN_FIFOMailBox_TypeDef;

typedef struct
{
    __IO uint32_t MCR;
    __IO uint32_t MSR;
    __IO uint32_t TSR;
    __IO uint32_t RF0R;
    __IO uint32_t RF1R;
    __IO uint32_t IER;
    __IO uint32_t ESR;
    __IO uint32_t BTR;
    uint32_t      RESERVED0[88];
    CAN_TxMailBox_TypeDef sTxMailBox[3];
    CAN_FIFOMailBox_TypeDef sFIFOMailBox[2];
} CAN_TypeDef;

#define CAN1_BASE             (APB1PERIPH_BASE + 0x6400UL)
#define CAN1                  ((CAN_TypeDef *)CAN1_BASE)

#define CAN_MCR_INRQ          (1UL << 0)
#define CAN_MCR_ABOM          (1UL << 6)
#define CAN_MSR_INAK          (1UL << 0)
#define CAN_TIR_TXRQ          (1UL << 0)
#define CAN_TSR_TME0          (1UL << 26)

/* ---------------- SysTick (core peripheral) ---------------- */
typedef struct
{
    __IO uint32_t CTRL;
    __IO uint32_t LOAD;
    __IO uint32_t VAL;
    __I  uint32_t CALIB;
} SysTick_TypeDef;

#define SysTick_BASE          0xE000E010UL
#define SysTick               ((SysTick_TypeDef *)SysTick_BASE)

#endif /* STM32F103XB_H */
