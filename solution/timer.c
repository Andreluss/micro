#include "timer.h"

static void (*timer_on_tick)(void);

void timer_init(int frequency_hz, void (*on_timer_tick_)())
{
    timer_on_tick = on_timer_tick_;

    // we're going to use TIM2:
    // - connected to APB1, 
    // - on startup clocked at f_PCLK1 = 16MHz (if PPRE1 = 1)
    // 16Mhz = 16_000_000Hz = 2000 * 8000Hz

    // the only supported frequency is 8kHz
    if (frequency_hz == 8000) {
        // enable TIM2 clock
        RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

        TIM2->CR1 = TIM_CR1_URS; // count upwards, only overflow/underflow 
                                 // generates an update interrupt

        TIM2->PSC = 2000-1;
        TIM2->ARR = 0;

        TIM2->EGR = TIM_EGR_UG; // force update (because of URS this will only
                                //        update but not generate an interrupt)

        TIM2->SR = ~TIM_SR_UIF; // clear interrupt flag at intialization
        TIM2->DIER = TIM_DIER_UIE; // enable update interrupt
        NVIC_EnableIRQ(TIM2_IRQn);

        TIM3->CR1 |= TIM_CR1_CEN; // start the timer
    }
}

void TIM2_IRQHandler(void)
{
    uint32_t it_status = TIM2->SR & TIM2->DIER;
    TIM2->SR = 0; // clear the interrupt

    if (it_status & TIM_SR_UIF) {
        // update interrupt
        TIM2->SR = ~TIM_SR_UIF; 
        if (timer_on_tick) 
            timer_on_tick();
    }
}