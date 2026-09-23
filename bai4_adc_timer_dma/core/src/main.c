#include "stm32f1xx_hal.h"
#include <stdio.h>


/* =========================================================
 * Handles
 * ========================================================= */

ADC_HandleTypeDef hadc1;
TIM_HandleTypeDef htim3;
UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_adc1;


/* =========================================================
 * 100 Hz = 100 mẫu / giây
 *
 * Vì vậy buffer 100 phần tử chứa đúng 1 giây dữ liệu.
 * ========================================================= */

#define ADC_BUFFER_SIZE 100
#define ADC_HALF_SIZE   50


uint32_t adc_buffer[ADC_BUFFER_SIZE];


/*
 * Hai cờ báo:
 *
 * first_half_ready:
 * mẫu 0 -> 49 đã an toàn.
 *
 * second_half_ready:
 * mẫu 50 -> 99 đã an toàn.
 */
volatile uint8_t first_half_ready = 0;
volatile uint8_t second_half_ready = 0;


/*
 * Buffer UART.
 *
 * ADC tối đa 4095:
 *
 * "4095\n\r" = 6 byte
 *
 * 50 mẫu ≈ 300 byte.
 *
 * 400 byte là đủ.
 */
char tx_buffer[400];


/* =========================================================
 * Prototypes
 * ========================================================= */

static void SystemClock_Config(void);
static void GPIO_Init(void);
static void DMA_Init(void);
static void USART1_Init(void);
static void TIM3_Init(void);
static void ADC1_Init(void);
static void Send_ADC_Block(uint16_t start,
                           uint16_t count);
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

    TIM3_Init();

    ADC1_Init();


    /*
     * Hiệu chuẩn ADC.
     */
    if (HAL_ADCEx_Calibration_Start(&hadc1)
        != HAL_OK)
    {
        Error_Handler();
    }


    /*
     * Bắt đầu ADC + DMA.
     *
     * DMA chạy ở Circular Mode.
     *
     * adc_buffer có 100 phần tử.
     */
    if (HAL_ADC_Start_DMA(
            &hadc1,
            adc_buffer,
            ADC_BUFFER_SIZE)
        != HAL_OK)
    {
        Error_Handler();
    }


    /*
     * Sau khi ADC + DMA đã sẵn sàng
     * mới bật Timer.
     *
     * TIM3 bắt đầu tạo trigger 100 Hz.
     */
    if (HAL_TIM_Base_Start(&htim3)
        != HAL_OK)
    {
        Error_Handler();
    }


    while (1)
    {
        /*
         * DMA đã ghi xong:
         *
         * adc_buffer[0] -> adc_buffer[49]
         *
         * Trong thời gian ta gửi nửa này,
         * DMA đang ghi vào nửa thứ hai.
         */
        if (first_half_ready)
        {
            first_half_ready = 0;

            Send_ADC_Block(
                0,
                ADC_HALF_SIZE
            );
        }


        /*
         * DMA đã ghi xong:
         *
         * adc_buffer[50] -> adc_buffer[99]
         *
         * Trong thời gian ta gửi nửa này,
         * DMA đã quay lại ghi nửa đầu.
         */
        if (second_half_ready)
        {
            second_half_ready = 0;

            Send_ADC_Block(
                ADC_HALF_SIZE,
                ADC_HALF_SIZE
            );
        }
    }
}


/* =========================================================
 * ADC DMA HALF TRANSFER CALLBACK
 *
 * Được gọi sau khi DMA nhận đủ 50 mẫu.
 * ========================================================= */

void HAL_ADC_ConvHalfCpltCallback(
    ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        first_half_ready = 1;
    }
}


/* =========================================================
 * ADC DMA TRANSFER COMPLETE CALLBACK
 *
 * Được gọi sau khi DMA nhận đủ 100 mẫu.
 * ========================================================= */

void HAL_ADC_ConvCpltCallback(
    ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        second_half_ready = 1;
    }
}


/* =========================================================
 * Gửi một nửa buffer lên UART
 *
 * Mỗi giá trị có dạng:
 *
 * 1234\n\r
 *
 * Đúng yêu cầu dữ liệu ngăn cách bằng \n\r.
 * ========================================================= */

static void Send_ADC_Block(uint16_t start,
                           uint16_t count)
{
    uint16_t length = 0;


    for (uint16_t i = 0;
         i < count;
         i++)
    {
        uint32_t value =
            adc_buffer[start + i];


        int written = snprintf(
            &tx_buffer[length],
            sizeof(tx_buffer) - length,
            "%lu\n\r",
            (unsigned long)value
        );


        if (written <= 0)
        {
            break;
        }


        length += (uint16_t)written;


        /*
         * Tránh vượt buffer UART.
         */
        if (length >= sizeof(tx_buffer) - 8)
        {
            break;
        }
    }


    /*
     * UART được gọi ở main(),
     * KHÔNG gọi blocking UART trong ISR.
     *
     * Vì vậy callback DMA chỉ đặt cờ.
     */
    HAL_UART_Transmit(
        &huart1,
        (uint8_t *)tx_buffer,
        length,
        HAL_MAX_DELAY
    );
}


/* =========================================================
 * ADC1
 *
 * PA4 = ADC1_IN4
 *
 * Trigger:
 * TIM3 TRGO
 * ========================================================= */

static void ADC1_Init(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};


    /*
     * ADC clock.
     *
     * PCLK2 = 8 MHz.
     *
     * ADC clock:
     * 8 MHz / 6
     * ≈ 1.33 MHz.
     */
    __HAL_RCC_ADC_CONFIG(
        RCC_ADCPCLK2_DIV6
    );


    __HAL_RCC_ADC1_CLK_ENABLE();


    hadc1.Instance =
        ADC1;


    /*
     * Chỉ 1 channel.
     */
    hadc1.Init.ScanConvMode =
        ADC_SCAN_DISABLE;


    /*
     * KHÔNG chạy liên tục.
     *
     * Mỗi conversion do Timer trigger.
     */
    hadc1.Init.ContinuousConvMode =
        DISABLE;


    hadc1.Init.DiscontinuousConvMode =
        DISABLE;


    /*
     * Trigger từ TIM3 TRGO.
     */
    hadc1.Init.ExternalTrigConv =
        ADC_EXTERNALTRIGCONV_T3_TRGO;


    hadc1.Init.DataAlign =
        ADC_DATAALIGN_RIGHT;


    /*
     * Mỗi trigger chỉ chuyển đổi 1 channel.
     */
    hadc1.Init.NbrOfConversion =
        1;


    if (HAL_ADC_Init(&hadc1)
        != HAL_OK)
    {
        Error_Handler();
    }


    /*
     * ADC Channel 4 = PA4
     */
    sConfig.Channel =
        ADC_CHANNEL_4;


    sConfig.Rank =
        ADC_REGULAR_RANK_1;


    sConfig.SamplingTime =
        ADC_SAMPLETIME_55CYCLES_5;


    if (HAL_ADC_ConfigChannel(
            &hadc1,
            &sConfig)
        != HAL_OK)
    {
        Error_Handler();
    }


    /*
     * Liên kết ADC1 với DMA1 Channel 1.
     */
    __HAL_LINKDMA(
        &hadc1,
        DMA_Handle,
        hdma_adc1
    );
}


/* =========================================================
 * TIMER 3
 *
 * Clock = 8 MHz
 *
 * PSC = 7999
 *
 * 8 MHz / (7999 + 1)
 * = 1000 Hz
 *
 * ARR = 9
 *
 * 1000 / (9 + 1)
 * = 100 Hz
 *
 * => Timer Update Event = 100 Hz
 * ========================================================= */

static void TIM3_Init(void)
{
    TIM_MasterConfigTypeDef sMasterConfig = {0};


    __HAL_RCC_TIM3_CLK_ENABLE();


    htim3.Instance =
        TIM3;


    htim3.Init.Prescaler =
        7999;


    htim3.Init.CounterMode =
        TIM_COUNTERMODE_UP;


    htim3.Init.Period =
        9;


    htim3.Init.ClockDivision =
        TIM_CLOCKDIVISION_DIV1;


    if (HAL_TIM_Base_Init(&htim3)
        != HAL_OK)
    {
        Error_Handler();
    }


    /*
     * Mỗi Update Event của TIM3
     * được xuất ra TRGO.
     *
     * ADC1 dùng TRGO này làm trigger.
     */
    sMasterConfig.MasterOutputTrigger =
        TIM_TRGO_UPDATE;


    sMasterConfig.MasterSlaveMode =
        TIM_MASTERSLAVEMODE_DISABLE;


    if (HAL_TIMEx_MasterConfigSynchronization(
            &htim3,
            &sMasterConfig)
        != HAL_OK)
    {
        Error_Handler();
    }
}


/* =========================================================
 * DMA
 *
 * ADC1 -> DMA1 Channel 1
 * ========================================================= */

static void DMA_Init(void)
{
    __HAL_RCC_DMA1_CLK_ENABLE();


    hdma_adc1.Instance =
        DMA1_Channel1;


    /*
     * ADC -> RAM
     */
    hdma_adc1.Init.Direction =
        DMA_PERIPH_TO_MEMORY;


    /*
     * ADC DR luôn cùng địa chỉ.
     */
    hdma_adc1.Init.PeriphInc =
        DMA_PINC_DISABLE;


    /*
     * RAM tăng địa chỉ:
     *
     * adc_buffer[0]
     * adc_buffer[1]
     * ...
     */
    hdma_adc1.Init.MemInc =
        DMA_MINC_ENABLE;


    /*
     * Buffer dùng uint32_t.
     */
    hdma_adc1.Init.PeriphDataAlignment =
        DMA_PDATAALIGN_WORD;


    hdma_adc1.Init.MemDataAlignment =
        DMA_MDATAALIGN_WORD;


    /*
     * CIRCULAR rất quan trọng.
     *
     * Sau phần tử 99,
     * DMA tự quay lại phần tử 0.
     */
    hdma_adc1.Init.Mode =
        DMA_CIRCULAR;


    hdma_adc1.Init.Priority =
        DMA_PRIORITY_HIGH;


    if (HAL_DMA_Init(&hdma_adc1)
        != HAL_OK)
    {
        Error_Handler();
    }


    /*
     * Bật interrupt cho DMA1 Channel 1.
     *
     * HAL sẽ xử lý:
     *
     * Half Transfer
     * Transfer Complete
     */
    HAL_NVIC_SetPriority(
        DMA1_Channel1_IRQn,
        0,
        0
    );


    HAL_NVIC_EnableIRQ(
        DMA1_Channel1_IRQn
    );
}


/* =========================================================
 * UART1
 *
 * PA9 = TX
 *
 * 115200 baud
 * ========================================================= */

static void USART1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};


    __HAL_RCC_USART1_CLK_ENABLE();


    /*
     * PA9 = USART1 TX
     */
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


    /*
     * Bài này chỉ cần TX.
     */
    huart1.Init.Mode =
        UART_MODE_TX;


    huart1.Init.HwFlowCtl =
        UART_HWCONTROL_NONE;


    huart1.Init.OverSampling =
        UART_OVERSAMPLING_16;


    if (HAL_UART_Init(&huart1)
        != HAL_OK)
    {
        Error_Handler();
    }
}


/* =========================================================
 * GPIO
 *
 * PA4 = ADC1_IN4
 * ========================================================= */

static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};


    __HAL_RCC_GPIOA_CLK_ENABLE();

    __HAL_RCC_AFIO_CLK_ENABLE();


    /*
     * PA4 = Analog input.
     *
     * Chân analog không dùng:
     *
     * pull-up
     * pull-down
     * digital input
     */
    GPIO_InitStruct.Pin =
        GPIO_PIN_4;


    GPIO_InitStruct.Mode =
        GPIO_MODE_ANALOG;


    HAL_GPIO_Init(
        GPIOA,
        &GPIO_InitStruct
    );
}


/* =========================================================
 * CLOCK
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
