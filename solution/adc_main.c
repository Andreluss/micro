#include "common.h"
#include "adc.h"
#include "usart.h"
#include "timer.h"
#include "user_button.h"
#include "led.h"

static void blocking_print(const char *str);
__attribute__((unused)) static void blocking_print_int(int i);

static void on_adc_conversion_complete(uint16_t result) {
    // Odczytaj wynik konwersji
    // uint16_t result = ADC1->DR;
    // Przetwórz wynik konwersji
    // ...
}

static void on_timer_tick() {
    // trigger ADC conversion ...
}

static void on_user_button_pressed() {
    // ...
    led_green_off();
    if (usart_send_string("User button pressed!\n\r")) {
        led_green_on();
    }
    adc_trigger_conversion(on_adc_conversion_complete);
}

static void set_cpu_clock_100Mhz() {
    // ... 
}

__attribute__((unused)) static void init(void) {
    set_cpu_clock_100Mhz();

    const int usart_baudrate = 115200; // todo: update the baudrate in minicom config too
    usart_init(usart_baudrate); // USART will use DMA

    const int adc_channel = 14; // idk why
    adc_init(adc_channel);

    const int sampling_frequency_hz = 8000;
    timer_init(sampling_frequency_hz, on_timer_tick);
}

static void init_dev(void) {
    const int usart_baudrate = 115200; // todo: update the baudrate in minicom config too
    usart_init(usart_baudrate); // USART will use DMA

    user_button_init(on_user_button_pressed);
}

int main() {
    init_dev();

    blocking_print("Hello, world!\n\r");
    
    led_green_init();
    led_green_on();

    while (true) {
        __NOP();
    }
}

void blocking_print(const char *str) {
    while (*str) {
        while (!(USART2->SR & USART_SR_TXE));
        USART2->DR = *str;
        str++;
    }
}

void blocking_print_int(int i) {
    char buff[16];
    buff[15] = 0;
    int pos = 14;
    if (i == 0) {
        buff[pos--] = '0';
    } else {
        while (i) {
            buff[pos--] = '0' + (i % 10);
            i /= 10;
        }
    }
    blocking_print(buff + pos + 1);
}