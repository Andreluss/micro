#pragma once
#include "common.h"

void adc_init(bool trigger_eoc_interrupt);
void adc_trigger_conversion(void (*on_adc_conversion_complete)(uint16_t result));