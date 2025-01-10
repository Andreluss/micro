#pragma once
#include "common.h"

void timer_init(int frequency_hz, void (*on_timer_tick)());