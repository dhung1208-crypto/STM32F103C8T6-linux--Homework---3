#include "stm32f1xx_hal.h"


SPI_HandleTypeDef hspi1;


/* =========================
 * Function prototypes
 * ========================= */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void SPI1_Init(void);
static void Error_Handler(void);

static void MAX7219_Send(uint8_t address, uint8_t data);
static void MAX7219_Init(void);
static void MAX7219_Clear(void);
static void MAX7219_DisplayMatrix(const uint8_t pattern[8]);


/* =========================================================
 * Font 8x8 cho các số 0 -> 9
 *
 * Mỗi byte tương ứng với 1 hàng của LED Matrix.
 *
 * bit = 1 : LED sáng
 * bit = 0 : LED tắt
 * ========================================================= */

static const uint8_t digits[10][8] =
{
    /* 0 */
    {
        0x3C,
        0x66,
        0x6E,
        0x76,
        0x66,
        0x66,
        0x3C,
        0x00
    },

    /* 1 */
    {
        0x18,
        0x38,
        0x18,
        0x18,
        0x18,
        0x18,
        0x7E,
        0x00
    },

    /* 2 */
    {
        0x3C,
        0x66,
        0x06,
        0x0C,
        0x18,
        0x30,
        0x7E,
        0x00
    },

    /* 3 */
    {
        0x3C,
        0x66,
        0x06,
        0x1C,
        0x06,
        0x66,
        0x3C,
        0x00
    },

    /* 4 */
    {
        0x0C,
        0x1C,
        0x3C,
        0x6C,
        0x7E,
        0x0C,
        0x0C,
        0x00
    },

    /* 5 */
    {
        0x7E,
        0x60,
        0x60,
        0x7C,
        0x06,
        0x66,
        0x3C,
        0x00
    },

    /* 6 */
    {
        0x1C,
        0x30,
        0x60,
        0x7C,
        0x66,
        0x66,
        0x3C,
        0x00
    },

    /* 7 */
    {
        0x7E,
        0x06,
        0x0C,
        0x18,
        0x30,
        0x30,
        0x30,
        0x00
    },

    /* 8 */
    {
        0x3C,
        0x66,
        0x66,
        0x3C,
        0x66,
        0x66,
        0x3C,
        0x00
    },

    /* 9 */
    {
        0x3C,
        0x66,
        0x66,
        0x3E,
        0x06,
        0x0C,
        0x38,
        0x00
    }
};


/* =========================================================
 * MAIN
 * ========================================================= */
int main(void)
{
    HAL_Init();

    SystemClock_Config();

    GPIO_Init();

    SPI1_Init();


    /*
     * Chờ MAX7219 ổn định sau khi cấp nguồn
     */
    HAL_Delay(100);


    /*
     * Khởi tạo MAX7219
     */
    MAX7219_Init();


    uint8_t number = 0;


    while (1)
    {
        /*
         * Hiển thị số hiện tại
         */
        MAX7219_DisplayMatrix(digits[number]);


        /*
         * Chờ 1 giây
         */
        HAL_Delay(1000);


        /*
         * Tăng số
         */
        number++;


        /*
         * Sau số 9 quay lại số 0
         */
        if (number > 9)
        {
            number = 0;
        }
    }
}


/* =========================================================
 * Gửi 16 bit tới MAX7219
 *
 * MAX7219 nhận:
 *
 * 8 bit đầu  = địa chỉ register
 * 8 bit sau  = dữ liệu
 *
 * PA4 = CS
 * ========================================================= */
static void MAX7219_Send(uint8_t address,
                         uint8_t data)
{
    uint8_t tx_data[2];


    tx_data[0] = address;

    tx_data[1] = data;


    /*
     * CS LOW
     *
     * Bắt đầu truyền dữ liệu.
     */
    HAL_GPIO_WritePin(
        GPIOA,
        GPIO_PIN_4,
        GPIO_PIN_RESET
    );


    /*
     * Truyền 2 byte qua SPI
     */
    HAL_SPI_Transmit(
        &hspi1,
        tx_data,
        2,
        HAL_MAX_DELAY
    );


    /*
     * CS HIGH
     *
     * MAX7219 chốt 16 bit vừa nhận.
     */
    HAL_GPIO_WritePin(
        GPIOA,
        GPIO_PIN_4,
        GPIO_PIN_SET
    );
}


/* =========================================================
 * Khởi tạo MAX7219 cho LED Matrix 8x8
 * ========================================================= */
static void MAX7219_Init(void)
{
    /*
     * 0x0F = Display Test Register
     *
     * 0x00 = tắt chế độ test
     *
     * Nếu bằng 0x01:
     * toàn bộ LED sẽ sáng.
     */
    MAX7219_Send(0x0F, 0x00);


    /*
     * 0x09 = Decode Mode
     *
     * 0x00 = NO DECODE
     *
     * RẤT QUAN TRỌNG với LED Matrix.
     *
     * Không được dùng 0xFF vì 0xFF
     * dành cho LED 7 đoạn.
     */
    MAX7219_Send(0x09, 0x00);


    /*
     * 0x0B = Scan Limit
     *
     * 0x07 = dùng đủ 8 hàng
     */
    MAX7219_Send(0x0B, 0x07);


    /*
     * 0x0A = Intensity
     *
     * Giá trị từ:
     *
     * 0x00 -> tối nhất
     * 0x0F -> sáng nhất
     *
     * chọn 0x03 để không quá sáng.
     */
    MAX7219_Send(0x0A, 0x03);


    /*
     * 0x0C = Shutdown Register
     *
     * 0x01 = Normal Operation
     */
    MAX7219_Send(0x0C, 0x01);


    /*
     * Xóa toàn bộ matrix.
     */
    MAX7219_Clear();
}


/* =========================================================
 * Xóa LED Matrix
 * ========================================================= */
static void MAX7219_Clear(void)
{
    /*
     * Register 1 -> 8 tương ứng
     * với 8 hàng của matrix.
     */
    for (uint8_t row = 1;
         row <= 8;
         row++)
    {
        MAX7219_Send(
            row,
            0x00
        );
    }
}


/* =========================================================
 * Hiển thị một hình 8x8
 * ========================================================= */
static void MAX7219_DisplayMatrix(
    const uint8_t pattern[8])
{
    /*
     * Gửi lần lượt 8 hàng
     * tới MAX7219.
     */
    for (uint8_t row = 0;
         row < 8;
         row++)
    {
        MAX7219_Send(
            row + 1,
            pattern[row]
        );
    }
}


/* =========================================================
 * SPI1 INIT
 *
 * STM32F103:
 *
 * PA5 = SPI1_SCK
 * PA7 = SPI1_MOSI
 * PA4 = CS
 *
 * PA6/MISO không cần dùng.
 * ========================================================= */
static void SPI1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};


    /*
     * Bật clock SPI1
     */
    __HAL_RCC_SPI1_CLK_ENABLE();


    /*
     * PA5 = SCK
     * PA7 = MOSI
     */
    GPIO_InitStruct.Pin =
        GPIO_PIN_5 |
        GPIO_PIN_7;


    /*
     * SPI output:
     * Alternate Function Push Pull
     */
    GPIO_InitStruct.Mode =
        GPIO_MODE_AF_PP;


    GPIO_InitStruct.Speed =
        GPIO_SPEED_FREQ_HIGH;


    HAL_GPIO_Init(
        GPIOA,
        &GPIO_InitStruct
    );


    /*
     * Chọn SPI1
     */
    hspi1.Instance =
        SPI1;


    /*
     * STM32 là SPI Master
     */
    hspi1.Init.Mode =
        SPI_MODE_MASTER;


    /*
     * Cấu hình 2 lines.
     *
     * Nhưng trong bài này chỉ dùng MOSI.
     */
    hspi1.Init.Direction =
        SPI_DIRECTION_2LINES;


    /*
     * Mỗi frame SPI = 8 bit
     */
    hspi1.Init.DataSize =
        SPI_DATASIZE_8BIT;


    /*
     * SPI Mode 0
     *
     * CPOL = 0
     * CPHA = 0
     */
    hspi1.Init.CLKPolarity =
        SPI_POLARITY_LOW;


    hspi1.Init.CLKPhase =
        SPI_PHASE_1EDGE;


    /*
     * CS được điều khiển thủ công
     * bằng GPIO PA4.
     */
    hspi1.Init.NSS =
        SPI_NSS_SOFT;


    /*
     * System Clock = 8 MHz
     *
     * SPI clock:
     *
     * 8 MHz / 16 = 500 kHz
     */
    hspi1.Init.BaudRatePrescaler =
        SPI_BAUDRATEPRESCALER_16;


    /*
     * Gửi bit MSB trước.
     */
    hspi1.Init.FirstBit =
        SPI_FIRSTBIT_MSB;


    /*
     * Không dùng TI mode.
     *
     * Phiên bản HAL của bạn dùng
     * SPI_TIMODE_DISABLED.
     */
    hspi1.Init.TIMode =
        SPI_TIMODE_DISABLED;


    /*
     * Không dùng CRC.
     */
    hspi1.Init.CRCCalculation =
        SPI_CRCCALCULATION_DISABLED;


    hspi1.Init.CRCPolynomial =
        7;


    /*
     * Khởi tạo SPI.
     */
    if (HAL_SPI_Init(&hspi1)
        != HAL_OK)
    {
        Error_Handler();
    }
}


/* =========================================================
 * GPIO INIT
 * ========================================================= */
static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};


    /*
     * Bật clock GPIOA
     */
    __HAL_RCC_GPIOA_CLK_ENABLE();


    /*
     * Bật clock GPIOC
     */
    __HAL_RCC_GPIOC_CLK_ENABLE();


    /*
     * Bật AFIO
     */
    __HAL_RCC_AFIO_CLK_ENABLE();


    /*
     * PA4 = CS / LOAD MAX7219
     */
    GPIO_InitStruct.Pin =
        GPIO_PIN_4;


    GPIO_InitStruct.Mode =
        GPIO_MODE_OUTPUT_PP;


    GPIO_InitStruct.Speed =
        GPIO_SPEED_FREQ_HIGH;


    HAL_GPIO_Init(
        GPIOA,
        &GPIO_InitStruct
    );


    /*
     * Bình thường CS ở mức HIGH.
     */
    HAL_GPIO_WritePin(
        GPIOA,
        GPIO_PIN_4,
        GPIO_PIN_SET
    );


    /*
     * PC13 = LED onboard Blue Pill
     *
     * dùng để báo Error_Handler.
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
     * LED onboard thường active LOW.
     *
     * SET = tắt LED.
     */
    HAL_GPIO_WritePin(
        GPIOC,
        GPIO_PIN_13,
        GPIO_PIN_SET
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


    /*
     * Sử dụng HSI nội = 8 MHz
     */
    RCC_OscInitStruct.OscillatorType =
        RCC_OSCILLATORTYPE_HSI;


    RCC_OscInitStruct.HSIState =
        RCC_HSI_ON;


    RCC_OscInitStruct.HSICalibrationValue =
        RCC_HSICALIBRATION_DEFAULT;


    /*
     * Không sử dụng PLL
     */
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
 * ERROR HANDLER
 *
 * Nếu có lỗi khởi tạo,
 * LED PC13 sẽ nhấp nháy.
 * ========================================================= */
static void Error_Handler(void)
{
    while (1)
    {
        HAL_GPIO_TogglePin(
            GPIOC,
            GPIO_PIN_13
        );

        HAL_Delay(200);
    }
}
