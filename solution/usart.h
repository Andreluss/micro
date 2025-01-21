#pragma once
#include <stdint.h>

void usart_init(int baudrate, int pclk_hz);
// Returns != 0 if send is incomplete
int usart_send_byte(uint8_t byte);