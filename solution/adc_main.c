#include <delay.h>
#include <gpio.h>
#include <stm32.h>
#include "adc.h"
#include "usart.h"
#include "timer.h"
#include "led.h"
#include "alaw.h"

// Things to make sure of:
#define SYS_CLK 96000000
#define PCLK_HZ 48000000
#define USART_BAUDRATE 115200

// Helper functions
static int int_to_string(int n, char *str, int len);
static void set_cpu_clock_96MHz(void);

// ADC conversion complete callback
static void on_adc_conversion_complete(uint16_t result) {
    // TODO: convert to signed and compress A-law (precompute 4096 values)
    // uint8_t linear_scaled_result = (result >> 4) - 128;
    if (result > 4095) {
        led_green_off();
    }
    uint8_t scaled_result = ALaw[result];
    // uint8_t scaled_result = result >> 4;
    // uint8_t scaled_result = result;

    usart_send_byte((uint8_t)scaled_result);
}

static void init(void) {
    set_cpu_clock_96MHz();
    led_green_init(); 
    led_green_on();
    usart_init(USART_BAUDRATE, PCLK_HZ); // TODO: ADC 8bit -> 12bit
    adc_init_with_external_trigger_tim2(on_adc_conversion_complete);

    // Display sampling info.
    const int timer_clock = 96000000;
    int psc = 3000-1, arr = 4-1; 
    int freq = timer_clock / (psc + 1) / (arr + 1);
    char num[16]; int_to_string(freq, num, sizeof(num));
    usart_send_string("\n\rSampling at ");
    usart_send_string(num);
    usart_send_string("Hz\n\r");
    Delay(12800000);

    timer_init_with_pin_output_on_update_event(psc, arr);
}

int main() {
    init();

    while (true) {
        __NOP();
    }
}

static void set_cpu_clock_96MHz(void) {
    RCC->CR &= ~(RCC_CR_PLLI2SON | RCC_CR_PLLON |
                RCC_CR_HSEBYP | RCC_CR_HSEON);

    RCC->CR |= RCC_CR_HSEON;
    while(!(RCC->CR & RCC_CR_HSERDY)) {}
    
    int mul = 4; // cpu clock = HSE / M * N / P = 8MHz / 8 * 48 * mul / 2 = 24Mhz * mul
    const int M = 8, N = 48 * mul, P = 2, Q = mul;
    RCC->PLLCFGR = RCC_PLLCFGR_PLLSRC_HSE |
        M | N << 6 | ((P >> 1) - 1) << 16 | Q << 24;

    RCC->CR |= RCC_CR_PLLON;
    while(!(RCC->CR & RCC_CR_PLLRDY)) {}
    
    // Set flash latency to floor(clock_mhz / 30)
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

    // PCLK2 = HCLK / 1                         [f_PCLK2 <= 100Mhz]
    reg = RCC->CFGR;
    reg &= ~RCC_CFGR_PPRE2;
    reg |= RCC_CFGR_PPRE2_DIV1;
    RCC->CFGR = reg;

    // PLL selected as system clock
    reg = RCC->CFGR;
    reg &= ~RCC_CFGR_SW;
    reg |= RCC_CFGR_SW_PLL;
    RCC->CFGR = reg;

    while(!((RCC->CFGR & RCC_CFGR_SWS) == RCC_CFGR_SWS_PLL)) {}
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
