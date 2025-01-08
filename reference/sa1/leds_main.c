#include <delay.h>
#include <gpio.h>
#include <stm32.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define RED_LED_GPIO GPIOA
#define GREEN_LED_GPIO GPIOA
#define BLUE_LED_GPIO GPIOB
#define GREEN2_LED_GPIO GPIOA

#define RED_LED_PIN 6
#define GREEN_LED_PIN 7
#define BLUE_LED_PIN 0
#define GREEN2_LED_PIN 5

bool red = false, green = false, blue = false;
void RedLEDon(void) {
    RED_LED_GPIO->BSRR = 1 << (RED_LED_PIN + 16);
    red = true;
}
void RedLEDoff(void) {
    RED_LED_GPIO->BSRR = 1 << RED_LED_PIN;
    red = false;
}
void RedLEDtoggle(void) {
    if (red) RedLEDoff(); else RedLEDon();
}

void GreenLEDon(void) {
    GREEN_LED_GPIO->BSRR = 1 << (GREEN_LED_PIN + 16);
    green = true;
}
void GreenLEDoff(void) {
    GREEN_LED_GPIO->BSRR = 1 << GREEN_LED_PIN;
    green = false;
}
void GreenLEDtoggle(void) {
    if (green) GreenLEDoff(); else GreenLEDon();
}

void BlueLEDon(void) {
    BLUE_LED_GPIO->BSRR = 1 << (BLUE_LED_PIN + 16);
    blue = true;
}
void BlueLEDoff(void) {
    BLUE_LED_GPIO->BSRR = 1 << BLUE_LED_PIN;
    blue = false;
}
void BlueLEDtoggle(void) {
    if (blue) BlueLEDoff(); else BlueLEDon();
}

bool green2 = false;
void Green2LEDon(void) {
    GREEN2_LED_GPIO->BSRR = 1 << GREEN2_LED_PIN;
    green2 = true;
}
void Green2LEDoff(void) {
    GREEN2_LED_GPIO->BSRR = 1 << (GREEN2_LED_PIN + 16);
    green2 = false;
}
void Green2LEDtoggle(void) {
    if (green2) Green2LEDoff(); else Green2LEDon();
}

void init_led(void) {
    RedLEDoff();
    GreenLEDoff();
    BlueLEDoff();
    Green2LEDoff();

    GPIOoutConfigure(RED_LED_GPIO,
        RED_LED_PIN,
        GPIO_OType_PP,
        GPIO_Low_Speed,
        GPIO_PuPd_NOPULL);
    
    GPIOoutConfigure(GREEN_LED_GPIO,
        GREEN_LED_PIN,
        GPIO_OType_PP,
        GPIO_Low_Speed,
        GPIO_PuPd_NOPULL);

    GPIOoutConfigure(BLUE_LED_GPIO,
        BLUE_LED_PIN,
        GPIO_OType_PP,
        GPIO_Low_Speed,
        GPIO_PuPd_NOPULL);
    
    GPIOoutConfigure(GREEN2_LED_GPIO,
        GREEN2_LED_PIN,
        GPIO_OType_PP,
        GPIO_Low_Speed,
        GPIO_PuPd_NOPULL);
}

#define USART_StopBits_1 0x0000
#define USART_StopBits_0_5 0x1000
#define USART_StopBits_2 0x2000
#define USART_StopBits_1_5 0x3000
#define USART_FlowControl_None 0x0000
#define USART_FlowControl_RTS USART_CR3_RTSE
#define USART_FlowControl_CTS USART_CR3_CTSE
#define HSI_HZ 16000000U
#define PCLK1_HZ HSI_HZ

#define USART_Mode_Rx_Tx (USART_CR1_RE | USART_CR1_TE)
#define USART_Enable USART_CR1_UE

#define USART_WordLength_8b 0x0000
#define USART_WordLength_9b USART_CR1_M

#define USART_Parity_No 0x0000
#define USART_Parity_Even USART_CR1_PCE
#define USART_Parity_Odd (USART_CR1_PCE | USART_CR1_PS)

void init_usart(void) {
    GPIOafConfigure(GPIOA,
                    2,
                    GPIO_OType_PP,
                    GPIO_Fast_Speed,
                    GPIO_PuPd_NOPULL,
                    GPIO_AF_USART2);
    GPIOafConfigure(GPIOA,
                    3,
                    GPIO_OType_PP,
                    GPIO_Fast_Speed,
                    GPIO_PuPd_UP,
                    GPIO_AF_USART2);

    USART2->CR1 = USART_Mode_Rx_Tx |
                  USART_WordLength_8b |
                  USART_Parity_No;
    USART2->CR2 = USART_StopBits_1;
    USART2->CR3 = USART_FlowControl_None;

    #define BAUD 9600U
    USART2->BRR = (PCLK1_HZ + (BAUD / 2U)) / BAUD;

    USART2->CR1 |= USART_Enable;
}

#define USER_BTN_GPIO GPIOC
#define USER_BTN_PIN  13

#define LEFT_BTN_GPIO GPIOB
#define LEFT_BTN_PIN  3

#define RIGHT_BTN_GPIO GPIOB
#define RIGHT_BTN_PIN  4

#define UP_BTN_GPIO GPIOB
#define UP_BTN_PIN  5

#define DOWN_BTN_GPIO GPIOB
#define DOWN_BTN_PIN  6

#define ACTION_BTN_GPIO GPIOB
#define ACTION_BTN_PIN 10

#define AT_MODE_BTN_GPIO GPIOA
#define AT_MODE_BTN_PIN 0

void init_buttons(void) {
    // GPIOInConfigure()
}

void init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;    
    __NOP();

    init_led();

    init_usart();

    init_buttons();
}

#define BUFF_SIZE 1024
typedef struct Buffer {
    char buff[BUFF_SIZE];
    int count;
    int first;
} Buffer;
void Buffer_init(Buffer* b) {
    b->count = 0;
    b->first = 0;
}
bool Buffer_empty(Buffer* b) {
    return b->count == 0;
}
char Buffer_pop(Buffer* b) {
    char res = b->buff[b->first];
    b->first = (b->first + 1ll) % BUFF_SIZE;
    b->count--;
    return res;
}
void Buffer_push(Buffer* b, char c) {
    int pos = (int)(((long long)b->first + b->count) % BUFF_SIZE);
    b->buff[pos] = c;
    b->count++;
}
void Buffer_push_string(Buffer* b, const char* c) {
    int LIMIT = 1024; 
    while ((LIMIT--) && *c) {
        Buffer_push(b, *c);
        c++;
    }
}
void Buffer_clear(Buffer* b) {
    Buffer_init(b);
}
Buffer bout, bin;

bool try_get_char(char* c) {
    if (USART2->SR & USART_SR_RXNE) {
        *c = USART2->DR;
        return true;
    }
    return false;
}

typedef struct {
    GPIO_TypeDef *gpio;
    uint16_t pin;
    bool last_state;
    bool PRESSED_WHEN;
} ButtonState;

ButtonState userButton = {USER_BTN_GPIO, USER_BTN_PIN, false, 0};
ButtonState leftButton = {LEFT_BTN_GPIO, LEFT_BTN_PIN, false, 0};
ButtonState rightButton = {RIGHT_BTN_GPIO, RIGHT_BTN_PIN, false, 0};
ButtonState upButton = {UP_BTN_GPIO, UP_BTN_PIN, false, 0};
ButtonState downButton = {DOWN_BTN_GPIO, DOWN_BTN_PIN, false, 0};
ButtonState actionButton = {ACTION_BTN_GPIO, ACTION_BTN_PIN, false, 0};
ButtonState atModeButton = {AT_MODE_BTN_GPIO, AT_MODE_BTN_PIN, false, 1};

bool is_button_pressed(ButtonState *button) {
    return ((button->gpio->IDR >> button->pin) & 1) == button->PRESSED_WHEN; 
}

void handle_button_update(ButtonState *button, const char *buttonName) {
    bool current_state = is_button_pressed(button);
    if (button->last_state == current_state)
        return;

    if (current_state == true) {
        Buffer_push_string(&bout, buttonName);
        Buffer_push_string(&bout, " PRESSED\r\n");
    }
    else {
        Buffer_push_string(&bout, buttonName);
        Buffer_push_string(&bout, " RELEASED\r\n");
    }

    button->last_state = current_state;
}

void buttons_update(void) {
    handle_button_update(&userButton, "USER");
    handle_button_update(&leftButton, "LEFT");
    handle_button_update(&rightButton, "RIGHT");
    handle_button_update(&upButton, "UP");
    handle_button_update(&downButton, "DOWN");
    handle_button_update(&actionButton, "ACTION");
    handle_button_update(&atModeButton, "AT MODE");
}

int main() {

    init();
    Buffer_init(&bout);
    Buffer_init(&bin);

    char c; 
    while (true) {
        if (try_get_char(&c)) {
            Buffer_push(&bin, c);
        } 
        // if there might be a command in the buffer
        if (bin.count >= 3) {
            // : LR1, LR0, LRT, LG1, LG0, LGT, LB1,
            //   LB0, LBT, Lg1, Lg0, LgT
            if(strncmp(bin.buff, "LR1", 3) == 0) {
                RedLEDon();
            }
            else if (strncmp(bin.buff, "LR0", 3) == 0) {
                RedLEDoff();
            }
            else if (strncmp(bin.buff, "LRT", 3) == 0) {
                RedLEDtoggle();
            }
            else if(strncmp(bin.buff, "LG1", 3) == 0) {
                GreenLEDon();
            }
            else if (strncmp(bin.buff, "LG0", 3) == 0) {
                GreenLEDoff();
            }
            else if (strncmp(bin.buff, "LGT", 3) == 0) {
                GreenLEDtoggle();
            }
            else if(strncmp(bin.buff, "LB1", 3) == 0) {
                BlueLEDon();
            }
            else if (strncmp(bin.buff, "LB0", 3) == 0) {
                BlueLEDoff();
            }
            else if (strncmp(bin.buff, "LBT", 3) == 0) {
                BlueLEDtoggle();
            }
            else if(strncmp(bin.buff, "Lg1", 3) == 0) {
                Green2LEDon();
            }
            else if (strncmp(bin.buff, "Lg0", 3) == 0) {
                Green2LEDoff();
            }
            else if (strncmp(bin.buff, "LgT", 3) == 0) {
                Green2LEDtoggle();
            }
            
            
            Buffer_clear(&bin);
        }

        buttons_update();
        // try to write 
        if (bout.count > 0 && (USART2->SR & USART_SR_TXE)) {
            c = Buffer_pop(&bout);
            USART2->DR = c;
        }
    }
}