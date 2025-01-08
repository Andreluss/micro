#include <delay.h>
#include <gpio.h>
#include <stm32.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

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

void init_uart(void) {
    // txd line on PA2
    GPIOafConfigure(GPIOA,
                    2,
                    GPIO_OType_PP,
                    GPIO_Fast_Speed,
                    GPIO_PuPd_NOPULL,
                    GPIO_AF_USART2);
    // rxd line on PA3
    GPIOafConfigure(GPIOA,
                    3,
                    GPIO_OType_PP,
                    GPIO_Fast_Speed,
                    GPIO_PuPd_UP,
                    GPIO_AF_USART2);

    // to chyba powinno pasowac do configa minicom
    #define BAUD 9600U
    USART2->CR1 = USART_CR1_RE | USART_CR1_TE;
    USART2->CR2 = 0;
    USART2->BRR = (PCLK1_HZ + (BAUD / 2U)) / BAUD;

    // !wysylanie i odbieranie za pomoca DMA!
    USART2->CR3 = USART_CR3_DMAT | USART_CR3_DMAR;

    // nadawczy strumien DMA (stream 6, channel 4) do tego bedzie przerwanie DMA1_Stream6)
    DMA1_Stream6->CR = 4U << 25 |
                        DMA_SxCR_PL_1 |
                        DMA_SxCR_MINC |
                        DMA_SxCR_DIR_0 |
                        DMA_SxCR_TCIE;
    // Adres układu peryferyjnego nie zmienia się (?)
    DMA1_Stream6->PAR = (uint32_t)&USART2->DR; // DR = data register?

    // odbiorczy strumien DMA (strumien 5, kanal 4) (nie potrzebny tutaj)
    DMA1_Stream5->CR = 4U << 25 |
                        DMA_SxCR_PL_1 |
                        DMA_SxCR_MINC |
                        DMA_SxCR_TCIE;
    // Adres układu peryferyjnego nie zmienia się
    DMA1_Stream5->PAR = (uint32_t)&USART2->DR;


    // ---- aktywuj przerwania DMA ----
    DMA1->HIFCR = DMA_HIFCR_CTCIF6 |
                  DMA_HIFCR_CTCIF5;
    // wlaczenie przerwan zw. z UARTem
    NVIC_EnableIRQ(DMA1_Stream6_IRQn);
    NVIC_EnableIRQ(DMA1_Stream5_IRQn);

    // Uaktywnij układ peryferyjny (?)
    USART2->CR1 |= USART_CR1_UE;
} 

void init_buttons_with_interrupts(void) {

    GPIOinConfigure(
        USER_BTN_GPIO, 
        USER_BTN_PIN,
        GPIO_PuPd_UP,
        EXTI_Mode_Interrupt,
        EXTI_Trigger_Rising_Falling);

    GPIOinConfigure(
        ACTION_BTN_GPIO, 
        ACTION_BTN_PIN, 
        GPIO_PuPd_UP,
        EXTI_Mode_Interrupt,
        EXTI_Trigger_Rising_Falling);

    GPIOinConfigure(
        UP_BTN_GPIO, 
        UP_BTN_PIN, 
        GPIO_PuPd_UP,
        EXTI_Mode_Interrupt,
        EXTI_Trigger_Rising_Falling);

    GPIOinConfigure(
        RIGHT_BTN_GPIO, 
        RIGHT_BTN_PIN, 
        GPIO_PuPd_UP,
        EXTI_Mode_Interrupt,
        EXTI_Trigger_Rising_Falling);

    GPIOinConfigure(
        DOWN_BTN_GPIO, 
        DOWN_BTN_PIN, 
        GPIO_PuPd_UP,
        EXTI_Mode_Interrupt,
        EXTI_Trigger_Rising_Falling);

    GPIOinConfigure(
        LEFT_BTN_GPIO, 
        LEFT_BTN_PIN, 
        GPIO_PuPd_UP,
        EXTI_Mode_Interrupt,
        EXTI_Trigger_Rising_Falling);

    GPIOinConfigure(
        AT_MODE_BTN_GPIO, 
        AT_MODE_BTN_PIN, 
        GPIO_PuPd_DOWN, // to bez UP 
        EXTI_Mode_Interrupt,
        EXTI_Trigger_Rising_Falling);

    // wlacz przerwania zw. z guzikami
    // pr wyczyscic 
    EXTI->PR = EXTI_PR_PR0 | EXTI_PR_PR3 | EXTI_PR_PR4 | EXTI_PR_PR5 | EXTI_PR_PR6 | EXTI_PR_PR10 | EXTI_PR_PR13;
    NVIC_EnableIRQ(EXTI15_10_IRQn);
    NVIC_EnableIRQ(EXTI9_5_IRQn);
    NVIC_EnableIRQ(EXTI0_IRQn);
    NVIC_EnableIRQ(EXTI3_IRQn);
    NVIC_EnableIRQ(EXTI4_IRQn);
}

Buffer bout;
void init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN
                 |  RCC_AHB1ENR_DMA1EN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; // do przerwan guzikow
    __NOP();
    __NOP();

    Buffer_init(&bout);

    init_uart();

    init_buttons_with_interrupts();
}


// Start dma transfer:
char dma_send_buff[1024];
int dma_send_buff_size = 0;
void dma_send_transfer_init(void) {
    DMA1_Stream6->M0AR = (uint32_t)dma_send_buff;
    DMA1_Stream6->NDTR = dma_send_buff_size;
    DMA1_Stream6->CR |= DMA_SxCR_EN; 
    // Bit EN w rejestrze CR strumienia
    // - ustawiany programowo w celu zainicjowania transferu
    // - zerowany sprzętowo po zakończeniu transferu
}

// void dma_recv_transfer_init(void) {
//     DMA1_Stream5->M0AR = (uint32_t)buff;
//     DMA1_Stream5->NDTR = 1;
//     DMA1_Stream5->CR |= DMA_SxCR_EN;
// }

void write_str(const char* str) {
    // dodaj str do bufora wiadomosci 
    Buffer_push_string(&bout, str);

    bool can_init_dma_transfer = (
        (DMA1_Stream6->CR & DMA_SxCR_EN) == 0 &&
        (DMA1->HISR & DMA_HISR_TCIF6) == 0
    );

    if (can_init_dma_transfer) {
        /* Jeśli jest coś do wysłania,
        wystartuj kolejną transmisję.  */
        if (bout.count > 0) {
            // kopiuj bufor z wiadomosciami do bufora dma
            memcpy(dma_send_buff, bout.buff, bout.count);
            dma_send_buff_size = bout.count;
            // wyczysc bufor z wiadomosciami
            Buffer_clear(&bout);
            // wystartuj
            dma_send_transfer_init();
        }
    }
    // else nic nie rob, kolejkuj
}

// -----------------------------
// Interrupt: DMA skonczylo send
// -----------------------------
void DMA1_Stream6_IRQHandler() {
    /* Odczytaj zgłoszone przerwania DMA1. */
    uint32_t isr = DMA1->HISR;

    // Transfer Complete Interrupt Flag na streamie 6
    if (isr & DMA_HISR_TCIF6) {
        /* Obsłuż zakończenie transferu
        w strumieniu 6. */
        DMA1->HIFCR = DMA_HIFCR_CTCIF6; // C=wyczysc TCIF 
                                        // kiedy LIFCR zamiast HIFCR ????
        
        // skonczony transfer, clear dma send buffor 
        dma_send_buff_size = 0; 

        /* Jeśli jest coś do wysłania,
        wystartuj kolejną transmisję.  */
        if (bout.count > 0) {
            // kopiuj bufor z wiadomosciami do bufora dma
            memcpy(dma_send_buff, bout.buff, bout.count);
            dma_send_buff_size = bout.count;
            // wyczysc bufor z wiadomosciami
            Buffer_clear(&bout);
            // wystartuj
            dma_send_transfer_init();
        }
    }
}

// -----------------------------
// Interrupt: DMA skonczylo recv
// -----------------------------
// void DMA1_Stream5_IRQHandler() {
//     /* Odczytaj zgłoszone przerwania DMA1. */
//     uint32_t isr = DMA1->HISR;
//     if (isr & DMA_HISR_TCIF5) {
//     /* Obsłuż zakończenie transferu
//     w strumieniu 5. */
//     DMA1->HIFCR = DMA_HIFCR_CTCIF5;
//     ...
//     /* Ponownie uaktywnij odbieranie. */
//     ...
//     }
// }

static bool is_pressed(uint32_t pin_number, GPIO_TypeDef *gpio) {
    return ((gpio->IDR >> pin_number) & 1) == 0;
}

void exti_handle_button(uint32_t btn_pin, GPIO_TypeDef *gpio, const char* name, bool signal_pressed) {
    if (EXTI->PR & (1 << btn_pin)) {
        EXTI->PR = (1 << btn_pin);
        write_str(name);
        if (is_pressed(btn_pin, gpio) == signal_pressed) {
            write_str(" PRESSED\r\n");
        } else {
            write_str(" RELEASED\r\n");
        }
    }
}

void EXTI15_10_IRQHandler(void) {
    exti_handle_button(USER_BTN_PIN, USER_BTN_GPIO, "USER", true);
    exti_handle_button(ACTION_BTN_PIN, ACTION_BTN_GPIO, "ACTION", true);
}

void EXTI9_5_IRQHandler(void) {
    exti_handle_button(UP_BTN_PIN, UP_BTN_GPIO, "UP", true);
    exti_handle_button(DOWN_BTN_PIN, DOWN_BTN_GPIO, "DOWN", true);
}

void EXTI0_IRQHandler(void) {
    exti_handle_button(AT_MODE_BTN_PIN, AT_MODE_BTN_GPIO, "AT", false);
}

void EXTI3_IRQHandler(void) {
    exti_handle_button(LEFT_BTN_PIN, LEFT_BTN_GPIO, "LEFT", true);
}

void EXTI4_IRQHandler(void) {
    exti_handle_button(RIGHT_BTN_PIN, RIGHT_BTN_GPIO, "RIGHT", true);
}

int main() {
    init();

    while (true) {
        __NOP();
    }
}