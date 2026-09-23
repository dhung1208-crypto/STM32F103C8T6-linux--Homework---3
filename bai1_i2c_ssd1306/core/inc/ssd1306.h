#ifndef __SSD1306_H
#define __SSD1306_H

#include "stm32f1xx_hal.h"

#define SSD1306_WIDTH   128
#define SSD1306_HEIGHT   64

HAL_StatusTypeDef SSD1306_Init(I2C_HandleTypeDef *hi2c);

void SSD1306_Clear(void);
void SSD1306_UpdateScreen(void);

void SSD1306_DrawPixel(uint8_t x,
                       uint8_t y,
                       uint8_t color);

void SSD1306_DrawBitmap(uint8_t x,
                        uint8_t y,
                        const uint8_t *bitmap,
                        uint8_t width,
                        uint8_t height);

void SSD1306_DrawBitmapScaled(uint8_t x,
                              uint8_t y,
                              const uint8_t *bitmap,
                              uint8_t width,
                              uint8_t height,
                              uint8_t scale);

#endif
