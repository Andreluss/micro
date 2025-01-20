#include <gpio.h>
#include <stm32.h>
#include "adc.h"
#include "usart.h"
#include "alaw.h"
#include "timer.h"

#define SYS_CLK 96000000
#define PCLK_HZ 48000000
#define USART_BAUDRATE 115200

const bool USE_ALAW = true;
const enum ADC_Mode ADC_MODE = ADC_MODE_12BIT;
const bool SIGNED = false;

static void on_adc_conversion_complete(uint16_t result) {
    uint16_t scaled_result;
    if (USE_ALAW) {
        scaled_result = ALaw[result]; //12bit -> a-law 8bit 
    } else if (ADC_MODE == ADC_MODE_12BIT) {
        scaled_result = result / 16;
    } else {
        scaled_result = result;
    }

    if (SIGNED) {
        scaled_result ^= 0x80; // unsigned -> signed
    }
    usart_send_byte(scaled_result);
}

static void set_cpu_clock_96MHz(void);

static void init(void) {
    set_cpu_clock_96MHz();
    usart_init(USART_BAUDRATE, PCLK_HZ);
    adc_init_with_external_trigger_tim2(on_adc_conversion_complete, ADC_MODE);

    int psc = 3000-1, arr = 4-1; 
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
