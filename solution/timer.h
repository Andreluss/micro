#pragma once

void timer_init(int psc, int arr, void (*on_timer_tick)());
// Note: this function requires that ADC using the timer is already initialized 
void timer_init_with_pin_output_on_update_event(int psc, int arr);