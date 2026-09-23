#include "stm32f1xx_hal.h"
#include "stm32f1xx_it.h"


extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_usart1_tx;


/* =========================================================
 * SysTick
 * ========================================================= */
void SysTick_Handler(void)
{
    HAL_IncTick();
}


/* =========================================================
 * Button PB12
 *
 * PB12 thuộc EXTI15_10
 * ========================================================= */
void EXTI15_10_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(
        GPIO_PIN_12
    );
}


/* =========================================================
 * USART1 TX DMA
 *
 * USART1_TX = DMA1 Channel 4
 * ========================================================= */
void DMA1_Channel4_IRQHandler(void)
{
    HAL_DMA_IRQHandler(
        &hdma_usart1_tx
    );
}


/* =========================================================
 * USART1
 * ========================================================= */
void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(
        &huart1
    );
}

