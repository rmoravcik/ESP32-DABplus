#ifndef ESP32_DABPLUS_H
#define ESP32_DABPLUS_H

enum State
{
  STATE_RECEIVING = 0,
  STATE_MAIN_MENU,
  STATE_SCANNING,
  STATE_BLUETOOTH_MENU
};

#define TFT_WIDTH  480
#define TFT_HEIGHT 320

#define GPIO_SI468X_RSTB   2
#define GPIO_TFT_CS        4
#define GPIO_SI468X_SSBSI  5
#define GPIO_TFT_RST      12
#define GPIO_SI468X_SSBNV 13
#define GPIO_SPI_SCLK     18
#define GPIO_SPI_MISO     19
#define GPIO_SPI_MOSI     23
#define GPIO_TFT_DC       25
#define GPIO_TOUCH_CS     27
#define GPIO_TOUCH_IRQ    32
#define GPIO_SI468X_INTB  39

#define SI4684_SPI_FREQ   8000000

#endif