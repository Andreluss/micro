#pragma once
#include "common.h"

void adc_init();
void adc_trigger_conversion(void (*on_adc_conversion_complete)());