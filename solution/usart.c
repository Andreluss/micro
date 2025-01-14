#include "usart.h"
#include "buffer.h"

static Buffer dma_bout[2];
static int _current_buffer_idx = 0;
static Buffer* active_buffer() { return &dma_bout[_current_buffer_idx]; }
static void switch_buffer() { _current_buffer_idx ^= 1; Buffer_clear(active_buffer()); }

static void usart_dma_init(void) {
    USART2->CR3 = USART_CR3_DMAT | USART_CR3_DMAR;

    // Transmitter DMA (stream 6, channel 4) -> interrupt DMA1_Stream6_IRQn
    DMA1_Stream6->CR = 4U << 25 |
                        DMA_SxCR_PL_1 |
                        DMA_SxCR_MINC |
                        DMA_SxCR_DIR_0 |
                        DMA_SxCR_TCIE;
    // Peripheral address (usart data register)
    DMA1_Stream6->PAR = (uint32_t)&USART2->DR;

    // Receiver DMA (stream 5, channel 4) -> interrupt DMA1_Stream5_IRQn
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

    Buffer_init(&dma_bout[0]);
    Buffer_init(&dma_bout[1]);
}

void usart_init(int baudrate, int pclk1_hz)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_DMA1EN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
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

    USART2->CR1 = USART_CR1_RE | USART_CR1_TE;
    USART2->BRR = (pclk1_hz + (baudrate / 2U)) / baudrate;

    usart_dma_init();

    USART2->CR1 |= USART_CR1_UE;
}

static void dma_init_transfer(const char* bytes, int count) {
    DMA1_Stream6->M0AR = (uint32_t)bytes;
    DMA1_Stream6->NDTR = count;
    DMA1_Stream6->CR |= DMA_SxCR_EN;
}

static void dma_send_buffer(Buffer* buffer) {
    char* dma_bytes;
    int dma_bout_bytes_to_send;
    Buffer_get_segment(active_buffer(), &dma_bytes, &dma_bout_bytes_to_send);
    dma_init_transfer(dma_bytes, dma_bout_bytes_to_send);
}

// Interrupt: DMA finished send
void DMA1_Stream6_IRQHandler(void) {
    uint32_t isr = DMA1->HISR;

    // Transfer Complete Interrupt Flag on stream 6
    if (isr & DMA_HISR_TCIF6) {
        DMA1->HIFCR = DMA_HIFCR_CTCIF6; // Clear TCIF 
        
        if (Buffer_size(active_buffer()) > BUFF_SIZE / 2) {
            dma_send_buffer(active_buffer());
            switch_buffer();
        }
    }
}

int usart_send_bytes(uint8_t *bytes, int length)
{
    int result = Buffer_push_bytes(active_buffer(), bytes, length);
    
    bool can_init_dma_transfer = (
        (Buffer_size(active_buffer()) > BUFF_SIZE / 2) && 
        (DMA1_Stream6->CR & DMA_SxCR_EN) == 0 &&
        (DMA1->HISR & DMA_HISR_TCIF6) == 0
    );

    if (can_init_dma_transfer) {
        dma_send_buffer(active_buffer());
        switch_buffer();
    }

    return result;
}

int usart_send_string(const char *string)
{
    int len = strlen(string);
    return usart_send_bytes((uint8_t*)string, len);
}

int usart_send_byte(uint8_t byte)
{
    return usart_send_bytes(&byte, 1);
}