# Reference 
[file:///opt/arm/stm32/doc/RM0383_rev2.pdf]
[file:///opt/arm/stm32/doc/STM32F411xC_411xE_data_rev7.pdf]

-- Jak uruchomić (ofc po `make` na studentsie) -----
scp $STUDENTS:~/dev/micro/solution/adc.bin program.bin \
&& sudo ./qfn4 \
&& sudo minicom

scp $STUDENTS:~/dev/micro/solution/adc.bin program.bin \
&& sudo ./qfn4
----------------------

---- minicom config ---- 
sudo nano /etc/minicom/minirc.dfl

pu port		/dev/ttyACM0
pu baudrate	115200
pu bits		8
pu parity	N
pu stopbits	1
pu rtscts	No
pu minit
pu mreset

---- clocks --------
set f sysclk to f_pll_out

tim is clocked by pclk

TIM2 (f_CK_TIM2) <---> APB1 <----> f_PCLK1 (*2 gdy PPRE1 > 1)

f_PCLK1 = 16Mhz on startup

f_CK_CNTx = f_CK_TIMx / (TIMx->PSC + 1) 

----- audio -------
play -t raw -r 8k -e signed -b 8 -c 1 audio.raw
play -t raw -r 8k -e unsigned -b 8 -c 1 audio.raw

play -r 8k -e a-law -c 1 5.al
play -r 8k -e unsigned -b 8 -c 1 5.raw

----- setup 100mhz cpu ----
Ustawiamy fSYSCLK = fPLL OUT
reg = RCC->CFGR;
reg &= ~RCC_CFGR_SW;
reg |= RCC_CFGR_SW_PLL;
RCC->CFGR = reg;
I To chwilę trwa, czekamy na spełnienie warunku
(RCC->CFGR & RCC_CFGR_SWS) == RCC_CFGR_SWS_PLL
I Ale oczywiście nie czekamy w nieskończoność... 
w4_clocks.pdf / 23


----------------------------

# Todo: 
- refactor: 
-- 96 mghz
-- param 
-- cleanup 
-- unused f
-- test
-- leave hardcoded
-- buffer signal change

# Pytania: 



# Project 
- jak przetestowac, czy dobrze podpiety
- gdzie szukac informacji o zlaczach na plytce CN10
- ", ustawiając bit SWSTART" - tego nie robimy? 
- kodowanie ze znakiem?? 

## Plan 
- podpiecie kabelkow
    - VDD <-> AVDD (pin 8 CN10)
    - OUT <-> wejscie ADC
    - GND <-> AGND (pin 32 CN10)
- uruchomic UART na 115200 b/s BRR + minicom
- zwieksz clock CPU [ na ile? do 100Mhz ]
- odczytywanie sygnalu z ukladu:
    - ustaw ADC: 
        - rozdzielczosc 12 bitow 
        - ustaw wejscia - 1 wejscie, kanal 14, zlacze PC4 [ jak to ustawic, czy PC4 to GPIOC pin 4?] 
        - ustaw # pomiarow = 1
        - wlacz przetwornik ADC1->CR2 |= ADON
        - przetwarzanie ADC: 
            - start <- trigger licznikiem o czest. z PWM 8kHz narastajacy albo opadajace zbocze wyzw pomiar 
                       [ jak ustawic te liczniki - TIM2 + ustawic w ADC1 wyzwalania z zewnatrz]
                        `Dla liczników TIM2 do TIM5 NVIC dostarcza przerwania o numerach TIMx IRQn, gdzie x = 2, 3, 4, 5`
            - koniec -> trigeruje przerwanie `void ADC_IRQHandler() { if (ADC1->SR & ADC_SR_EOC) ADC1->D1 ... `
- przesylanie odczytanego 0...4095 sygnalu przez UART na komputer:
    - w obsłudze przerwania ADC push do wewnetrznej kolejki
    - przerwania mozliwosci zapisu od DMA/UART wyciagaja elementy z kolejki i przesylaja na komputer
- kodowanie A-law na komputerze i zapis do pliku 
- zapisywanie sygnalu w raw


-------------------------- backup ----------------------------
static void init(void) {
    set_cpu_clock_96MHz();
    led_green_init(); 
    led_green_on();
    usart_init(USART_BAUDRATE, PCLK_HZ);
    adc_init_with_external_trigger_tim2(on_adc_conversion_complete);

    // Display sampling info.
    int psc = 3000-1, arr = 4-1; 
    
    const int timer_clock = 96000000;
    int freq = timer_clock / (psc + 1) / (arr + 1);
    char num[16]; int_to_string(freq, num, sizeof(num));
    usart_send_string("\n\rSampling at ");
    usart_send_string(num);
    usart_send_string("Hz\n\r");
    Delay(12800000);

    timer_init_with_pin_output_on_update_event(psc, arr);
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


void adc_trigger_single_conversion(void (*on_adc_conversion_complete_)(uint16_t result)/*, int conversion_id*/)
{
    on_adc_conversion_complete = on_adc_conversion_complete_;

    // Start the conversion
    ADC1->CR2 |= ADC_CR2_SWSTART;

    if (!trigger_eoc_interrupt) {
        // Wait for the conversion to complete (non-DMA mode) TODO: use DMA?
        while (!(ADC1->SR & ADC_SR_EOC)) {}

        adc_result_callback();
    }
}