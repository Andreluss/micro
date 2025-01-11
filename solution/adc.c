#include "adc.h"

static void (*on_adc_conversion_complete)(uint16_t) = 0;
static bool trigger_eoc_interrupt = false;

#define ADC_GPIO GPIOC
#define ADC_PIN 4
#define ADC_CHANNEL 14

void adc_init(bool trigger_eoc_interrupt_)
{
    trigger_eoc_interrupt = trigger_eoc_interrupt_;
    // Microphone is connected to PC4 (GPIOC pin 4)
    // It has additional functionality as ADC channel 14 (STM32 datasheet)

    // Enable GPIOC (the same GPIO as for the user/action buttons)
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    // Enable ADC1 clock
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    // Configure GPIO pin 4 as an analog input
    GPIOainConfigure(GPIOC, ADC_PIN);

    // Configure the ADC
    
    // Set the resolution to 12 bits
    // 8bit
    ADC1->CR1 |= ADC_CR1_RES_1;
    ADC1->CR1 &= ~ADC_CR1_RES_0;
    // 12bit 
    // ADC1->CR1 &= ~ADC_CR1_RES;

    if (trigger_eoc_interrupt) {
        // Enable the End of Conversion interrupt
        ADC1->CR1 |= ADC_CR1_EOCIE;
        NVIC_EnableIRQ(ADC_IRQn);
    }

    // Set the sampling time (n.o. cycles) for channel ADC_CHANNEL
    // Highest sampling time for the most accurate conversion...
    // 000 for 3 cpu cycles
    // 100 for 84 cpu cycles
    ADC1->SMPR1 &= ~ADC_SMPR1_SMP14; // (clears the SMP14 bits)
    ADC1->SMPR1 |= 0b100 << ADC_SMPR1_SMP14_Pos;
    // ADC1->SMPR1 &= ~ADC_SMPR1_SMP14; // (clears the SMP14 bits)
    // 111 for 480 cpu cycles
    // ADC1->SMPR1 |= 0b111 << ADC_SMPR1_SMP14_Pos;

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

static void adc_result_callback(void) {
    // Read the converted data
    uint16_t result = ADC1->DR;  // Get the converted value from the Data Register (DR)

    // Callback
    if (on_adc_conversion_complete) {
        on_adc_conversion_complete(result);
    }
}

void adc_trigger_conversion(void (*on_adc_conversion_complete_)(uint16_t result)/*, int conversion_id*/)
{
    on_adc_conversion_complete = on_adc_conversion_complete_;

    // Start the conversion
    ADC1->CR2 |= ADC_CR2_SWSTART;

    if (!trigger_eoc_interrupt) {
        // Wait for the conversion to complete (non-DMA mode) TODO: use DMA?
        while (!(ADC1->SR & ADC_SR_EOC)) {}

        adc_result_callback();
    }
}

void ADC_IRQHandler(void)
{
    if (ADC1->SR & ADC_SR_EOC) {
        adc_result_callback();
    }
}