#include "usart.h"
#include "buffer.h"

static int dma_bout_bytes_to_send = 0;
static Buffer dma_bout;

static void usart_dma_init(void) {
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

    Buffer_init(&dma_bout);
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

    USART2->CR1 = USART_CR1_RE | USART_CR1_TE;
    USART2->BRR = (PCLK1_HZ + (baudrate / 2U)) / baudrate;

    usart_dma_init();

    USART2->CR1 |= USART_CR1_UE;
}

static void dma_init_transfer(const char* bytes, int count) {
    DMA1_Stream6->M0AR = (uint32_t)bytes;
    DMA1_Stream6->NDTR = count;
    DMA1_Stream6->CR |= DMA_SxCR_EN;
    // EN bit in CR register initiates the transfer
    // EN bit is cleared by hardware when the transfer is complete
}

// Send the next contiguous segment from buffer (without copying)
static void dma_init_transfer_from_buffer(void) {
    char* dma_bytes;
    Buffer_get_segment(&dma_bout, &dma_bytes, &dma_bout_bytes_to_send);
    dma_init_transfer(dma_bytes, dma_bout_bytes_to_send);
}

static void usart_send_common(void) {
    if (Buffer_empty(&dma_bout))
        return;

    bool can_init_dma_transfer = (
        (DMA1_Stream6->CR & DMA_SxCR_EN) == 0 &&
        (DMA1->HISR & DMA_HISR_TCIF6) == 0
    );

    if (can_init_dma_transfer) {
        dma_init_transfer_from_buffer();
    }
}

// Interrupt: DMA finished send
void DMA1_Stream6_IRQHandler(void) {
    // Read DMA1 interrupts
    uint32_t isr = DMA1->HISR;

    // Transfer Complete Interrupt Flag on stream 6
    if (isr & DMA_HISR_TCIF6) {
        DMA1->HIFCR = DMA_HIFCR_CTCIF6; // Clear TCIF 
        
        // DMA transfer finished, release data from buffer
        Buffer_pop(&dma_bout, dma_bout_bytes_to_send);
        dma_bout_bytes_to_send = 0; 

        if (!Buffer_empty(&dma_bout)) {
            dma_init_transfer_from_buffer();
        }
    }
}

int usart_send_bytes(uint8_t *bytes, int length)
{
    int result = Buffer_push_bytes(&dma_bout, bytes, length);
    usart_send_common();
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