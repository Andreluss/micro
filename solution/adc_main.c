#include "common.h"
#include "adc.h"
#include "usart.h"
#include "timer.h"
#include "user_button.h"
#include "led.h"

static void on_adc_conversion_complete() {
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
    usart_send_string("User button pressed!\n");
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

    led_green_init();
    led_green_on();

    while (true) {
        __NOP();
    }
}