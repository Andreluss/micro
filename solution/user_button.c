#include "user_button.h"

#define USER_BTN_GPIO GPIOC
#define USER_BTN_PIN  13

#define ACTION_BTN_GPIO GPIOB
#define ACTION_BTN_PIN 10

static void (*on_user_button_pressed)();

void user_button_set_on_pressed(void (*on_user_button_pressed_)())
{
    on_user_button_pressed = on_user_button_pressed_;
}

static void user_button_invoke_on_pressed()
{
    if (on_user_button_pressed) {
        on_user_button_pressed();
    }
}

void user_button_init(void (*on_user_button_pressed_)())
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN; // enable GPIOs of the button
    __NOP(); __NOP();

    user_button_set_on_pressed(on_user_button_pressed_);

    // configure button's GPIO pin as input pin triggering interrupts
    GPIOinConfigure(
        USER_BTN_GPIO, 
        USER_BTN_PIN,
        GPIO_PuPd_UP,
        EXTI_Mode_Interrupt,
        EXTI_Trigger_Rising_Falling);

    // enable interrupt & init-clean PR register
    EXTI->PR = EXTI_PR_PR10 | EXTI_PR_PR13; // user button sets pin 10 or 13
    NVIC_EnableIRQ(EXTI15_10_IRQn);
}

static bool is_pressed(uint32_t pin_number, GPIO_TypeDef *gpio) {
    return ((gpio->IDR >> pin_number) & 1) == 0;
}

static void exti_handle_button(uint32_t btn_pin, GPIO_TypeDef *gpio, const char* name, bool signal_pressed) {
    if (EXTI->PR & (1 << btn_pin)) {
        EXTI->PR = (1 << btn_pin); // setting the bit clears the interrupt
        user_button_invoke_on_pressed();
        // if (is_pressed(btn_pin, gpio) == signal_pressed) {
        //     user_button_invoke_on_pressed();
        // }
    }
}

void EXTI15_10_IRQHandler(void) {
    exti_handle_button(USER_BTN_PIN, USER_BTN_GPIO, "USER", true);
}
