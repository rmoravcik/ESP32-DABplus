#include <string.h>
#include <LittleFS.h>
#include "ESP32-DABplus.h"
#include "display.h"

// #define SLIDESHOW_DEBUG

TFT_eSPI *Display::m_tft = NULL;

TFT_eSprite *Display::m_serviceDataSprite = NULL;
TFT_eSprite *Display::m_statusBarSprite = NULL;

int Display::m_ImagePosX = 0;
int Display::m_ImagePosY = 0;
TFT_eSprite *Display::m_ImageSprite = NULL;

fs::File file;
PNG png;

#define SCROLLING_TEXT_SPACING 40
#define MAX_NOT_WRAPPED_SCROLING_TEXT_SPACING 20

#define STATUS_BAR_BG_COLOR 0x2104

#define minimum(a,b)     (((a) < (b)) ? (a) : (b))

Display::Display()
{
  Serial.println("Initializing display");

  m_calibrationData[0] = 328;
  m_calibrationData[1] = 3504;
  m_calibrationData[2] = 348;
  m_calibrationData[3] = 3233;
  m_calibrationData[4] = 3;

  m_rdsText = "";
  m_rdsTextWidth = 0;
  m_rdsTextOffset = 0;

  m_tft = new TFT_eSPI(TFT_WIDTH, TFT_HEIGHT);
  m_tft->begin();
  m_tft->setRotation(1);
  m_tft->fillScreen(TFT_BLACK);
  m_tft->setTextColor(TFT_WHITE, TFT_BLACK);
  m_tft->setTextSize(1);
  m_tft->setTouch(m_calibrationData);

  m_statusBarSprite = new TFT_eSprite(m_tft);
  m_statusBarSprite->createSprite(480, 25);
  m_statusBarSprite->setColorDepth(16);
  m_statusBarSprite->fillSprite(STATUS_BAR_BG_COLOR);
  m_statusBarSprite->setTextColor(TFT_WHITE, STATUS_BAR_BG_COLOR);
  m_statusBarSprite->setTextSize(1);
  m_statusBarSprite->loadFont("Roboto-Regular15", LittleFS);

  m_serviceDataSprite = new TFT_eSprite(m_tft);
  m_serviceDataSprite->createSprite(320, 30);
  m_serviceDataSprite->setTextColor(TFT_WHITE, TFT_BLACK);
  m_serviceDataSprite->setTextSize(1);
  m_serviceDataSprite->setColorDepth(8);
  m_serviceDataSprite->setScrollRect(0, 0, 320, 30);
  m_serviceDataSprite->loadFont("Roboto-Regular20", LittleFS);

  drawTime(12, 0, false);
  drawSlideShow(NULL, 0);
  drawRdsText(m_welcomeText);
  drawBtIndicator(BT_INDICATOR_STATE_DISCONNECTED, false);
  drawSignalIndicator(0, false);
  drawControls();
}

Display::~Display()
{
}

bool Display::getTouch(uint16_t *x, uint16_t *y)
{
  if (m_tft)
  {
    return m_tft->getTouch(x, y, 300);
  }

  return false;
}

void Display::update()
{
  if (m_rdsTextWidth > m_serviceDataSprite->width())
  {
    drawRdsText(m_rdsText, m_rdsTextOffset++);
    if (m_rdsTextOffset >= (m_rdsTextWidth + SCROLLING_TEXT_SPACING))
    {
      m_rdsTextOffset = 0;
    }
  }
  else if ((m_rdsTextWidth + MAX_NOT_WRAPPED_SCROLING_TEXT_SPACING) < m_serviceDataSprite->width())
  {  
    if (m_rdsTextScrollLeft)
    {
      if (m_rdsTextOffset >= 0)
      {
        m_rdsTextScrollLeft = false;
        m_rdsTextOffset = 0;
      }
      else
      {
        m_rdsTextOffset++;
      }
    }
    else
    {
      if ((m_rdsTextWidth - m_rdsTextOffset) >= m_serviceDataSprite->width())
      {
        m_rdsTextScrollLeft = true;
        m_rdsTextOffset++;
      }
      else
      {
        m_rdsTextOffset--;
      }
    }

    drawRdsText(m_rdsText, m_rdsTextOffset);
  }
}

SPIClass* Display::getSPIinstance()
{
  return &m_tft->getSPIinstance();
}

void Display::drawReceivingScreen()
{
  TFT_eSprite screen = TFT_eSprite(m_tft);
  screen.createSprite(480, 320);

  screen.fillScreen(TFT_BLACK);
  drawControls(&screen);
  drawSlideShow(NULL, 0, &screen);
  m_statusBarSprite->pushToSprite(&screen, 0, 0);

  screen.pushSprite(0, 0);
  screen.deleteSprite();
}

void Display::drawScanningScreen(uint8_t progress, uint8_t stations)
{
  Serial.print("drawScanningScreen progress=");
  Serial.println(progress);

  if (progress == 0)
  {
    TFT_eSprite screen = TFT_eSprite(m_tft);
    screen.createSprite(480, 320);

    screen.fillScreen(TFT_BLACK);
    screen.fillRoundRect(40, 105, 400, 110, 10, TFT_WHITE);
    screen.drawRoundRect(40, 105, 400, 110, 10, TFT_DARKGREY);
    screen.drawRoundRect(60, 168, 360, 24, 4, TFT_BLACK);

    screen.setTextColor(TFT_BLACK, TFT_WHITE);
    screen.setTextSize(1);
    screen.loadFont("Roboto-Regular20", LittleFS);
    screen.setTextDatum(TC_DATUM);
    screen.drawString("Vyhledávam stanice", 240, 130);
    screen.unloadFont();

    screen.pushSprite(0, 0);
    screen.deleteSprite();
  }
  else
  {
    m_tft->fillRoundRect(60, 168, (progress * 360) / 100, 24, 4, TFT_DARKGREY);
  }
}

void Display::drawTime(uint8_t hour, uint8_t min, bool menuVisible)
{
  char time[6];

  sprintf(time, "%2u:%02u", hour, min);

//  Serial.print("Time: ");
//  Serial.println(time);

  if (menuVisible == true)
  {
    m_tft->setViewport(0, 0, 480, 20);
  }

  m_statusBarSprite->fillRect(0, 0, 60, 25, STATUS_BAR_BG_COLOR);
  m_statusBarSprite->setTextDatum(TL_DATUM);
  m_statusBarSprite->drawString(time, 5, 6);
  m_statusBarSprite->pushSprite(0, 0);

  if (menuVisible == true)
  {
    m_tft->resetViewport();
  }
}

void Display::drawBtIndicator(bt_indicator_state state, bool menuVisible)
{
  uint16_t x = 436, y = 6;
  static uint32_t lastColour = TFT_DARKGREY;
  uint32_t colour = TFT_DARKGREY;

  switch (state)
  {
    case BT_INDICATOR_STATE_CONNECTING:
      if (lastColour == TFT_DARKGREY)
      {
        colour = TFT_WHITE;
      }
      else
      {
        colour = TFT_DARKGREY;
      }
      break;
    case BT_INDICATOR_STATE_CONNECTED:
      colour = TFT_WHITE;
      break;
    case BT_INDICATOR_STATE_DISCONNECTED:
    default:
      colour = TFT_DARKGREY;
      break;
  }

  lastColour = colour;

  if (menuVisible == true)
  {
    m_tft->setViewport(0, 0, 480, 20);
  }

  m_statusBarSprite->fillRect(x + 3, y, 2, 11, colour);
  m_statusBarSprite->fillRect(x, y + 2, 2, 2, colour);
  m_statusBarSprite->fillRect(x, y + 7, 2, 2, colour);
  m_statusBarSprite->fillRect(x + 6, y + 2, 2, 2, colour);
  m_statusBarSprite->fillRect(x + 6, y + 7, 2, 2, colour);
  m_statusBarSprite->fillRect(x + 2, y + 3, 4, 2, colour);
  m_statusBarSprite->fillRect(x + 2, y + 6, 4, 2, colour);
  m_statusBarSprite->fillRect(x + 5, y, 1, 1, colour);
  m_statusBarSprite->fillRect(x + 5, y + 1, 2, 1, colour);
  m_statusBarSprite->fillRect(x + 5, y + 9, 2, 1, colour);
  m_statusBarSprite->fillRect(x + 5, y + 10, 1, 1, colour);
  m_statusBarSprite->pushSprite(0, 0);

  if (menuVisible == true)
  {
    m_tft->resetViewport();
  }
}


void Display::drawSignalIndicator(int8_t strength, bool menuVisible)
{
  uint16_t x = 451, y = 14;
  uint8_t level = 0; // 0..5

//  Serial.printf("strength=%d\n", strength);

  if (strength < 10)
  {
    level = 1;
  }
  else if (strength < 20)
  {
    level = 2;
  }
  else if (strength < 30)
  {
    level = 3;
  }
  else if (strength < 40)
  {
    level = 4;
  }
    else if (strength >= 40)
  {
    level = 5;
  }

  if (menuVisible == true)
  {
    m_tft->setViewport(0, 0, 480, 20);
  }

  for (uint8_t i = 0; i < 5; i++)
  {
    if (i < level)
    {
      m_statusBarSprite->fillRect(x + (5 *i), y - (2 * i), 3, 3 + (2 * i), TFT_WHITE);
    }
    else
    {
      m_statusBarSprite->fillRect(x + (5 *i), y - (2 * i), 3, 3 + (2 * i), TFT_DARKGREY);
    }
  }

  m_statusBarSprite->pushSprite(0, 0);

  if (menuVisible == true)
  {
    m_tft->resetViewport();
  }
}

void Display::drawStationLabel(String label, bool menuVisible)
{
  if (label.length() == 0)
  {
    return;
  }

  if (menuVisible == false)
  {
    // Reset slideshow and rds text
    drawSlideShow(NULL, 0);
    drawRdsText(m_welcomeText);
  }

  if (menuVisible == true)
  {
    m_tft->setViewport(0, 0, 480, 20);
  }

  m_statusBarSprite->fillRect(80, 0, 240, 25, STATUS_BAR_BG_COLOR);
  m_statusBarSprite->setTextDatum(TC_DATUM);
  m_statusBarSprite->drawString(label, 240, 6);
  m_statusBarSprite->pushSprite(0, 0);

  if (menuVisible == true)
  {
    m_tft->resetViewport();
  }
}

void Display::drawSlideShow(uint8_t* data, uint32_t size, TFT_eSprite *sprite)
{
  if ((data != NULL) && (size > 0))
  {
    if (isJpegFile(data, size))
    {
      renderJpeg(data, size, sprite);
    } 
    else 
    {
      renderPng(data, size, 80, 40, sprite);
    }
  }
  else
  {
    renderPng((const char *)"/dab_logo.png", 80, 40, sprite);
  }
}

void Display::drawRdsText(String text)
{
  if (text.isEmpty() || m_rdsText.equals(text))
  {
    return;
  }

  m_rdsTextOffset = 0;
  m_rdsText = text;
  m_rdsTextScrollLeft = true;

  m_rdsTextWidth = drawRdsText(m_rdsText, m_rdsTextOffset);
}

int32_t Display::drawRdsText(String text, uint16_t offset)
{
  int32_t rdsTextWidth = 0;
 
  m_serviceDataSprite->fillRect(0, 0, 320, 30, TFT_BLACK);
  m_serviceDataSprite->setTextDatum(TL_DATUM);
  m_serviceDataSprite->setTextWrap(false);
  
  rdsTextWidth = m_serviceDataSprite->textWidth(text.c_str());

  m_serviceDataSprite->setCursor(0 - offset, 4);
  m_serviceDataSprite->print(text.c_str());

  if (rdsTextWidth > m_serviceDataSprite->width())
  {
    m_serviceDataSprite->setCursor(0 + rdsTextWidth  + SCROLLING_TEXT_SPACING - offset, 5);
    m_serviceDataSprite->print(text.c_str());
  }
  
  m_serviceDataSprite->pushSprite(80, 290);

  return rdsTextWidth;
}

void Display::drawMainMenu()
{
  TFT_eSprite menu = TFT_eSprite(m_tft);
  menu.createSprite(400, 280);

  menu.fillRoundRect(0, 0, 400, 280, 10, TFT_WHITE);
  menu.drawRoundRect(0, 0, 400, 280, 10, TFT_DARKGREY);
  menu.drawLine(10, 0, 390, 0, TFT_DARKGREY);
  menu.drawLine(10, 70, 390, 70, TFT_DARKGREY);
  menu.drawLine(10, 140, 390, 140, TFT_DARKGREY);
  menu.drawLine(10, 210, 390, 210, TFT_DARKGREY);

  renderPng((const char *)"/radio.png", 20, 23, &menu);
  renderPng((const char *)"/bluetooth.png", 20, 93, &menu);
  renderPng((const char *)"/volume.png", 20, 163, &menu);
  renderPng((const char *)"/back.png", 20, 233, &menu);

  menu.setTextColor(TFT_BLACK, TFT_WHITE);
  menu.setTextSize(1);
  menu.loadFont("Roboto-Regular20", LittleFS);
  menu.setTextDatum(TL_DATUM);

  menu.drawString("Stanice", 54, 27);
  menu.drawString("Připojit", 54, 97);
  menu.drawString("Hlasitost", 54, 167);
  menu.drawString("Zpět", 54, 237);

  menu.unloadFont();

  menu.pushSprite(40, 20);
  menu.deleteSprite();
}

void Display::drawMainMenuVolumeDown(uint16_t volume)
{
  TFT_eSprite menu = TFT_eSprite(m_tft);
  menu.createSprite(51, 51);

  menu.fillRect(0, 0, 51, 51, TFT_WHITE);

  if (volume > VOLUME_MIN)
  {
    menu.fillCircle(25, 25, 25, TFT_DARKGREY);
    menu.fillRect(12, 23, 26, 4, TFT_WHITE);
  }
  else
  {
    menu.fillCircle(25, 25, 25, TFT_LIGHTGREY);
    menu.fillRect(12, 23, 26, 4, TFT_WHITE);
  }

  menu.pushSprite(300, 170);
  menu.deleteSprite();
}

void Display::drawMainMenuVolumeUp(uint16_t volume)
{
  TFT_eSprite menu = TFT_eSprite(m_tft);
  menu.createSprite(51, 51);

  menu.fillRect(0, 0, 51, 51, TFT_WHITE);

  if (volume < VOLUME_MAX)
  {
    menu.fillCircle(25, 25, 25, TFT_DARKGREY);
    menu.fillRect(23, 12, 4, 26, TFT_WHITE);
    menu.fillRect(12, 23, 26, 4, TFT_WHITE);
  }
  else
  {
    menu.fillCircle(25, 25, 25, TFT_LIGHTGREY);
    menu.fillRect(23, 12, 4, 26, TFT_WHITE);
    menu.fillRect(12, 23, 26, 4, TFT_WHITE);
  }

  menu.pushSprite(370, 170);
  menu.deleteSprite();
}

void Display::drawListMenu(void)
{
  TFT_eSprite menu = TFT_eSprite(m_tft);
  menu.createSprite(400, 280);

  menu.fillRoundRect(0, 0, 400, 280, 10, TFT_WHITE);
  menu.drawRoundRect(0, 0, 400, 280, 10, TFT_DARKGREY);

  menu.drawRect(10, 10, 380, 210, TFT_DARKGREY);
  menu.fillRoundRect(363, 54, 10, 122, 5, TFT_BLACK);
  renderPng((const char *)"/up.png", 356, 20, &menu);
  renderPng((const char *)"/down.png", 356, 186, &menu);
  renderPng((const char *)"/back.png", 20, 233, &menu);

  menu.setTextColor(TFT_BLACK, TFT_WHITE);
  menu.setTextSize(1);
  menu.loadFont("Roboto-Regular20", LittleFS);
  menu.setTextDatum(TL_DATUM);

  menu.drawString("Zpět", 54, 237);

  menu.unloadFont();

  menu.pushSprite(40, 20);
  menu.deleteSprite();
}

void Display::drawListMenuEntries(struct list_entry *entries, const uint8_t page, const uint8_t pages)
{
  TFT_eSprite menu = TFT_eSprite(m_tft);
  menu.createSprite(380, 210);

  uint32_t scrollbarHeight = 122 / pages;

  menu.fillRect(0, 0, 380, 210, TFT_WHITE);
  menu.drawRect(0, 0, 380, 210, TFT_DARKGREY);

  menu.fillRoundRect(353, 44 + (page * scrollbarHeight), 10, scrollbarHeight, 5, TFT_BLACK);
  renderPng((const char *)"/up.png", 346, 10, &menu);
  renderPng((const char *)"/down.png", 346, 176, &menu);

  menu.setTextColor(TFT_BLACK, TFT_WHITE);
  menu.setTextSize(1);
  menu.loadFont("Roboto-Regular20", LittleFS);
  menu.setTextDatum(TL_DATUM);

  if (entries[3 * page + 0].text)
  {
    renderPng((const char *)entries[3 * page + 0].icon, 10, 23, &menu);
    menu.drawString(entries[3 * page + 0].text, 44, 27);
    menu.drawLine(10, 70, 326, 70, TFT_DARKGREY);
    if (entries[3 * page + 0].is_selected)
    {
      renderPng((const char *)"/ok.png", 292, 23, &menu);
    }
  }

  if (entries[3 * page + 1].text)
  {
    renderPng((const char *)entries[3 * page + 1].icon, 10, 93, &menu);
    menu.drawString(entries[3 * page + 1].text, 44, 97);
    menu.drawLine(10, 140, 326, 140, TFT_DARKGREY);
    if (entries[3 * page + 1].is_selected)
    {
      renderPng((const char *)"/ok.png", 292, 93, &menu);
    }
  }

  if (entries[3 * page + 2].text)
  {
    renderPng((const char *)entries[3 * page + 2].icon, 10, 163, &menu);
    menu.drawString(entries[3 * page + 2].text, 44, 167);
    if (entries[3 * page + 2].is_selected)
    {
      renderPng((const char *)"/ok.png", 292, 163, &menu);
    }
  }

  menu.unloadFont();

  menu.pushSprite(50, 30);
  menu.deleteSprite();
}

void Display::drawRadioMenu()
{
  TFT_eSprite menu = TFT_eSprite(m_tft);
  menu.createSprite(400, 280);

  menu.fillRoundRect(0, 0, 400, 280, 10, TFT_WHITE);
  menu.drawRoundRect(0, 0, 400, 280, 10, TFT_DARKGREY);
  menu.drawLine(10, 0, 390, 0, TFT_DARKGREY);
  menu.drawLine(10, 70, 390, 70, TFT_DARKGREY);
  menu.drawLine(10, 140, 390, 140, TFT_DARKGREY);
  menu.drawLine(10, 210, 390, 210, TFT_DARKGREY);

  renderPng((const char *)"/list.png", 20, 23, &menu);
  renderPng((const char *)"/search.png", 20, 93, &menu);
  renderPng((const char *)"/back.png", 20, 233, &menu);

  menu.setTextColor(TFT_BLACK, TFT_WHITE);
  menu.setTextSize(1);
  menu.loadFont("Roboto-Regular20", LittleFS);
  menu.setTextDatum(TL_DATUM);

  menu.drawString("Seznam", 54, 27);
  menu.drawString("Vyhledat", 54, 97);
  menu.drawString("Zpět", 54, 237);

  menu.unloadFont();

  menu.pushSprite(40, 20);
  menu.deleteSprite();
}

void Display::drawControls(TFT_eSprite *sprite)
{
  if (sprite)
  {
    // previous station
    sprite->fillCircle(40, 160, 30, TFT_DARKGREY);
    sprite->fillTriangle(30, 160, 52, 147, 52, 173, TFT_WHITE);
    sprite->fillRect(27, 147, 4, 26, TFT_WHITE);

    // next station
    sprite->fillCircle(440, 160, 30, TFT_DARKGREY);
    sprite->fillTriangle(450, 160, 428, 147, 428, 173, TFT_WHITE);
    sprite->fillRect(450, 147, 4, 26, TFT_WHITE);

    // menu
    sprite->fillRect(27, 50, 25, 3, TFT_WHITE);
    sprite->fillRect(27, 58, 25, 3, TFT_WHITE);
    sprite->fillRect(27, 66, 25, 3, TFT_WHITE);
  }
  else
  {
    // previous station
    m_tft->fillCircle(40, 160, 30, TFT_DARKGREY);
    m_tft->fillTriangle(30, 160, 52, 147, 52, 173, TFT_WHITE);
    m_tft->fillRect(27, 147, 4, 26, TFT_WHITE);

    // next station
    m_tft->fillCircle(440, 160, 30, TFT_DARKGREY);
    m_tft->fillTriangle(450, 160, 428, 147, 428, 173, TFT_WHITE);
    m_tft->fillRect(450, 147, 4, 26, TFT_WHITE);

    // menu
    m_tft->fillRect(27, 50, 25, 3, TFT_WHITE);
    m_tft->fillRect(27, 58, 25, 3, TFT_WHITE);
    m_tft->fillRect(27, 66, 25, 3, TFT_WHITE);
  }
}

bool Display::isJpegFile(uint8_t* data, uint32_t size)
{
  if ((data[0] == 0xFF) && (data[1] == 0xD8) && (data[2] == 0xFF))
  {
#ifdef SLIDESHOW_DEBUG
    Serial.println("is JPEG");
#endif
    return true;
  }

#ifdef SLIDESHOW_DEBUG
    Serial.println("is PNG");
#endif
  return false;
}

void Display::renderPng(const char *filename, int x, int y, TFT_eSprite *sprite)
{
    m_ImagePosX = x;
    m_ImagePosY = y;
    m_ImageSprite = sprite;

    int ret = png.open(filename, pngOpen, pngClose, pngRead, pngSeek, pngDraw);
    if (ret == PNG_SUCCESS)
    {
      ret = png.decode(NULL, 0);
    }
    png.close();
}

void Display::renderPng(uint8_t* data, uint32_t size, int x, int y, TFT_eSprite *sprite)
{
    m_ImagePosX = x;
    m_ImagePosY = y;
    m_ImageSprite = sprite;

    int ret = png.openRAM(data, size, pngDraw);
    if (ret == PNG_SUCCESS)
    {
      ret = png.decode(NULL, 0);
    }
    png.close();
}

void Display::renderJpeg(uint8_t* data, uint32_t size, TFT_eSprite *sprite)
{
  bool decoded = JpegDec.decodeArray(data, size);
  if (!decoded)
  {
    Serial.println("ERROR: JPEG decoding!");
    return;
  }

  // jpegInfo();

  uint16_t *pImg;
  uint16_t mcu_w = JpegDec.MCUWidth;
  uint16_t mcu_h = JpegDec.MCUHeight;
  uint32_t max_x = JpegDec.width;
  uint32_t max_y = JpegDec.height;
  int xpos = 80;
  int ypos = 40;

  bool swapBytes = m_tft->getSwapBytes();
  if (sprite)
  {
    sprite->setSwapBytes(true);
  }
  else
  {
    m_tft->setSwapBytes(true);
  }
  
  // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
  // Typically these MCUs are 16x16 pixel blocks
  // Determine the width and height of the right and bottom edge image blocks
  uint32_t min_w = jpg_min(mcu_w, max_x % mcu_w);
  uint32_t min_h = jpg_min(mcu_h, max_y % mcu_h);
    
  // save the current image block size
  uint32_t win_w = mcu_w;
  uint32_t win_h = mcu_h;
  
  // save the coordinate of the right and bottom edges to assist image cropping
  // to the screen size
  max_x += xpos;
  max_y += ypos;

  // Fetch data from the file, decode and display
  while (JpegDec.read()) {    // While there is more data in the file
    pImg = JpegDec.pImage ;   // Decode a MCU (Minimum Coding Unit, typically a 8x8 or 16x16 pixel block)

    // Calculate coordinates of top left corner of current MCU
    int mcu_x = JpegDec.MCUx * mcu_w + xpos;
    int mcu_y = JpegDec.MCUy * mcu_h + ypos;

    // check if the image block size needs to be changed for the right edge
    if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
    else win_w = min_w;

    // check if the image block size needs to be changed for the bottom edge
    if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
    else win_h = min_h;

    // copy pixels into a contiguous block
    if (win_w != mcu_w)
    {
      uint16_t *cImg;
      int p = 0;
      cImg = pImg + win_w;
      for (int h = 1; h < win_h; h++)
      {
        p += mcu_w;
        for (int w = 0; w < win_w; w++)
        {
          *cImg = *(pImg + w + p);
          cImg++;
        }
      }
    }

    // calculate how many pixels must be drawn
    uint32_t mcu_pixels = win_w * win_h;

    // draw image MCU block only if it will fit on the screen
    if (( mcu_x + win_w ) <= m_tft->width() && ( mcu_y + win_h ) <= m_tft->height())
    {
      if (sprite)
      {
        sprite->pushImage(mcu_x, mcu_y, win_w, win_h, pImg);
      }
      else
      {
        m_tft->pushImage(mcu_x, mcu_y, win_w, win_h, pImg);
      }
    }
    else if ( (mcu_y + win_h) >= m_tft->height())
      JpegDec.abort(); // Image has run off bottom of screen so abort decoding
  }
  
  if (sprite)
  {
    sprite->setSwapBytes(swapBytes);
  }
  else
  {
    m_tft->setSwapBytes(swapBytes);
  }

  file.close();
}

void Display::jpegInfo()
{
  Serial.println(F("==============="));
  Serial.println(F("JPEG image info"));
  Serial.println(F("==============="));
  Serial.print(F(  "Width      :")); Serial.println(JpegDec.width);
  Serial.print(F(  "Height     :")); Serial.println(JpegDec.height);
  Serial.print(F(  "Components :")); Serial.println(JpegDec.comps);
  Serial.print(F(  "MCU / row  :")); Serial.println(JpegDec.MCUSPerRow);
  Serial.print(F(  "MCU / col  :")); Serial.println(JpegDec.MCUSPerCol);
  Serial.print(F(  "Scan type  :")); Serial.println(JpegDec.scanType);
  Serial.print(F(  "MCU width  :")); Serial.println(JpegDec.MCUWidth);
  Serial.print(F(  "MCU height :")); Serial.println(JpegDec.MCUHeight);
  Serial.println(F("==============="));
}

void* Display::pngOpen(const char *filename, int32_t *size)
{
  file = LittleFS.open(filename, "rb");
  if (!file)
  {
    Serial.print("Failed to open ");
    Serial.println(filename);
    return NULL;
  }
  *size = file.size();
  return &file;
}

void Display::pngClose(void *handle)
{
  if (file)
  {
    file.close();
  }
}

int32_t Display::pngRead(PNGFILE *handle, uint8_t *buffer, int32_t length)
{
  if (!file)
  {
    return 0;
  }

  return file.read(buffer, length);
}

int32_t Display::pngSeek(PNGFILE *handle, int32_t position)
{
  if (!file)
  {
    return 0;
  }
  
  return file.seek(position);
}

void Display::pngDraw(PNGDRAW *pDraw)
{
  uint16_t usPixels[320];

  png.getLineAsRGB565(pDraw, usPixels, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
  if (m_ImageSprite)
  {
    m_ImageSprite->pushImage(m_ImagePosX, m_ImagePosY + pDraw->y, pDraw->iWidth, 1, usPixels);
  }
  else
  {
    m_tft->pushImage(m_ImagePosX, m_ImagePosY + pDraw->y, pDraw->iWidth, 1, usPixels);
  }
}
