#include "usart.h"

#define HSI_HZ 16000000U // TODO
#define PCLK1_HZ HSI_HZ

static void usart_dma_init() {
    // sending and receiving via DMA
    USART2->CR3 = USART_CR3_DMAT | USART_CR3_DMAR;

    // Transmitter DMA (stream 6, channel 4) will trigger interrupt DMA1_Stream6_IRQn
    DMA1_Stream6->CR = 4U << 25 |
                        DMA_SxCR_PL_1 |
                        DMA_SxCR_MINC |
                        DMA_SxCR_DIR_0 |
                        DMA_SxCR_TCIE;
    // Peripheral address (usart data register)
    DMA1_Stream6->PAR = (uint32_t)&USART2->DR;

    // Receiver DMA (stream 5, channel 4) will trigger interrupt DMA1_Stream5_IRQn
    DMA1_Stream5->CR = 4U << 25 |
                        DMA_SxCR_PL_1 |
                        DMA_SxCR_MINC |
                        DMA_SxCR_TCIE;
    // Peripheral address 
    DMA1_Stream5->PAR = (uint32_t)&USART2->DR;

    // Enable DMA interrupts
    DMA1->HIFCR = DMA_HIFCR_CTCIF6 |
                  DMA_HIFCR_CTCIF5;
    NVIC_EnableIRQ(DMA1_Stream6_IRQn);
    NVIC_EnableIRQ(DMA1_Stream5_IRQn);
}

void usart_init(int baudrate)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_DMA1EN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; // TODO
    __NOP(); __NOP();

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

    USART2->CR1 = USART_CR1_RE | USART_CR1_TE | USART_CR1_UE;
    USART2->BRR = (PCLK1_HZ + (baudrate / 2U)) / baudrate;

    usart_dma_init();

    USART2->CR1 |= USART_CR1_UE;
}

void usart_send_byte(uint8_t byte)
{
}

void usart_send_bytes(uint8_t *bytes, int length)
{
}

void usart_send_string(const char *string)
{
}
