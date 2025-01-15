#include "dma.h"

/* -------------------------------------------------------------------------- */
/*                                  Variables                                 */
/* -------------------------------------------------------------------------- */
static volatile DMA_ERROR error = DMA_ERROR_NO_ERRORS;
static volatile uint8_t transfer_status = 0;

/* -------------------------------------------------------------------------- */
/*                                Declarations                                */
/* -------------------------------------------------------------------------- */
void dma_init(DMA_Stream_TypeDef* dma_stream, IRQn_Type dma_irq, uint32_t periph_address, uint32_t mem_address);
void dmamux_init(void);
void dma_clear_status_flags(void);


/* -------------------------------------------------------------------------- */
/*                              Public Functions                              */
/* -------------------------------------------------------------------------- */
DMA_ERROR DMA_GetError(void) {
    return error;
}

uint8_t DMA_GetTransferStatus(void) {
    return transfer_status;
}

void DMA_ClearTransferStatus(void) {
    transfer_status = 0;
}

void DMA_InitPeriph(uint16_t* memory0_address) {
    dma_init(DMA1_Stream4, DMA1_Stream4_IRQn, (uint32_t)&(ADC1->DR), (uint32_t)memory0_address);
    dmamux_init();
}

void DMA_EnablePeriph(uint16_t* mem_address, const uint16_t mem_size) {
    // check if the size is the multiple of D-Cache line size (32 words)
    // 16bit - half words -> x2 multiplication
    if (mem_size % (__SCB_DCACHE_LINE_SIZE << 1)) {
        error = DMA_ERROR_MEMORY_WRONG_SIZE;
    }

    // cache must be invalidated before reading transferred data
    // second argument in bytes
    SCB_InvalidateDCache_by_Addr(mem_address, mem_size << 1);

    dma_clear_status_flags();
    SET_BIT(DMA1_Stream4->CR, DMA_SxCR_EN);
}

void DMA_DisablePeriph(void) {
    CLEAR_BIT(DMA1_Stream4->CR, DMA_SxCR_EN);
    while(READ_BIT(DMA1_Stream4->CR, DMA_SxCR_EN));
    dma_clear_status_flags();
}


/* -------------------------------------------------------------------------- */
/*                              Private Functions                             */
/* -------------------------------------------------------------------------- */
void dma_init(DMA_Stream_TypeDef* dma_stream, IRQn_Type dma_irq, uint32_t periph_address, uint32_t mem_address) {
    
    if (READ_BIT(dma_stream->CR, DMA_SxCR_EN)) {
        error = DMA_ERROR_ENABLED_BEFORE_INIT;
    }
    
    // Clock
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_DMA1EN);  // DMA1 Clock

    // Priority
    MODIFY_REG(dma_stream->CR, DMA_SxCR_PL, 0b10 << DMA_SxCR_PL_Pos); // High Priority

    // Half-word (16bit) data sizes
    MODIFY_REG(dma_stream->CR, DMA_SxCR_MSIZE, 0b01 << DMA_SxCR_MSIZE_Pos); // memory
    MODIFY_REG(dma_stream->CR, DMA_SxCR_PSIZE, 0b01 << DMA_SxCR_PSIZE_Pos); // peripheral

    // Address incrementation
    SET_BIT(dma_stream->CR, DMA_SxCR_MINC); // memory
    CLEAR_BIT(dma_stream->CR, DMA_SxCR_PINC); // peripheral

    // Circular mode
    SET_BIT(dma_stream->CR, DMA_SxCR_CIRC); // ON

    // Data transfer direction
    MODIFY_REG(dma_stream->CR, DMA_SxCR_DIR, 0b00 << DMA_SxCR_DIR_Pos); // peripheral -> memory

    // Enable Interrupts
    SET_BIT(dma_stream->CR, DMA_SxCR_TCIE); // transfer complete
    SET_BIT(dma_stream->CR, DMA_SxCR_TEIE); // transfer error
    NVIC_SetPriority(dma_irq, 3);
    NVIC_EnableIRQ(dma_irq);

    // Number of data items to transfer
    MODIFY_REG(dma_stream->NDTR, DMA_SxNDT, (64U) << DMA_SxNDT_Pos);
    
    // Peripheral data register address
    WRITE_REG(dma_stream->PAR, periph_address);

    // Memory data register address
    WRITE_REG(dma_stream->M0AR, mem_address);
}

void dmamux_init(void) {
    // DMAMUX1 request ID - ADC1
    MODIFY_REG(DMAMUX1_Channel4->CCR, DMAMUX_CxCR_DMAREQ_ID, (9U) << DMAMUX_CxCR_DMAREQ_ID_Pos); 
}

void dma_clear_status_flags(void) {
    SET_BIT(DMA1->HIFCR, DMA_HIFCR_CTCIF4);
    SET_BIT(DMA1->HIFCR, DMA_HIFCR_CHTIF4);
    SET_BIT(DMA1->HIFCR, DMA_HIFCR_CTEIF4);

    if (READ_REG(DMA1->LISR) | READ_REG(DMA1->HISR)) {
        error = DMA_ERROR_INTERRUPTS_NOT_CLEARED;
    }
}

void DMA1_Stream4_IRQHandler(void) {
    if (READ_BIT(DMA1->HISR, DMA_HISR_TCIF4)) {
        SET_BIT(DMA1->HIFCR, DMA_HIFCR_CTCIF4);
        transfer_status = 1;
    }

    if (READ_BIT(DMA1->HISR, DMA_HISR_TEIF4)) {
        SET_BIT(DMA1->HIFCR, DMA_HIFCR_CTEIF4);
        error = DMA_ERROR_INTERRUPT_TRANSFER_ERROR;
    }
}
