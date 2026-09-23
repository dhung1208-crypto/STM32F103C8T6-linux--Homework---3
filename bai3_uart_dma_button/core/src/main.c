#include "stm32f1xx_hal.h"
#include <stdio.h>


/* =========================================================
 * ID lớp + nhóm
 *
 * THAY XX bằng mã nhóm thật của bạn.
 *
 * Ví dụ nhóm 01:
 * #define CLASS_GROUP_ID "D23DT0401"
 * ========================================================= */
#define CLASS_GROUP_ID "D23DT04"


UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_tx;


/*
 * Số lần nút đã được nhấn.
 */
volatile uint32_t button_count = 0;


/*
 * Giá trị cuối cùng đã gửi lên PC.
 *
 * Nếu button_count > last_sent_count
 * nghĩa là còn dữ liệu cần gửi.
 */
volatile uint32_t last_sent_count = 0;


/*
 * = 1 khi DMA UART đang truyền.
 * = 0 khi truyền xong.
 */
volatile uint8_t uart_tx_busy = 0;


/*
 * Dùng chống dội nút.
 */
volatile uint32_t last_press_tick = 0;


/*
 * Buffer phải là biến global/static.
 *
 * Vì DMA vẫn sử dụng vùng nhớ này
 * sau khi HAL_UART_Transmit_DMA()
 * đã trả về.
 */
char tx_buffer[64];


/* Function prototypes */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void DMA_Init(void);
static void USART1_Init(void);
static void Error_Handler(void);


/* =========================================================
 * MAIN
 * ========================================================= */
int main(void)
{
    HAL_Init();

    SystemClock_Config();

    GPIO_Init();

    DMA_Init();

    USART1_Init();


    while (1)
    {
        /*
         * Nếu:
         *
         * - DMA đang rảnh
         * - còn lần nhấn chưa được gửi
         *
         * thì tạo bản tin tiếp theo.
         */
        if ((uart_tx_busy == 0) &&
            (last_sent_count < button_count))
        {
            /*
             * Gửi lần nhấn tiếp theo.
             */
            last_sent_count++;


            int length = snprintf(
                tx_buffer,
                sizeof(tx_buffer),
                CLASS_GROUP_ID ":BTN:%lu\n\r",
                (unsigned long)last_sent_count
            );


            if (length > 0)
            {
                uart_tx_busy = 1;


                /*
                 * QUAN TRỌNG:
                 *
                 * Truyền UART bằng DMA.
                 *
                 * KHÔNG sử dụng:
                 * HAL_UART_Transmit()
                 */
                if (HAL_UART_Transmit_DMA(
                        &huart1,
                        (uint8_t *)tx_buffer,
                        (uint16_t)length)
                    != HAL_OK)
                {
                    /*
                     * Nếu khởi động DMA thất bại
                     * thì cho phép thử lại.
                     */
                    uart_tx_busy = 0;

                    last_sent_count--;
                }
            }
        }
    }
}


/* =========================================================
 * CALLBACK NÚT NHẤN
 *
 * PB12 sử dụng EXTI.
 *
 * Hàm này được HAL gọi khi PB12
 * xuất hiện cạnh xuống.
 * ========================================================= */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_12)
    {
        uint32_t now = HAL_GetTick();


        /*
         * Chống dội nút 50 ms.
         *
         * Một lần nhấn cơ khí có thể tạo
         * nhiều xung rất nhanh.
         *
         * Chỉ công nhận một lần nhấn nếu
         * cách lần trước ít nhất 50 ms.
         */
        if ((now - last_press_tick) >= 50)
        {
            last_press_tick = now;

            button_count++;
        }
    }
}


/* =========================================================
 * CALLBACK UART TX COMPLETE
 *
 * Được gọi khi UART + DMA đã truyền
 * xong hoàn toàn một bản tin.
 * ========================================================= */
void HAL_UART_TxCpltCallback(
    UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        uart_tx_busy = 0;
    }
}


/* =========================================================
 * USART1 INIT
 *
 * PA9  = TX
 * PA10 = RX
 *
 * 115200 8N1
 * ========================================================= */
static void USART1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};


    __HAL_RCC_USART1_CLK_ENABLE();


    /* =========================
     * PA9 = USART1 TX
     * ========================= */
    GPIO_InitStruct.Pin =
        GPIO_PIN_9;

    GPIO_InitStruct.Mode =
        GPIO_MODE_AF_PP;

    GPIO_InitStruct.Speed =
        GPIO_SPEED_FREQ_HIGH;


    HAL_GPIO_Init(
        GPIOA,
        &GPIO_InitStruct
    );


    /* =========================
     * PA10 = USART1 RX
     * ========================= */
    GPIO_InitStruct.Pin =
        GPIO_PIN_10;

    GPIO_InitStruct.Mode =
        GPIO_MODE_INPUT;

    GPIO_InitStruct.Pull =
        GPIO_NOPULL;


    HAL_GPIO_Init(
        GPIOA,
        &GPIO_InitStruct
    );


    /* =========================
     * UART
     * ========================= */
    huart1.Instance =
        USART1;


    huart1.Init.BaudRate =
        115200;


    huart1.Init.WordLength =
        UART_WORDLENGTH_8B;


    huart1.Init.StopBits =
        UART_STOPBITS_1;


    huart1.Init.Parity =
        UART_PARITY_NONE;


    huart1.Init.Mode =
        UART_MODE_TX_RX;


    huart1.Init.HwFlowCtl =
        UART_HWCONTROL_NONE;


    huart1.Init.OverSampling =
        UART_OVERSAMPLING_16;


    if (HAL_UART_Init(&huart1)
        != HAL_OK)
    {
        Error_Handler();
    }


    /*
     * Liên kết USART1 TX
     * với DMA1 Channel 4.
     */
    __HAL_LINKDMA(
        &huart1,
        hdmatx,
        hdma_usart1_tx
    );


    /*
     * Bật ngắt USART1.
     *
     * DMA truyền hết dữ liệu trước,
     * sau đó UART cần xử lý cờ
     * Transmission Complete.
     */
    HAL_NVIC_SetPriority(
        USART1_IRQn,
        1,
        0
    );


    HAL_NVIC_EnableIRQ(
        USART1_IRQn
    );
}


/* =========================================================
 * DMA INIT
 *
 * STM32F103:
 *
 * USART1_TX -> DMA1 Channel 4
 * ========================================================= */
static void DMA_Init(void)
{
    /*
     * Bật clock DMA1.
     */
    __HAL_RCC_DMA1_CLK_ENABLE();


    /*
     * Chọn DMA1 Channel 4.
     */
    hdma_usart1_tx.Instance =
        DMA1_Channel4;


    /*
     * Dữ liệu đi:
     *
     * RAM -> USART1
     */
    hdma_usart1_tx.Init.Direction =
        DMA_MEMORY_TO_PERIPH;


    /*
     * Địa chỉ thanh ghi UART
     * không được tăng.
     */
    hdma_usart1_tx.Init.PeriphInc =
        DMA_PINC_DISABLE;


    /*
     * Địa chỉ buffer trong RAM
     * phải tăng từng byte.
     */
    hdma_usart1_tx.Init.MemInc =
        DMA_MINC_ENABLE;


    /*
     * UART truyền từng byte.
     */
    hdma_usart1_tx.Init.PeriphDataAlignment =
        DMA_PDATAALIGN_BYTE;


    hdma_usart1_tx.Init.MemDataAlignment =
        DMA_MDATAALIGN_BYTE;


    /*
     * NORMAL:
     *
     * truyền hết một bản tin rồi dừng.
     */
    hdma_usart1_tx.Init.Mode =
        DMA_NORMAL;


    hdma_usart1_tx.Init.Priority =
        DMA_PRIORITY_LOW;


    if (HAL_DMA_Init(
            &hdma_usart1_tx)
        != HAL_OK)
    {
        Error_Handler();
    }


    /*
     * Bật interrupt DMA1 Channel 4.
     */
    HAL_NVIC_SetPriority(
        DMA1_Channel4_IRQn,
        0,
        0
    );


    HAL_NVIC_EnableIRQ(
        DMA1_Channel4_IRQn
    );
}


/* =========================================================
 * GPIO
 *
 * PB12 = Button
 * ========================================================= */
static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};


    __HAL_RCC_GPIOA_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();

    __HAL_RCC_GPIOC_CLK_ENABLE();

    __HAL_RCC_AFIO_CLK_ENABLE();


    /*
     * PB12 = nút nhấn.
     *
     * Input interrupt Falling Edge.
     *
     * Pull-up nội:
     *
     * bình thường = HIGH
     * nhấn nút = LOW
     */
    GPIO_InitStruct.Pin =
        GPIO_PIN_12;


    GPIO_InitStruct.Mode =
        GPIO_MODE_IT_FALLING;


    GPIO_InitStruct.Pull =
        GPIO_PULLUP;


    HAL_GPIO_Init(
        GPIOB,
        &GPIO_InitStruct
    );


    /*
     * PB12 thuộc EXTI10 -> EXTI15.
     *
     * Vì vậy dùng EXTI15_10_IRQn.
     */
    HAL_NVIC_SetPriority(
        EXTI15_10_IRQn,
        2,
        0
    );


    HAL_NVIC_EnableIRQ(
        EXTI15_10_IRQn
    );
}


/* =========================================================
 * SYSTEM CLOCK
 *
 * HSI = 8 MHz
 * ========================================================= */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};

    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};


    RCC_OscInitStruct.OscillatorType =
        RCC_OSCILLATORTYPE_HSI;


    RCC_OscInitStruct.HSIState =
        RCC_HSI_ON;


    RCC_OscInitStruct.HSICalibrationValue =
        RCC_HSICALIBRATION_DEFAULT;


    RCC_OscInitStruct.PLL.PLLState =
        RCC_PLL_NONE;


    if (HAL_RCC_OscConfig(
            &RCC_OscInitStruct)
        != HAL_OK)
    {
        Error_Handler();
    }


    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;


    RCC_ClkInitStruct.SYSCLKSource =
        RCC_SYSCLKSOURCE_HSI;


    RCC_ClkInitStruct.AHBCLKDivider =
        RCC_SYSCLK_DIV1;


    RCC_ClkInitStruct.APB1CLKDivider =
        RCC_HCLK_DIV1;


    RCC_ClkInitStruct.APB2CLKDivider =
        RCC_HCLK_DIV1;


    if (HAL_RCC_ClockConfig(
            &RCC_ClkInitStruct,
            FLASH_LATENCY_0)
        != HAL_OK)
    {
        Error_Handler();
    }
}


/* =========================================================
 * ERROR
 * ========================================================= */
static void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
    }
}
