#include <string.h>
#include <LittleFS.h>
#include "ESP32-DABplus.h"
#include "display.h"

TFT_eSPI *Display::m_tft = NULL;

TFT_eSprite *Display::m_serviceDataSprite = NULL;
TFT_eSprite *Display::m_statusBarSprite = NULL;

fs::File file;
PNG png;

#define SCROLLING_TEXT_SPACING 40
#define MAX_NOT_WRAPPED_SCROLING_TEXT_SPACING 20

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
  m_statusBarSprite->setTextColor(TFT_WHITE, TFT_BLACK);
  m_statusBarSprite->setTextSize(1);
  m_statusBarSprite->loadFont("Roboto-Regular15", LittleFS);

//  m_slideshowSprite = new TFT_eSprite(m_tft);
//  m_slideshowSprite->createSprite(320, 240);

  m_serviceDataSprite = new TFT_eSprite(m_tft);
  m_serviceDataSprite->createSprite(320, 30);
  m_serviceDataSprite->setTextColor(TFT_WHITE, TFT_BLACK);
  m_serviceDataSprite->setTextSize(1);
  m_serviceDataSprite->setColorDepth(8);
  m_serviceDataSprite->setScrollRect(0, 0, 320, 30);
  m_serviceDataSprite->loadFont("Roboto-Regular20", LittleFS);

#if 0
  m_tft->fillScreen((0xFFFF));
  m_tft->setCursor(20, 0, 2);
  m_tft->setTextColor(TFT_BLACK, TFT_WHITE);  m_tft->setTextSize(1);
  m_tft->println("calibration run");

  m_tft->calibrateTouch(m_calibrationData, TFT_WHITE, TFT_RED, 15);
  Serial.print("m_calibrationData: ");
  Serial.print(m_calibrationData[0]);
  Serial.print(" ");
  Serial.print(m_calibrationData[1]);
  Serial.print(" ");
  Serial.print(m_calibrationData[2]);
  Serial.print(" ");
  Serial.print(m_calibrationData[3]);
  Serial.print(" ");
  Serial.println(m_calibrationData[4]);
#endif

  drawTime(12, 0);
  drawSlideShow(true);
  drawRdsText(m_welcomeText);
  drawSignalIndicator(0);
}

Display::~Display()
{

}

void Display::getTouch(uint16_t *x, uint16_t *y)
{
  if (m_tft)
  {
    m_tft->getTouch(x, y);
  }
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

void Display::drawTime(uint8_t hour, uint8_t min)
{
  char time[6];

  sprintf(time, "%2u:%02u", hour, min);

  Serial.print("Time: ");
  Serial.println(time);

  m_statusBarSprite->fillRect(0, 0, 60, 25, TFT_BLACK);
  m_statusBarSprite->setTextDatum(TL_DATUM);
  m_statusBarSprite->drawString(time, 5, 5);
  m_statusBarSprite->pushSprite(0, 0);
}

void Display::drawSignalIndicator(int8_t strength)
{
  uint16_t x = 451, y = 13;
  uint8_t level = 0; // 0..5

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
}

void Display::drawStationLabel(String label)
{
  if (label.length() == 0)
  {
    return;
  }

  m_statusBarSprite->fillRect(80, 0, 240, 25, TFT_BLACK);
  m_statusBarSprite->setTextDatum(TC_DATUM);
  m_statusBarSprite->drawString(label, 240, 5);
  m_statusBarSprite->pushSprite(0, 0);
}

void Display::drawSlideShow(bool logo)
{
  if (logo)
  {
    int ret = png.open((const char *)"/dab_logo.png", pngOpen, pngClose, pngRead, pngSeek, pngDraw);
    if (ret == PNG_SUCCESS)
    {
      ret = png.decode(NULL, 0);
      png.close();
    }
  }
  else
  {

  }
//  m_slideshowSprite->pushSprite(80, 30);
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

  m_serviceDataSprite->setCursor(0 - offset, 5);
  m_serviceDataSprite->print(text.c_str());

  if (rdsTextWidth > m_serviceDataSprite->width())
  {
    m_serviceDataSprite->setCursor(0 + rdsTextWidth  + SCROLLING_TEXT_SPACING - offset, 5);
    m_serviceDataSprite->print(text.c_str());
  }
  
  m_serviceDataSprite->pushSprite(80, 280);

  return rdsTextWidth;
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
  m_tft->pushImage(80, 25 + pDraw->y, pDraw->iWidth, 1, usPixels);
}
