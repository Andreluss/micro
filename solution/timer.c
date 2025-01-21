#include "timer.h"
#include <stm32.h>

static void timer_clock_setup(int psc, int arr);

void timer_init_with_pin_output_on_update_event(int psc, int arr) 
{
    // enable TIM2 clock
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    timer_clock_setup(psc, arr);

    // ---- Configure the signal output ----- 
    TIM2->CR2 &= ~TIM_CR2_MMS; // reset MMS bits
    TIM2->CR2 |= TIM_CR2_MMS_1; // MMS=010 -> output update event on TRGO
    // TIM2 will now generate a TRGO signal on each update event

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