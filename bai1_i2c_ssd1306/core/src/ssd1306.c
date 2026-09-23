#include "ssd1306.h"
#include <string.h>


#define SSD1306_ADDR_3C   (0x3C << 1)
#define SSD1306_ADDR_3D   (0x3D << 1)


static I2C_HandleTypeDef *ssd1306_i2c;

static uint16_t ssd1306_address;

static uint8_t SSD1306_Buffer[
    SSD1306_WIDTH * SSD1306_HEIGHT / 8
];


/* =========================================================
 * Gửi command cho SSD1306
 * Byte đầu 0x00 nghĩa là dữ liệu sau nó là command
 * ========================================================= */
static HAL_StatusTypeDef SSD1306_WriteCommand(uint8_t command)
{
    uint8_t data[2];

    data[0] = 0x00;
    data[1] = command;

    return HAL_I2C_Master_Transmit(
        ssd1306_i2c,
        ssd1306_address,
        data,
        2,
        HAL_MAX_DELAY
    );
}


/* =========================================================
 * Khởi tạo SSD1306
 * ========================================================= */
HAL_StatusTypeDef SSD1306_Init(I2C_HandleTypeDef *hi2c)
{
    ssd1306_i2c = hi2c;

    HAL_Delay(100);


    /* Thử tìm OLED ở địa chỉ 0x3C */
    if (HAL_I2C_IsDeviceReady(
            ssd1306_i2c,
            SSD1306_ADDR_3C,
            3,
            100) == HAL_OK)
    {
        ssd1306_address = SSD1306_ADDR_3C;
    }

    /* Nếu không có thì thử 0x3D */
    else if (HAL_I2C_IsDeviceReady(
                 ssd1306_i2c,
                 SSD1306_ADDR_3D,
                 3,
                 100) == HAL_OK)
    {
        ssd1306_address = SSD1306_ADDR_3D;
    }

    else
    {
        return HAL_ERROR;
    }


    /* Display OFF */
    if (SSD1306_WriteCommand(0xAE) != HAL_OK)
        return HAL_ERROR;


    /* Memory Addressing Mode */
    SSD1306_WriteCommand(0x20);

    /* Horizontal Addressing Mode */
    SSD1306_WriteCommand(0x00);


    /* Page start */
    SSD1306_WriteCommand(0xB0);


    /* COM Scan Direction */
    SSD1306_WriteCommand(0xC8);


    /* Lower Column */
    SSD1306_WriteCommand(0x00);


    /* Higher Column */
    SSD1306_WriteCommand(0x10);


    /* Display Start Line */
    SSD1306_WriteCommand(0x40);


    /* Contrast */
    SSD1306_WriteCommand(0x81);
    SSD1306_WriteCommand(0x7F);


    /* Segment Remap */
    SSD1306_WriteCommand(0xA1);


    /* Normal display */
    SSD1306_WriteCommand(0xA6);


    /* Multiplex Ratio = 64 */
    SSD1306_WriteCommand(0xA8);
    SSD1306_WriteCommand(0x3F);


    /* Display follows RAM */
    SSD1306_WriteCommand(0xA4);


    /* Display Offset */
    SSD1306_WriteCommand(0xD3);
    SSD1306_WriteCommand(0x00);


    /* Display clock */
    SSD1306_WriteCommand(0xD5);
    SSD1306_WriteCommand(0x80);


    /* Pre-charge */
    SSD1306_WriteCommand(0xD9);
    SSD1306_WriteCommand(0xF1);


    /* COM Pins */
    SSD1306_WriteCommand(0xDA);
    SSD1306_WriteCommand(0x12);


    /* VCOM detect */
    SSD1306_WriteCommand(0xDB);
    SSD1306_WriteCommand(0x40);


    /* Charge Pump */
    SSD1306_WriteCommand(0x8D);
    SSD1306_WriteCommand(0x14);


    /* Display ON */
    SSD1306_WriteCommand(0xAF);


    SSD1306_Clear();

    SSD1306_UpdateScreen();

    return HAL_OK;
}


/* =========================================================
 * Xóa framebuffer
 * ========================================================= */
void SSD1306_Clear(void)
{
    memset(SSD1306_Buffer,
           0x00,
           sizeof(SSD1306_Buffer));
}


/* =========================================================
 * Vẽ một pixel
 * ========================================================= */
void SSD1306_DrawPixel(uint8_t x,
                       uint8_t y,
                       uint8_t color)
{
    if (x >= SSD1306_WIDTH ||
        y >= SSD1306_HEIGHT)
    {
        return;
    }


    uint16_t index =
        x + (y / 8) * SSD1306_WIDTH;

    uint8_t bit =
        1 << (y % 8);


    if (color)
    {
        SSD1306_Buffer[index] |= bit;
    }
    else
    {
        SSD1306_Buffer[index] &= ~bit;
    }
}


/* =========================================================
 * Vẽ bitmap 1-bit
 *
 * Bitmap:
 * bit = 1 -> pixel sáng
 * bit = 0 -> pixel tắt
 * ========================================================= */
void SSD1306_DrawBitmap(uint8_t x,
                        uint8_t y,
                        const uint8_t *bitmap,
                        uint8_t width,
                        uint8_t height)
{
    uint8_t row_bytes =
        (width + 7) / 8;


    for (uint8_t j = 0; j < height; j++)
    {
        for (uint8_t i = 0; i < width; i++)
        {
            uint16_t byte_index =
                j * row_bytes + i / 8;

            uint8_t bit_index =
                7 - (i % 8);


            uint8_t pixel =
                (bitmap[byte_index] >> bit_index) & 0x01;


            SSD1306_DrawPixel(
                x + i,
                y + j,
                pixel
            );
        }
    }
}


/* =========================================================
 * Vẽ bitmap và phóng to
 *
 * scale = 4:
 * 1 pixel ảnh -> 4x4 pixel OLED
 * ========================================================= */
void SSD1306_DrawBitmapScaled(uint8_t x,
                              uint8_t y,
                              const uint8_t *bitmap,
                              uint8_t width,
                              uint8_t height,
                              uint8_t scale)
{
    uint8_t row_bytes =
        (width + 7) / 8;


    for (uint8_t j = 0; j < height; j++)
    {
        for (uint8_t i = 0; i < width; i++)
        {
            uint16_t byte_index =
                j * row_bytes + i / 8;

            uint8_t bit_index =
                7 - (i % 8);


            uint8_t pixel =
                (bitmap[byte_index] >> bit_index) & 0x01;


            for (uint8_t sy = 0; sy < scale; sy++)
            {
                for (uint8_t sx = 0; sx < scale; sx++)
                {
                    SSD1306_DrawPixel(
                        x + i * scale + sx,
                        y + j * scale + sy,
                        pixel
                    );
                }
            }
        }
    }
}


/* =========================================================
 * Gửi framebuffer 1024 byte lên OLED
 * ========================================================= */
void SSD1306_UpdateScreen(void)
{
    /*
     * Column address:
     * 0 -> 127
     */
    SSD1306_WriteCommand(0x21);
    SSD1306_WriteCommand(0x00);
    SSD1306_WriteCommand(0x7F);


    /*
     * Page address:
     * 0 -> 7
     */
    SSD1306_WriteCommand(0x22);
    SSD1306_WriteCommand(0x00);
    SSD1306_WriteCommand(0x07);


    /*
     * Gửi từng block 16 byte.
     *
     * Byte 0 = 0x40:
     * báo cho SSD1306 rằng phần sau là DATA.
     */
    uint8_t packet[17];


    packet[0] = 0x40;


    for (uint16_t i = 0;
         i < sizeof(SSD1306_Buffer);
         i += 16)
    {
        memcpy(
            &packet[1],
            &SSD1306_Buffer[i],
            16
        );


        HAL_I2C_Master_Transmit(
            ssd1306_i2c,
            ssd1306_address,
            packet,
            17,
            HAL_MAX_DELAY
        );
    }
}

