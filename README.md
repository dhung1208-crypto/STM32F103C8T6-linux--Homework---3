# STM32F103 Linux Homework - I2C, SPI, DMA

Link video : https://drive.google.com/drive/folders/129RSeuvNvJx8Qyxd3OvkLSIG_CHmDNoe?usp=drive_link

## Thông tin chung

- MCU: STM32F103C8T6
- Thư viện: STM32 HAL
- Môi trường: Ubuntu Linux
- Compiler: `arm-none-eabi-gcc`
- Build: GNU Make
- Nạp chương trình: ST-Link V2 + OpenOCD
- Không sử dụng KeilC

Repository gồm 4 bài tập thực hành về I2C, SPI, UART DMA và ADC DMA.

---

## Bài 1 - I2C + SSD1306

Cấu hình I2C1 giao tiếp với màn hình OLED SSD1306 và hiển thị dữ liệu ảnh đen trắng.

### Kết nối

```text
SSD1306      STM32F103
VCC    ->    3.3V
GND    ->    GND
SCL    ->    PB6
SDA    ->    PB7
```

### Thông số

- I2C1
- Tốc độ: 100 kHz
- OLED SSD1306: 128x64
- Địa chỉ I2C: `0x3C` hoặc `0x3D`

---

## Bài 2 - SPI + MAX7219

Cấu hình SPI1 giao tiếp với MAX7219 để điều khiển LED Matrix 8x8.

### Kết nối

```text
MAX7219      STM32F103
VCC    ->    5V
GND    ->    GND
DIN    ->    PA7
CLK    ->    PA5
CS     ->    PA4
```

### Thông số

- SPI1
- PA5: SCK
- PA7: MOSI
- PA4: CS
- SPI Mode 0
- MSB First

Chương trình hiển thị lần lượt các số từ `0` đến `9` trên LED Matrix 8x8.

---

## Bài 3 - Button + UART + DMA

Cấu hình nút nhấn, UART và DMA.

### Kết nối

```text
Button:
PB12 -> Nút nhấn -> GND

UART:
PA9  -> USART1_TX
PA10 -> USART1_RX
```

### Thông số

- Nút nhấn: PB12
- USART1
- Baudrate: 115200
- USART1_TX sử dụng DMA1 Channel 4

Mỗi lần nhấn nút, giá trị tăng thêm 1 và được gửi lên PC theo định dạng:

```text
<ID-Lớp><ID-Nhóm>:BTN:<Giá trị>
```

Ví dụ:

```text
D23DT04XX:BTN:1
D23DT04XX:BTN:2
D23DT04XX:BTN:3
```

UART truyền dữ liệu bằng DMA:

```c
HAL_UART_Transmit_DMA()
```

---

## Bài 4 - ADC + Timer + DMA

Cấu hình ADC, Timer, DMA và UART.

### Kết nối biến trở

```text
Biến trở      STM32F103
3.3V     ->   Chân ngoài
PA4      ->   Chân giữa
GND      ->   Chân ngoài còn lại
```

### Thông số

- ADC1_IN4: PA4
- Timer: TIM3
- Tần số Timer: 100 Hz
- ADC được trigger bởi Timer
- ADC1 sử dụng DMA1 Channel 1
- DMA hoạt động ở Circular Mode
- Buffer gồm 100 mẫu ADC

DMA sử dụng hai sự kiện:

Nửa buffer đã được ghi xong sẽ được xử lý và truyền dữ liệu lên PC qua UART.

Giá trị ADC nằm trong khoảng: 0 - 4095
