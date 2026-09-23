#include "stm32f1xx_hal.h"
#include "stm32f1xx_it.h"


extern DMA_HandleTypeDef hdma_adc1;


/* SysTick */
void SysTick_Handler(void)
{
    HAL_IncTick();
}


/*
 * ADC1 dùng DMA1 Channel 1.
 *
 * HAL_DMA_IRQHandler() sẽ kiểm tra
 * Half Transfer và Transfer Complete,
 * sau đó gọi callback tương ứng.
 */
void DMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(
        &hdma_adc1
    );
}
