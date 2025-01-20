#pragma once
#include "common.h"

void usart_init(int baudrate, int pclk_hz);
// Returns != 0 if send is incomplete
int usart_send_byte(uint8_t byte);
// Returns != 0 if send is incomplete
int usart_send_string(const char *string);