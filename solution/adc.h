#include "common.h"

void adc_init(int channel);
void adc_trigger_conversion(void (*on_adc_conversion_complete)());