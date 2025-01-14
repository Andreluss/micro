#include "common.h"
#include "adc.h"
#include "usart.h"
#include "timer.h"
#include "user_button.h"
#include "led.h"

// Things to make sure of:
#define SYS_CLK 96000000
#define PCLK_HZ 48000000
#define USART_BAUDRATE 115200

// Helper functions
static void blocking_print(const char *str);
__attribute__((unused)) static void blocking_print_int(int i);
static int int_to_string(int n, char *str, int len);

// Callbacks
static void on_adc_conversion_complete(uint16_t result) {
    usart_send_byte((uint8_t)result);
}

#define until(x) while(!(x))
static void set_cpu_clock_96MHz(void) {
    RCC->CR &= ~(RCC_CR_PLLI2SON | RCC_CR_PLLON |
                RCC_CR_HSEBYP | RCC_CR_HSEON);

    RCC->CR |= RCC_CR_HSEON;
    until(RCC->CR & RCC_CR_HSERDY) {}
    
    int mul = 4; // cpu clock = HSE / M * N / P = 8MHz / 8 * 48 * mul / 2 = 24Mhz * mul
    const int M = 8, N = 48 * mul, P = 2, Q = mul;
    RCC->PLLCFGR = RCC_PLLCFGR_PLLSRC_HSE |
        M | N << 6 | ((P >> 1) - 1) << 16 | Q << 24;

    RCC->CR |= RCC_CR_PLLON;
    until(RCC->CR & RCC_CR_PLLRDY) {}
    
    // latency_ws = floor(clock_mhz / 30); 
    uint32_t latency_ws_flag = 0; {
        if (24 * mul / 30 < 1) {
            latency_ws_flag = FLASH_ACR_LATENCY_0WS;
        } else if (24 * mul / 30 < 2) {
            latency_ws_flag = FLASH_ACR_LATENCY_1WS;
        } else if (24 * mul / 30 < 3) {
            latency_ws_flag = FLASH_ACR_LATENCY_2WS;
        } else if (24 * mul / 30 < 4) {
            latency_ws_flag = FLASH_ACR_LATENCY_3WS;
        } else if (24 * mul / 30 < 5) {
            latency_ws_flag = FLASH_ACR_LATENCY_4WS;
        } else if (24 * mul / 30 < 6) {
            latency_ws_flag = FLASH_ACR_LATENCY_5WS;
        } else if (24 * mul / 30 < 7) {
            latency_ws_flag = FLASH_ACR_LATENCY_6WS;
        } else if (24 * mul / 30 < 8) {
            latency_ws_flag = FLASH_ACR_LATENCY_7WS;
        }
    }
    FLASH->ACR = FLASH_ACR_DCEN | /* data cache */
                 FLASH_ACR_ICEN | /* instr. cache */
                 FLASH_ACR_PRFTEN | /* prefetch */
                 latency_ws_flag;

    // HCLK = SYSCLK / 1
    uint32_t reg;
    reg = RCC->CFGR;
    reg &= ~RCC_CFGR_HPRE;
    reg |= RCC_CFGR_HPRE_DIV1;
    RCC->CFGR = reg;

    // PCLK1 = HCLK / 2 (-> TIM2, ADC, USART2)  [f_PCLK1 <= 50Mhz]
    reg = RCC->CFGR;
    reg &= ~RCC_CFGR_PPRE1;
    reg |= RCC_CFGR_PPRE1_DIV2;
    RCC->CFGR = reg;

    // PCLK2 = HCLK / 1 (IDGAF)                 [f_PCLK2 <= 100Mhz]
    reg = RCC->CFGR;
    reg &= ~RCC_CFGR_PPRE2;
    reg |= RCC_CFGR_PPRE2_DIV1;
    RCC->CFGR = reg;

    // PLL selected as system clock
    reg = RCC->CFGR;
    reg &= ~RCC_CFGR_SW;
    reg |= RCC_CFGR_SW_PLL;
    RCC->CFGR = reg;

    until((RCC->CFGR & RCC_CFGR_SWS) == RCC_CFGR_SWS_PLL) {}

    led_green_init();
    led_green_on();
}

static void init(void) {
    set_cpu_clock_96MHz();
    usart_init(USART_BAUDRATE, PCLK_HZ); // USART will use DMA    
    adc_init_with_external_trigger_tim2(on_adc_conversion_complete);

    // Display sampling info.
    const int timer_clock = 96000000; // experimentally checked, its actually 96MHz
    // int psc = 48000-1, arr = 200-1; // to verify clock 
    int psc = 3000-1, arr = 4-1;
    usart_send_string("\n\rSampling at ");
    char num[16];
    int_to_string(timer_clock / (psc + 1) / (arr + 1), num, sizeof(num));
    usart_send_string(num);
    usart_send_string("Hz\n\r");
    Delay(12800000);

    // timer_init(psc, arr, on_dev_timer_tick);
    timer_init_with_pin_output_on_update_event(psc, arr);
}

int main() {
    init();

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