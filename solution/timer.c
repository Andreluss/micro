#include "timer.h"

static void (*timer_on_tick)(void);

static void timer_clock_setup(int psc, int arr);
static void timer_enable_interrupt(void);

// Timer runs at 16MHz, the frequency will be 16_000_000 / (psc + 1) / (arr + 1)
void timer_init(int psc, int arr, void (*on_timer_tick_)())
{
    timer_on_tick = on_timer_tick_;

    // we're going to use TIM2:
    // - connected to APB1, 
    // - on startup clocked at f_PCLK1 = 16MHz (if PPRE1 = 1)
    // 16Mhz = 16_000_000Hz = 2000 * 8000Hz

    // enable TIM2 clock
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    timer_clock_setup(psc, arr);

    timer_enable_interrupt();

    TIM2->CR1 |= TIM_CR1_CEN; // start the timer
}

void TIM2_IRQHandler(void)
{
    uint32_t it_status = TIM2->SR; // & TIM2->DIER;

    if (it_status & TIM_SR_UIF) {
        // clear interrupt flag
        TIM2->SR &= ~TIM_SR_UIF; 
        if (timer_on_tick)
            timer_on_tick();
    }
}

void timer_init_with_pin_output_on_update_event(int psc, int arr) 
{
    // enable TIM2 clock
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    timer_clock_setup(psc, arr);

    // ---- Configure the signal output ----- 
    TIM2->CR2 &= ~TIM_CR2_MMS; // reset MMS bits
    TIM2->CR2 |= TIM_CR2_MMS_1; // MMS=010 -> output update event on TRGO
    // TIM2 will now generate a TRGO signal on each update event
    // ----------------------------------------------

    // timer_enable_interrupt();

    TIM2->CR1 |= TIM_CR1_CEN; // start the timer
}

static void timer_clock_setup(int psc, int arr) {
    TIM2->CR1 = TIM_CR1_URS; // count upwards, only overflow/underflow 
                                 // generates an update interrupt

    TIM2->PSC = psc;
    TIM2->ARR = arr;

    TIM2->EGR = TIM_EGR_UG; // force update (because of URS this will only
                            //        update but not generate an interrupt)
}

static void timer_enable_interrupt(void) {
    TIM2->SR = ~TIM_SR_UIF; // clear interrupt flag at intialization
    TIM2->DIER = TIM_DIER_UIE; // enable update interrupt
    NVIC_EnableIRQ(TIM2_IRQn);
}