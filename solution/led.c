#include "led.h"

#define GREEN_LED_GPIO GPIOA
#define GREEN_LED_PIN 7

#define GreenLEDon() \
GREEN_LED_GPIO->BSRR = 1 << (GREEN_LED_PIN + 16)
#define GreenLEDoff() \
GREEN_LED_GPIO->BSRR = 1 << GREEN_LED_PIN

static void led_common_init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;
    __NOP();
}

void led_green_init(void)
{
    led_common_init();

    GreenLEDoff();

    GPIOoutConfigure(GREEN_LED_GPIO,
        GREEN_LED_PIN,
        GPIO_OType_PP,
        GPIO_Low_Speed,
        GPIO_PuPd_NOPULL);
}

void led_green_on(void)
{
    GreenLEDon();
}

void led_green_off(void)
{
    GreenLEDoff();
}
