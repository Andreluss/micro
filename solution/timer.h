#pragma once
#include "common.h"

void timer_init(int frequency_hz, void (*on_timer_tick)());
// Warning: this function requires that ADC using the timer is already initialized 
void timer_init_with_pin_output_on_update_event(int frequency_hz);