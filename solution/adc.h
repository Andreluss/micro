#pragma once
#include "common.h"

void adc_init_with_external_trigger_tim2(void (*on_adc_conversion_complete_)(uint16_t result));