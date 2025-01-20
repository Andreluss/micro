#pragma once
#include "common.h"

enum ADC_Mode {
    ADC_MODE_8BIT = 0,
    ADC_MODE_12BIT = 1
};

void adc_init_with_external_trigger_tim2(void (*on_adc_conversion_complete_)(uint16_t result), enum ADC_Mode mode);