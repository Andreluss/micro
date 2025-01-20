#pragma once

// Note: this function requires that ADC using the timer is already initialized 
void timer_init_with_pin_output_on_update_event(int psc, int arr);