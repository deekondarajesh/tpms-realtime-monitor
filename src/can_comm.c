/*
 * can_comm.c
 *
 * bxCAN (CAN1) driver for the STM32F103, configured for 500 kbit/s.
 *
 * Bit-timing derivation (STM32F103 bxCAN, APB1 = 36 MHz):
 *   CAN bit rate = APB1_clk / (Prescaler * (1 + BS1_quanta + BS2_quanta))
 *   500,000      = 36,000,000 / (4 * (1 + 13 + 4))
 *               = 36,000,000 / (4 * 18)
 *               = 36,000,000 / 72
 *               = 500,000  <- exact match, no rounding error
 *
 * Sample point = (1 + BS1) / (1 + BS1 + BS2) = 14/18 = 77.8%, within the
 * 75-87.5% range recommended by the CAN specification for 500 kbit/s.
 *
 * Two CAN identifier ranges are used so that alert frames win bus
 * arbitration over routine data frames (lower CAN ID = higher priority
 * in CSMA/CA arbitration):
 *   0x100 + channel  -> alert frames      (highest priority)
 *   0x200 + channel  -> periodic sensor data
 */

#include "can_comm.h"
#include "stm32f103xb.h"

#define CAN_BTR_PRESCALER   (4U - 1U)   /* BRP field = prescaler - 1 */
#define CAN_BTR_TS1         (13U - 1U)  /* TS1 field = quanta - 1    */
#define CAN_BTR_TS2         (4U - 1U)   /* TS2 field = quanta - 1    */
#define CAN_BTR_SJW         (1U - 1U)   /* SJW field = quanta - 1    */

static void can_gpio_init(void)
{
    /* PA11 = CAN1_RX (floating input), PA12 = CAN1_TX (AF push-pull) */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

    /* PA11: CNF=01 (floating input), MODE=00 (input) -> bits [15:12] of CRH */
    GPIOA->CRH &= ~(0xFUL << 12);
    GPIOA->CRH |=  (0x4UL << 12);

    /* PA12: CNF=10 (AF push-pull), MODE=11 (output 50MHz) -> bits [19:16] */
    GPIOA->CRH &= ~(0xFUL << 16);
    GPIOA->CRH |=  ((GPIO_CNF_AF_PP << 2) | 0x3UL) << 16;
}

void can_init(void)
{
    can_gpio_init();
    RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;

    /* Enter initialisation mode */
    CAN1->MCR |= CAN_MCR_INRQ;
    while ((CAN1->MSR & CAN_MSR_INAK) == 0)
    {
        /* wait for hardware to acknowledge init mode entry */
    }

    CAN1->MCR |= CAN_MCR_ABOM; /* automatic bus-off management */

    CAN1->BTR = (CAN_BTR_SJW << 24) | (CAN_BTR_TS2 << 20) |
                (CAN_BTR_TS1 << 16) | (CAN_BTR_PRESCALER);

    /* Leave initialisation mode, return to normal operation */
    CAN1->MCR &= ~CAN_MCR_INRQ;
    while ((CAN1->MSR & CAN_MSR_INAK) != 0)
    {
        /* wait for hardware to confirm normal mode */
    }
}

/*
 * Transmits a single CAN data frame: 1 byte channel ID + 4 bytes float
 * pressure value (5 data bytes total), using transmit mailbox 0.
 * Returns false if no mailbox was free (bus busy / not yet serviced).
 */
static bool can_transmit_frame(uint32_t identifier, uint8_t channel, float pressure_kpa)
{
    if ((CAN1->TSR & CAN_TSR_TME0) == 0)
    {
        return false; /* mailbox 0 still pending a previous transmission */
    }

    uint8_t bytes[4];
    /* Portable float->bytes copy - avoids strict-aliasing issues from a raw cast */
    __builtin_memcpy(bytes, &pressure_kpa, sizeof(bytes));

    CAN1->sTxMailBox[0].TDLR = ((uint32_t)channel)      |
                               ((uint32_t)bytes[0] << 8)  |
                               ((uint32_t)bytes[1] << 16) |
                               ((uint32_t)bytes[2] << 24);
    CAN1->sTxMailBox[0].TDHR = (uint32_t)bytes[3];
    CAN1->sTxMailBox[0].TDTR = 5U; /* DLC = 5 data bytes */

    /* Standard (11-bit) identifier goes in bits [31:21] of TIR */
    CAN1->sTxMailBox[0].TIR = (identifier << 21) | CAN_TIR_TXRQ;

    return true;
}

bool can_send_sensor_data(uint8_t channel, float pressure_kpa)
{
    return can_transmit_frame(CAN_ID_SENSOR_DATA_BASE + channel, channel, pressure_kpa);
}

bool can_send_alert(uint8_t channel, float pressure_kpa)
{
    return can_transmit_frame(CAN_ID_ALERT_BASE + channel, channel, pressure_kpa);
}
