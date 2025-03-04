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
  void drawSignalIndicator(int8_t strength);
  void drawServiceName(const char *name);
  void drawSlideShow(bool logo = false);
  void drawRdsText(String text);

  static TFT_eSPI *m_tft;
  static TFT_eSprite *m_statusBarSprite;
  static TFT_eSprite *m_serviceDataSprite;

 private:
  uint16_t m_calibrationData[5];

  const String m_welcomeText = "Více rádia";

  String m_rdsText;
  int16_t m_rdsTextWidth;
  int16_t m_rdsTextOffset;
  bool m_rdsTextScrollLeft;

  int32_t drawRdsText(String text, uint16_t offset);

  static void* pngOpen(const char *filename, int32_t *size);
  static void pngClose(void *handle);
  static int32_t pngRead(PNGFILE *handle, uint8_t *buffer, int32_t length);
  static int32_t pngSeek(PNGFILE *handle, int32_t position);
  static void pngDraw(PNGDRAW *pDraw);
};

#endif