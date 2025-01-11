#include "common.h"
#include "adc.h"
#include "usart.h"
#include "timer.h"
#include "user_button.h"
#include "led.h"

static void blocking_print(const char *str);
__attribute__((unused)) static void blocking_print_int(int i);
static int int_to_string(int n, char *str, int len);

static void on_adc_conversion_complete(uint16_t result) {
    // Odczytaj wynik konwersji
    // uint16_t result = ADC1->DR;
    // Przetwórz wynik konwersji
    // ...
    led_green_on();
    char buff[16];
    if (int_to_string(result, buff, 16) == 0) {
        usart_send_string("ADC result: ");
        usart_send_string(buff);
        usart_send_string("\n\r");
    }
    else {
        usart_send_string("ADC-RES-ERR\n\r");
    }
}

static void on_timer_tick() {
    // trigger ADC conversion ...
}

static void on_user_button_pressed() {
    // ...
    led_green_off();
    usart_send_string("Starting conversion!\n\r");
    adc_trigger_single_conversion(on_adc_conversion_complete);
}

static void set_cpu_clock_100Mhz() {
    // ... 
}

__attribute__((unused)) static void init(void) {
    set_cpu_clock_100Mhz();

    const int usart_baudrate = 115200; // todo: update the baudrate in minicom config too
    usart_init(usart_baudrate); // USART will use DMA

    adc_init(true);

    const int sampling_frequency_hz = 8000;
    timer_init(sampling_frequency_hz, on_timer_tick);
}

static void on_dev_timer_tick() {
    blocking_print("WTF!\n\r");
    // static int counter = 0;
    // counter++;
    // if (counter > 0) {
    //     led_green_off();
    // }
    // if (counter % 16000 == 0) {
    //     led_green_on();
    // }
    // blocking_print("Timer tick\n\r");
}

static void init_dev(void) {
    timer_init(8000, on_dev_timer_tick);

    const int usart_baudrate = 115200; // todo: update the baudrate in minicom config too
    usart_init(usart_baudrate); // USART will use DMA

    // user_button_init(on_user_button_pressed);

    // adc_init(true);
}

int main() {
    led_green_init();
    led_green_on();
    init_dev();

    blocking_print("Hello, world!\n\r");
    

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

int int_to_string(int n, char *buff, int len) {
    buff[len - 1] = 0;
    int pos = len - 2;
    if (n == 0) {
        buff[pos--] = '0';
    } else {
        while (n && pos >= 0) {
            buff[pos--] = '0' + (n % 10);
            n /= 10;
        }
    }

    if (n != 0) {
        return -1;
    }

    // shift the string to the beginning
    int shift = pos + 1;
    for (int i = 0; i < len; i++) {
        if (i + shift < len) {
            buff[i] = buff[i + shift];
        } else {
            buff[i] = 0;
        }
    }

    return 0;
}
