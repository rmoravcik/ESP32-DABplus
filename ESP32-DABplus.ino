#include <stdint.h>
#include <LittleFS.h>

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

volatile bool penDown = false;

struct StationInfo
{
  uint8_t freqIndex;
  uint32_t serviceId;
  String label;
};

StationInfo stationList[100];
uint8_t stationCount = 0;

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
  attachInterrupt(digitalPinToInterrupt(GPIO_TOUCH_IRQ), penIrq, FALLING);
}

void rdsTextUpdated(String text)
{
  m_display->drawRdsText(text);
}

void stationFound(uint8_t freqIndex, uint32_t serviceId, String label)
{
  stationList[stationCount].freqIndex = freqIndex;
  stationList[stationCount].serviceId = serviceId;
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
      String label;

      int pos = line.indexOf(';');
      freqIndex = line.substring(0, pos - 1).toInt();
      line.remove(0, pos + 1);

      pos = line.indexOf(';');
      serviceId = line.substring(0, pos - 1).toInt();
      line.remove(0, pos + 1);

      pos = line.indexOf(';');
      label = line.substring(0, pos - 1);
      label.trim();

      Serial.print("i=");
      Serial.print(stationCount);
      Serial.print(" ");
      Serial.print(freqIndex);
      Serial.print(" ");
      Serial.print(serviceId);
      Serial.print(" ");
      Serial.println(label);

      stationList[stationCount].freqIndex = freqIndex;
      stationList[stationCount].serviceId = serviceId;
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
    file.printf("%u;%u;%s;\n", stationList[i].freqIndex, stationList[i].serviceId, stationList[i].label.c_str());
  }

  file.close();
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
  m_radio->setRdsTextUpdatedCallack(rdsTextUpdated);
  m_radio->setStationFoundCallack(stationFound);

  loadStationList();
  // m_radio->scan();
  // saveStationList();

  for (uint8_t i = 0; i < stationCount; i++)
  {
    // if (stationList[i].label == "Radio Relax")
    if (i == 50)
    {
      Serial.print("Tunning to station ");
      Serial.println(stationList[i].label);
      m_radio->tuneStation(stationList[i].freqIndex, stationList[i].serviceId);
      m_display->drawStationLabel(stationList[i].label);
      break;
    }
  }
}

void penIrq()
{
  penDown = true;
}

void loop()
{
  uint8_t hour = 0, min = lastMin;
  unsigned long curMillis = millis();

  m_btaudio->update();
  m_btscanner->update();
  m_display->update();
  m_radio->update();

  // m_btscanner->printAvailable();

  if (penDown)
  {
    uint16_t x;
    uint16_t y;

    m_display->getTouch(&x, &y);

    Serial.print("x=");
    Serial.print(x);
    Serial.print(" y=");
    Serial.println(y);

    penDown = false;
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
