#include "adc.h"
#include <stdbool.h>
#include <stm32.h>
#include <stddef.h>
#include <gpio.h>

static void (*on_adc_conversion_complete)(uint16_t) = NULL;
static bool trigger_eoc_interrupt = false;

#define ADC_GPIO GPIOC
#define ADC_PIN 4
#define ADC_CHANNEL 14

static void adc_init_common(bool trigger_eoc_interrupt_, void (*on_adc_conversion_complete_)(uint16_t result), enum ADC_Mode mode);

void adc_init_with_external_trigger_tim2(void (*on_adc_conversion_complete_)(uint16_t result), enum ADC_Mode mode) 
{
    adc_init_common(true, on_adc_conversion_complete_, mode);

    // Set the external trigger to TIM2 
    ADC1->CR2 &= ~ADC_CR2_EXTSEL_Msk;
    ADC1->CR2 |= ADC_CR2_EXTSEL_1 | ADC_CR2_EXTSEL_2; // EXTSEL: 0110 -> TIM2_TRGO
    // Set the external trigger polarity to rising edge
    ADC1->CR2 &= ~ADC_CR2_EXTEN_Msk;
    ADC1->CR2 |= ADC_CR2_EXTEN_0; // EXTEN: 01 -> rising edge 

    // Enable the ADC
    ADC1->CR2 |= ADC_CR2_ADON; // Turn on the ADC
}

void ADC_IRQHandler(void)
{
    if (ADC1->SR & ADC_SR_EOC) {
        uint16_t result = ADC1->DR;

        if (on_adc_conversion_complete) {
            on_adc_conversion_complete(result);
        }
    }
}

void adc_init_common(bool trigger_eoc_interrupt_, void (*on_adc_conversion_complete_)(uint16_t result), enum ADC_Mode mode)
{
    trigger_eoc_interrupt = trigger_eoc_interrupt_;
    on_adc_conversion_complete = on_adc_conversion_complete_;
    // Microphone is connected to PC4 (GPIOC pin 4)
    // It has additional functionality as ADC channel 14 (STM32 datasheet)

    // Enable GPIOC (the same GPIO as for the user/action buttons)
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    // Enable ADC1 clock
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    // Configure GPIO pin 4 as an analog input
    GPIOainConfigure(GPIOC, ADC_PIN);
    
    // Set the ADC resolution 
    if (mode == ADC_MODE_8BIT) {
        ADC1->CR1 |= ADC_CR1_RES_1;
        ADC1->CR1 &= ~ADC_CR1_RES_0;
    } else if (mode == ADC_MODE_12BIT) {
        ADC1->CR1 &= ~ADC_CR1_RES;
    }

    if (trigger_eoc_interrupt) {
        // Enable the End of Conversion interrupt
        ADC1->CR1 |= ADC_CR1_EOCIE;
        NVIC_EnableIRQ(ADC_IRQn);
    }

    // Set the sampling time (n.o. cycles) for channel ADC_CHANNEL
    // Highest sampling time for the most accurate conversion...
    // 000 for 3 cpu cycles
    // 100 for 84 cpu cycles
    // 111 for 480 cpu cycles
    ADC1->SMPR1 &= ~ADC_SMPR1_SMP14; // (clears the SMP14 bits)
    ADC1->SMPR1 |= 0b100 << ADC_SMPR1_SMP14_Pos;

    // Set the number of conversions to 1
    ADC1->SQR1 &= ~ADC_SQR1_L; // 0000 for 1 conversion
    // Set the 1st conversion channel n.o. to ADC_CHANNEL
    ADC1->SQR3 = ADC_CHANNEL; // this fills the smallest 4 bits,
    // which map exactly to the 1st conversion channel register 

    ADC1->CR2 &= ~ADC_CR2_ALIGN; // left align the 12bit data
    // ADC1->CR2 |= ADC_CR2_ALIGN; // right align the 12bit data
}
