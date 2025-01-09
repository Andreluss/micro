#include "adc.h"

static void (*on_adc_conversion_complete)();

void adc_init(int channel)
{
    
}

void adc_trigger_conversion(void (*on_adc_conversion_complete_)())
{
    // Włącz konwersję
    ADC1->CR2 |= ADC_CR2_SWSTART;

    on_adc_conversion_complete = on_adc_conversion_complete_;
}