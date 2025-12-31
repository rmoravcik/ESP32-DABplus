#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <JPEGDecoder.h>
#include <PNGdec.h>

enum bt_indicator_state {
  BT_INDICATOR_STATE_DISCONNECTED = 0,
  BT_INDICATOR_STATE_CONNECTING,
  BT_INDICATOR_STATE_CONNECTED
};

struct list_entry {
  char *text;
  char *icon;
  bool is_selected;
};

class Display
{
public:
  Display();
  virtual ~Display();

  void update();
  SPIClass* getSPIinstance();

  bool getTouch(uint16_t *x, uint16_t *y);

  void drawReceivingScreen();
  void drawScanningScreen(uint8_t progress, uint8_t stations);

  void drawTime(uint8_t hour, uint8_t min, bool menuVisible);
  void drawBtIndicator(bt_indicator_state state, bool menuVisible);
  void drawSignalIndicator(int8_t strength, bool menuVisible);
  void drawStationLabel(String label, bool menuVisible);
  void drawSlideShow(uint8_t* data, uint32_t size, TFT_eSprite *sprite = NULL);
  void drawRdsText(String text);
  void drawMainMenu(void);
  void drawMainMenuVolumeDown(uint16_t volume);
  void drawMainMenuVolumeUp(uint16_t volume);
  void drawRadioMenu(void);
  void drawListMenu(void);
  void drawListMenuEntries(struct list_entry *entries, const uint8_t page, const uint8_t pages);

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

  void drawControls(TFT_eSprite *sprite = NULL);

  bool isJpegFile(uint8_t* data, uint32_t size);
  void renderPng(const char *filename, int x, int y, TFT_eSprite *sprite = NULL);
  void renderPng(uint8_t* data, uint32_t size, int x, int y, TFT_eSprite *sprite = NULL);
  void renderJpeg(uint8_t* data, uint32_t size, TFT_eSprite *sprite = NULL);
  
  void jpegInfo();

  static void* pngOpen(const char *filename, int32_t *size);
  static void pngClose(void *handle);
  static int32_t pngRead(PNGFILE *handle, uint8_t *buffer, int32_t length);
  static int32_t pngSeek(PNGFILE *handle, int32_t position);
  static void pngDraw(PNGDRAW *pDraw);

  static int m_ImagePosX;
  static int m_ImagePosY;
  static TFT_eSprite *m_ImageSprite;
};

#endif