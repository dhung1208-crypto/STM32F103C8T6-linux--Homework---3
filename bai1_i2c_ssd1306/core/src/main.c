#include "stm32f1xx_hal.h"
#include "ssd1306.h"


I2C_HandleTypeDef hi2c1;


static void SystemClock_Config(void);
static void GPIO_Init(void);
static void I2C1_Init(void);
static void Error_Handler(void);


/*
 * Bitmap 16x16 đen trắng.
 *
 * Đây là hình mặt cười đơn giản.
 * 1 bit = 1 pixel.
 *
 * Sau đó chương trình phóng to 4 lần
 * thành hình 64x64 trên OLED.
 */
static const uint8_t smile_bitmap[] =
{
    0x07, 0xE0,
    0x18, 0x18,
    0x20, 0x04,
    0x46, 0x62,
    0x46, 0x62,
    0x80, 0x01,
    0x80, 0x01,
    0x80, 0x01,
    0x90, 0x09,
    0x88, 0x11,
    0x47, 0xE2,
    0x40, 0x02,
    0x20, 0x04,
    0x18, 0x18,
    0x07, 0xE0,
    0x00, 0x00
};


int main(void)
{
    HAL_Init();

    SystemClock_Config();

    GPIO_Init();

    I2C1_Init();


    /*
     * Chờ OLED ổn định nguồn.
     */
    HAL_Delay(100);


    /*
     * Khởi tạo OLED.
     *
     * Driver tự thử:
     * 0x3C
     * sau đó 0x3D
     */
    if (SSD1306_Init(&hi2c1) != HAL_OK)
    {
        /*
         * Không tìm thấy OLED:
         * LED PC13 sẽ nháy liên tục.
         */
        while (1)
        {
            HAL_GPIO_TogglePin(
                GPIOC,
                GPIO_PIN_13
            );

            HAL_Delay(200);
        }
    }


    /*
     * Nếu tới được đây:
     * STM32 đã giao tiếp I2C thành công với OLED.
     */

    HAL_GPIO_WritePin(
        GPIOC,
        GPIO_PIN_13,
        GPIO_PIN_RESET
    );


    /*
     * Xóa màn hình.
     */
    SSD1306_Clear();


    /*
     * Bitmap gốc 16x16.
     *
     * scale = 4
     *
     * 16 x 4 = 64 pixel
     *
     * OLED cao 64 pixel nên hình sẽ cao
     * toàn bộ màn hình.
     *
     * x = 32:
     *
     * (128 - 64) / 2 = 32
     *
     * => hình nằm giữa màn hình.
     */
    SSD1306_DrawBitmapScaled(
        32,
        0,
        smile_bitmap,
        16,
        16,
        4
    );


    /*
     * Gửi framebuffer thực tế lên OLED.
     */
    SSD1306_UpdateScreen();


    while (1)
    {
        /*
         * Không cần làm gì thêm.
         *
         * SSD1306 giữ nguyên hình
         * sau khi dữ liệu đã được gửi.
         */
    }
}


/* =========================================================
 * Clock System
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
            &RCC_OscInitStruct) != HAL_OK)
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
            FLASH_LATENCY_0) != HAL_OK)
    {
        Error_Handler();
    }
}


/* =========================================================
 * GPIO
 * ========================================================= */
static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};


    __HAL_RCC_GPIOB_CLK_ENABLE();

    __HAL_RCC_GPIOC_CLK_ENABLE();

    __HAL_RCC_AFIO_CLK_ENABLE();


    /*
     * PC13 = LED onboard Blue Pill
     *
     * LED onboard thường active LOW.
     */
    GPIO_InitStruct.Pin =
        GPIO_PIN_13;

    GPIO_InitStruct.Mode =
        GPIO_MODE_OUTPUT_PP;

    GPIO_InitStruct.Speed =
        GPIO_SPEED_FREQ_LOW;


    HAL_GPIO_Init(
        GPIOC,
        &GPIO_InitStruct
    );


    /*
     * Ban đầu tắt LED.
     */
    HAL_GPIO_WritePin(
        GPIOC,
        GPIO_PIN_13,
        GPIO_PIN_SET
    );
}


/* =========================================================
 * I2C1
 *
 * PB6 = I2C1_SCL
 * PB7 = I2C1_SDA
 *
 * Frequency = 100 kHz
 * ========================================================= */
static void I2C1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};


    __HAL_RCC_I2C1_CLK_ENABLE();


    /*
     * I2C sử dụng Open Drain.
     */
    GPIO_InitStruct.Pin =
        GPIO_PIN_6 |
        GPIO_PIN_7;


    GPIO_InitStruct.Mode =
        GPIO_MODE_AF_OD;


    GPIO_InitStruct.Speed =
        GPIO_SPEED_FREQ_HIGH;


    HAL_GPIO_Init(
        GPIOB,
        &GPIO_InitStruct
    );


    hi2c1.Instance =
        I2C1;


    hi2c1.Init.ClockSpeed =
        100000;


    hi2c1.Init.DutyCycle =
        I2C_DUTYCYCLE_2;


    hi2c1.Init.OwnAddress1 =
        0;


    hi2c1.Init.AddressingMode =
        I2C_ADDRESSINGMODE_7BIT;


    hi2c1.Init.DualAddressMode =
        I2C_DUALADDRESS_DISABLE;


    hi2c1.Init.OwnAddress2 =
        0;


    hi2c1.Init.GeneralCallMode =
        I2C_GENERALCALL_DISABLE;


    hi2c1.Init.NoStretchMode =
        I2C_NOSTRETCH_DISABLE;


    if (HAL_I2C_Init(
            &hi2c1) != HAL_OK)
    {
        Error_Handler();
    }
}


/* =========================================================
 * Error Handler
 * ========================================================= */
static void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
    }
}
