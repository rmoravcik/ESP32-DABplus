#ifndef DISPLAY_H
#define DISPLAY_H

#include <PNGdec.h>
#include <TFT_eSPI.h>
#include <Arduino.h>

class Display
{
public:
  Display();
  virtual ~Display();

  void update();
  SPIClass* getSPIinstance();

  void getTouch(uint16_t *x, uint16_t *y);

  void drawTime(uint8_t hour, uint8_t min);
  void drawSlideShow(bool logo = false);
  void drawServiceData(const char *data);

  static TFT_eSPI *m_tft;
  static TFT_eSprite *m_statusSprite;
//  static TFT_eSprite *m_slideshowSprite;
  static TFT_eSprite *m_serviceDataSprite;

 private:
  uint16_t m_calibrationData[5];

  static void* pngOpen(const char *filename, int32_t *size);
  static void pngClose(void *handle);
  static int32_t pngRead(PNGFILE *handle, uint8_t *buffer, int32_t length);
  static int32_t pngSeek(PNGFILE *handle, int32_t position);
  static void pngDraw(PNGDRAW *pDraw);
};

#endif