#pragma once
#include "common.h"

void adc_init(bool trigger_eoc_interrupt);
void adc_init_with_external_trigger_tim2(void (*on_adc_conversion_complete_)(uint16_t result));
void adc_trigger_single_conversion(void (*on_adc_conversion_complete)(uint16_t result));