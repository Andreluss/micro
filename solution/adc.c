#include "adc.h"

static void (*on_adc_conversion_complete)();

#define ADC_GPIO GPIOC
#define ADC_PIN 4
#define ADC_CHANNEL 14

void adc_init()
{
    // Microphone is connected to PC4 (GPIOC pin 4)
    // It has additional functionality as ADC channel 14 (STM32 datasheet)

    // Enable GPIOC (the same GPIO as for the user/action buttons)
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    // Enable ADC1 clock
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    // Configure GPIO pin 4 as an analog input
    GPIOainConfigure(GPIOC, 4);

    // Configure the ADC
    // Set the resolution to 12 bits
    ADC1->CR1 &= ~ADC_CR1_RES;
    // Set the sample time (n.o. cycles) for channel ADC_CHANNEL
    // ADC1->SMPR1 &= ~(ADC_SMPR1_SMP14_0 | ADC_SMPR1_SMP14_2); 
    // Set the number of conversions to 1
    ADC1->SQR1 &= ~ADC_SQR1_L; // 0000 for 1 conversion
    // Set the 1st conversion channel n.o. to ADC_CHANNEL
    ADC1->SQR3 = ADC_CHANNEL; // this fills the smallest 4 bits,
    // which map exactly to the 1st conversion channel register 

    // ---- ADC Config schema ----
    // ADC->CCR = ...; // for setting adc clock prescaler
    // ADC1->CR1 = ...; // i.a. resolution (6, 8, 10, 12 bits)
    // ADC1->CR2 = ...;
    // ADC1->SMPR1 = ...; // no. of cycles per each 10-18 channel's conversion 
    // ADC1->SMPR2 = ...; // no. of cycles per each 0-9 channel's conversion
    // ADC1->SQR1 = ...; // i.a. no. of conversions
    // ADC1->SQR2 = 0; // result ALIGN
    // ADC1->SQR3 = 4U; /* on channel 4 */

    // Enable the ADC
    ADC1->CR2 |= ADC_CR2_ADON; // Turn on the ADC
}

void adc_trigger_conversion(void (*on_adc_conversion_complete_)())
{
    // Włącz konwersję
    ADC1->CR2 |= ADC_CR2_SWSTART;

    on_adc_conversion_complete = on_adc_conversion_complete_;
}