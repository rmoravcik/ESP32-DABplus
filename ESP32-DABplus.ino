#include <stdint.h>
#include <LittleFS.h>
#include <Preferences.h>

#include "bt_audio.h"
#include "bt_scanner.h"
#include "display.h"
#include "radio.h"

#include "ESP32-DABplus.h"

BtAudio *m_btaudio = NULL;
BtScanner *m_btscanner = NULL;
Display *m_display = NULL;
Radio *m_radio = NULL;

uint8_t lastMin = -1;
unsigned long lastMillis = 0;

struct StationInfo
{
  uint8_t freqIndex;
  uint32_t serviceId;
  uint32_t compId;
  String label;
};

StationInfo stationList[100];
uint8_t stationCount = 0;
uint8_t currentStation = 0;

Preferences preferences;

void init_psram()
{
  // Check if PSRAM is enabled
  if (psramInit()) 
  {
    Serial.println("PSRAM initialized successfully!");
  } 
  else 
  {
    Serial.println("PSRAM initialization failed...");
    while (1); // Stop if PSRAM isn't available
  }

  // Check total available PSRAM
  size_t psramSize = ESP.getPsramSize();
  Serial.print("Total PSRAM: ");
  Serial.println(psramSize);

  // Check available PSRAM
  size_t freePsram = ESP.getFreePsram();
  Serial.print("Free PSRAM: ");
  Serial.println(freePsram);
}

void init_gpio()
{
  pinMode(GPIO_TFT_CS, OUTPUT);
  digitalWrite(GPIO_TFT_CS, HIGH);
      
  pinMode(GPIO_TOUCH_CS, OUTPUT);
  digitalWrite(GPIO_TOUCH_CS, HIGH);
      
  pinMode(GPIO_TFT_RST, OUTPUT);
  digitalWrite(GPIO_TFT_RST, HIGH);

  pinMode(GPIO_SI468X_SSBSI, OUTPUT);
  digitalWrite(GPIO_SI468X_SSBSI, HIGH);

  pinMode(GPIO_SI468X_SSBNV, OUTPUT);
  digitalWrite(GPIO_SI468X_SSBNV, HIGH);

  pinMode(GPIO_TOUCH_IRQ, INPUT_PULLUP);
}

void rdsTextUpdated(String text)
{
  m_display->drawRdsText(text);
}

void slideShowUpdated(void)
{
  m_display->drawSlideShow();
}

void stationFound(uint8_t freqIndex, uint32_t serviceId, uint32_t compId, String label)
{
  stationList[stationCount].freqIndex = freqIndex;
  stationList[stationCount].serviceId = serviceId;
  stationList[stationCount].compId = compId;
  stationList[stationCount].label = label;
  stationCount++;
}

void loadStationList(void)
{
  File file = LittleFS.open("/station_list.conf", "r");
  if (!file)
  {
    Serial.println("Error opening /station_list.conf for reading");
    return;
  }

  while (file.available())
  {
    String line = file.readStringUntil('\n');

    if (line.length())
    {
      uint8_t freqIndex;
      uint32_t serviceId;
      uint32_t compId;
      String label;

      int pos = line.indexOf(';');
      freqIndex = line.substring(0, pos).toInt();
      line.remove(0, pos + 1);

      pos = line.indexOf(';');
      serviceId = line.substring(0, pos).toInt();
      line.remove(0, pos + 1);

      pos = line.indexOf(';');
      compId = line.substring(0, pos).toInt();
      line.remove(0, pos + 1);

      pos = line.indexOf(';');
      label = line.substring(0, pos);
      label.trim();

      stationList[stationCount].freqIndex = freqIndex;
      stationList[stationCount].serviceId = serviceId;
      stationList[stationCount].compId = compId;
      stationList[stationCount].label = label;
      stationCount++;
    }
  }

  file.close();
}

void saveStationList(void)
{
  if (LittleFS.exists("/station_list.conf"))
  {
    LittleFS.remove("/station_list.conf");
  }

  File file = LittleFS.open("/station_list.conf", "w");
  if (!file)
  {
    Serial.println("Error opening /station_list.conf for writing");
    return;
  }

  for (uint8_t i = 0; i < stationCount; i++)
  {
    file.printf("%u;%u;%u;%s;\n", stationList[i].freqIndex, stationList[i].serviceId, stationList[i].compId, stationList[i].label.c_str());
  }

  file.close();
}

void tuneStation(uint8_t index)
{
  Serial.print("Tunning to station ");
  Serial.println(stationList[index].label);
  m_display->drawStationLabel(stationList[index].label);
  m_radio->tuneStation(stationList[index].freqIndex, stationList[index].serviceId, stationList[index].compId);
}

void setup()
{
  Serial.begin(115200);

  init_gpio();
  init_psram();

  LittleFS.begin();

  m_btscanner = new BtScanner();
  m_btaudio = new BtAudio(m_btscanner);
  m_display = new Display();
  m_radio = new Radio(m_display->getSPIinstance());
  m_radio->setRdsTextUpdatedCallback(rdsTextUpdated);
  m_radio->setSlideShowUpdatedCallback(slideShowUpdated);
  m_radio->setStationFoundCallback(stationFound);

  // m_radio->scan();
  // saveStationList();
  // while (1) {}

  loadStationList();

  preferences.begin("ESP32-DABplus", false);
  currentStation = preferences.getUChar("currentStation", 0);

  if (currentStation >= stationCount)
  {
    currentStation = 0;
  }

  tuneStation(currentStation);
}

void loop()
{
  uint8_t hour = 0, min = lastMin;
  unsigned long curMillis = millis();
  uint16_t x;
  uint16_t y;

  m_btaudio->update();
  m_btscanner->update();
  m_display->update();
  m_radio->update();

  // m_btscanner->printAvailable();

  if (m_display->getTouch(&x, &y))
  {
    if (((x > 0) && (x < 80)) && ((y > 120) && (y < 200)))
    {
      if (currentStation > 0)
      {

        currentStation--;
      }
      else
      {
        currentStation = stationCount - 1;
      }
    }
    else if (((x > 400) && (x < 480)) && ((y > 120) && (y < 200)))
    {
      currentStation++;
      if (currentStation >= stationCount)
      {
        currentStation = 0;
      }
    }

    tuneStation(currentStation);
    preferences.putUChar("currentStation", currentStation);
  }

  // one second jobs
  if ((curMillis - lastMillis) >= 1000)
  {
    m_radio->getTime(&hour, &min);
    if (min != lastMin)
    {
      m_display->drawTime(hour, min);
      lastMin = min;
    }

    m_display->drawSignalIndicator(m_radio->getSignalStrength());

    lastMillis = curMillis;
  }
}
