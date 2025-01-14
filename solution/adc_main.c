#include "common.h"
#include "adc.h"
#include "usart.h"
#include "timer.h"
#include "user_button.h"
#include "led.h"

static void blocking_print(const char *str);
__attribute__((unused)) static void blocking_print_int(int i);
static int int_to_string(int n, char *str, int len);
static bool clock_failed = false;

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

static void set_cpu_clock_higher_broken(void) {
    // Make sure PL and HSE are disabled 
    RCC->CR &= ~(RCC_CR_PLLI2SON | RCC_CR_PLLON | RCC_CR_HSEBYP | RCC_CR_HSEON);
    for (volatile int i = 0; i < 1000; i++); // Small delay before restarting HSE

    // Enable HSE
    RCC->CR |= RCC_CR_HSEON;
    // Wait until HSE is ready (or until timeout)
    int timeout = 10000;
    while (!(RCC->CR & RCC_CR_HSERDY) && timeout--);
    if (timeout <= 0) {
        clock_failed = true;
        led_green_off();
        return;
    }

    // Configure PLL | Conditions: 
    int M = 8;  // 1MHz <= HSE / M <= 2MHz
    int N = 144; // 100 MHz <= fVCO = HSE * N / M <= 432 MHz
    int P = 3;  // 24 MHz <= fPLL_OUT = HSE * N / M / P <= 100 MHz
    int Q = 3;  // fPLL48CK it's better to be 48MHz even if it's not used
    // M = 2, 3, 4, ..., 63
    // N = 50, 51, 52, ..., 432
    // P = 2, 4, 6, 8
    // Q = 2, 3, 4, ..., 15
    // Note: microcontroller's HSE = 8MHz (ST-LINK/V2-1)
    RCC->PLLCFGR = RCC_PLLCFGR_PLLSRC_HSE // HSE as PLL source
        | (M << RCC_PLLCFGR_PLLM_Pos) 
        | (N << RCC_PLLCFGR_PLLN_Pos) 
        | (((P >> 1) - 1) << RCC_PLLCFGR_PLLP_Pos) 
        | (Q << RCC_PLLCFGR_PLLQ_Pos);
    // The PLL_OUT = (HSE / M) * N / P                  = 8MHz * 100 / 2 = 400MHz
    // The USB clock (<=48Mhz) = (HSE / M) * N / Q      = 8MHz * 100 / 2 = 400MHz
    // PL_OUT clock = 8000 / 8 * 144 / 3 = 
    // PLL48 clock  = 8000 / 8 * 1 = 

    RCC->CR |= RCC_CR_PLLON; // Enable PLL
    // Wait until PLL is ready (or until timeout)
    timeout = 10000;
    while (!(RCC->CR & RCC_CR_PLLRDY) && timeout--);
    if (timeout <= 0) {
        clock_failed = true;
        return;
    }

    
    // Change the system clock source to PLL
    uint32_t reg = RCC->CFGR;
    reg &= ~RCC_CFGR_SW;
    reg |= RCC_CFGR_SW_PLL;
    RCC->CFGR = reg;

    timeout = 100000;

    while (((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) /* && timeout-- */) {
            // led_green_off();

        // if (timeout % 2 == 1) {
        //     led_green_off();
        //         for (volatile int i = 0; i < 10000; i++); // Small delay for visibility
        // } else {
        //     led_green_on();
        //         for (volatile int i = 0; i < 10000; i++); // Small delay for visibility
        // }
    }
    
    led_green_off();

    if (timeout <= 0) {
        clock_failed = true;
        return;
    }

    
}

#define until(x) while(!(x))
static void set_cpu_clock_higher(void) {
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
    uint32_t latency_ws_flag = 0;
    {
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

__attribute__((unused)) static void init(void) {
    set_cpu_clock_higher();

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

__attribute__((unused)) static void init_dev(void) {

    const int usart_baudrate = 115200; // todo: update the baudrate in minicom config too
    usart_init(usart_baudrate); // USART will use DMA
    blocking_print("Hello, World!\n\r");

    adc_init(true);

    user_button_init(on_user_button_pressed);

    adc_init_with_external_trigger_tim2(on_adc_conversion_complete);

    // now that 96MHz = SYSCLK  = HCLK -> 
    // PCLK1 = 48MHz = HCLK / 2 (PRE > 1) 
    // -> TIM2CLK = PCLK1 / 2 = 24MHz (instead of 16MHz)
    int timer_clock = 96000000;
    int psc = 48000-1;
    int arr = 200-1;
    usart_send_string("\n\rSampling at ");
    char num[16];
    int_to_string(timer_clock / (psc + 1) / (arr + 1), num, sizeof(num));
    usart_send_string(num);
    usart_send_string("Hz\n\r");

    Delay(6400000);

    timer_init(psc, arr, on_dev_timer_tick);
    // timer_init_with_pin_output_on_update_event(psc, arr);
}

int main() {
    // led_green_init();
    // led_green_on();
    set_cpu_clock_higher();
    // if (clock_failed) {
    //     while (true) {
    //         led_green_off();
    //         Delay(1000000);
    //         led_green_on();
    //         Delay(1000000);
    //     }
    // } else {
    //     led_green_on();
    // }
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
