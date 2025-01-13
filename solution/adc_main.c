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
    // led_green_on();
    char buff[16];
    if (int_to_string(result, buff, 16) == 0) {
        // usart_send_string("ADC result: ");
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

    // 16_000_000 / (2000-1+1) / (1+1) * 2 = 8000Hz
    timer_init(2000-1, 1, on_timer_tick);
}

__attribute__((unused)) static void on_dev_timer_tick() {
    static int counter = 0;
    counter++;
    if (counter % 2 == 1) {
        led_green_off();
        blocking_print_int(counter);
        blocking_print("s\n\r");
    }
    if (counter % 2 == 0) {
        led_green_on();
        blocking_print_int(counter);
        blocking_print("s\n\r");
    }
}

static void init_dev(void) {
    const int usart_baudrate = 115200; // todo: update the baudrate in minicom config too
    usart_init(usart_baudrate); // USART will use DMA

    user_button_init(on_user_button_pressed);

    adc_init_with_external_trigger_tim2(on_adc_conversion_complete);

    int psc = 1000-1;
    int arr = 8-1;
    usart_send_string("\n\rSampling at ");
    char num[16];
    int_to_string(16000000 / (psc + 1) / (arr + 1), num, sizeof(num));
    usart_send_string(num);
    usart_send_string("Hz\n\r");

    Delay(6400000);

    timer_init_with_pin_output_on_update_event(psc, arr);
}

int main() {
    led_green_init();
    led_green_on();
    init_dev();

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
