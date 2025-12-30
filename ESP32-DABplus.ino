#include <stdint.h>
#include <LittleFS.h>
#include <Preferences.h>

#include "bt_audio.h"
#include "bt_scanner.h"
#include "display.h"
#include "radio.h"

#include "ESP32-DABplus.h"

State m_state = STATE_RECEIVING;

BtAudio *m_btaudio = NULL;
BtScanner *m_btscanner = NULL;
Display *m_display = NULL;
Radio *m_radio = NULL;

uint8_t lastMin = -1;
#ifdef DEBUG_HEAP
unsigned long lastMillisMain = 0;
#endif
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

uint8_t volume = 25;

Preferences preferences;

void init_psram()
{
  // Check if PSRAM is enabled
  if (psramInit()) 
  {
    Serial.println(F("PSRAM initialized successfully!"));
  } 
  else 
  {
    Serial.println(F("PSRAM initialization failed..."));
    while (1); // Stop if PSRAM isn't available
  }
}

void check_mem_usage()
{
  uint32_t freeHeap = ESP.getFreeHeap();
  uint32_t freePsram = ESP.getFreePsram();
  Serial.print(F("Free HEAP: "));
  Serial.print(freeHeap);
  Serial.print(F(", PSRAM: "));
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

void slideShowUpdated(uint8_t* data, uint32_t size)
{
  m_display->drawSlideShow(data, size);
}

void stationFound(uint8_t freqIndex, uint32_t serviceId, uint32_t compId, String label)
{
  stationList[stationCount].freqIndex = freqIndex;
  stationList[stationCount].serviceId = serviceId;
  stationList[stationCount].compId = compId;
  stationList[stationCount].label = label;
  stationCount++;

  m_display->drawScanningScreen(freqIndex * 100/DAB_FREQS, stationCount);
}

void loadStationList(void)
{
  File file = LittleFS.open("/station_list.conf", "r");
  if (!file)
  {
    Serial.println(F("Error opening /station_list.conf for reading"));
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
    Serial.println(F("Error opening /station_list.conf for writing"));
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
  Serial.print(F("Tunning to station "));
  Serial.println(stationList[index].label);
  m_display->drawStationLabel(stationList[index].label);
  m_radio->tuneStation(stationList[index].freqIndex, stationList[index].serviceId, stationList[index].compId);
}

void setup()
{
  Serial.begin(115200);
  check_mem_usage();

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

  loadStationList();

  preferences.begin("ESP32-DABplus", false);
  currentStation = preferences.getUChar("currentStation", 0);
  volume = preferences.getUChar("volume", 25);

  if (currentStation >= stationCount)
  {
    currentStation = 0;
  }

  tuneStation(currentStation);
  m_radio->setVolume(volume);
  m_btaudio->setVolume(volume);
}

void state_receiving()
{
  uint8_t hour = 0, min = lastMin;
  unsigned long curMillis = millis();
  uint16_t x;
  uint16_t y;

  m_btaudio->update();
  m_display->update();
  m_radio->update();

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
      tuneStation(currentStation);
      preferences.putUChar("currentStation", currentStation);
    }
    else if (((x > 400) && (x < 480)) && ((y > 120) && (y < 200)))
    {
      currentStation++;
      if (currentStation >= stationCount)
      {
        currentStation = 0;
      }
      tuneStation(currentStation);
      preferences.putUChar("currentStation", currentStation);
    }
    else if (((x >10) && (x < 70)) && ((y > 30) && (y < 90)))
    {
      m_display->drawMainMenu();
      m_state = STATE_MAIN_MENU;
    }
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

    m_display->drawBtIndicator((bt_indicator_state)m_btaudio->getState());
    m_display->drawSignalIndicator(m_radio->getSignalStrength());

    lastMillis = curMillis;
  }
}
      
void state_main_menu()
{
  uint16_t x;
  uint16_t y;

  m_btaudio->update();
  m_btscanner->update();

  m_display->drawMainMenuVolumeDown(volume);
  m_display->drawMainMenuVolumeUp(volume);

  if (m_display->getTouch(&x, &y))
  {
    if ((x > 40) && (x < 440))
    {
      if ((y > 20) && (y < 90))
      {
        // scan radio stations
        m_state = STATE_SCANNING;
      }
      else if ((y > 90) && (y < 160))
      {
        // connect to bluetooth
        m_display->drawBluetoothMenu();
        m_state = STATE_BLUETOOTH_MENU;
      }
      else if ((y > 160) && (y < 230))
      {
        // volume down
        if ((x > 300) && (x < 350))
        {
          if (volume > VOLUME_MIN)
          {
            volume--;
            m_radio->setVolume(volume);
            m_btaudio->setVolume(volume);
            preferences.putUChar("volume", volume);
          }
        }
        // volume up
        else if ((x > 370) && (x < 420))
        {
          if (volume < VOLUME_MAX)
          {
            volume++;
            m_radio->setVolume(volume);
            m_btaudio->setVolume(volume);
            preferences.putUChar("volume", volume);
          }
        }
      }
      else if ((y > 230) && (y < 300))
      {
        // exit
        m_display->drawReceivingScreen();
        m_state = STATE_RECEIVING;
      }
    }
  }
}

void state_scanning()
{
  stationCount = 0;

  m_display->drawScanningScreen(0, stationCount);
  m_radio->scan();
  m_display->drawScanningScreen(100, stationCount);
  saveStationList();

  m_display->drawReceivingScreen();

  currentStation = 0;
  if (stationCount > 0)
  {
    tuneStation(currentStation);
    preferences.putUChar("currentStation", currentStation);
  }

  m_state = STATE_RECEIVING;
}

void state_bluetooth_menu()
{
  unsigned long curMillis = millis();
  uint16_t x;
  uint16_t y;

  m_btaudio->update();
  m_btscanner->update();

  if (m_display->getTouch(&x, &y))
  {
    if ((x > 40) && (x < 440))
    {
      if ((y > 10) && (y < 220))
      {
        if (m_btaudio->getState() == BT_AUDIO_STATE_DISCONNECTED)
        {
          m_btaudio->connectTo("BEACHBOOM");
        }
        else
        {
          m_btaudio->disconnect();
        }
      }
      else if ((y > 230) && (y < 300))
      {
        // back
        m_display->drawMainMenu();
        m_state = STATE_MAIN_MENU;
      }
    }
  }  

  // ten seconds jobs
  if ((curMillis - lastMillis) >= 10000)
  {
    m_btscanner->printAvailable();

    lastMillis = curMillis;
  }

}

void loop()
{
#ifdef DEBUG_HEAP
  unsigned long curMillis = millis();
#endif

  switch (m_state)
  {
    case STATE_RECEIVING:
      state_receiving();
      break;
    case STATE_MAIN_MENU:
      state_main_menu();
      break;
    case STATE_SCANNING:
      state_scanning();
      break;
    case STATE_BLUETOOTH_MENU:
      state_bluetooth_menu();
      break;
    default:
      break;
  }

  // check if volume was not changed from speaker 
  uint8_t btAudioVolume = m_btaudio->getVolume();
  if (volume != btAudioVolume)
  {
    Serial.print("New volume=");
    Serial.print(btAudioVolume);
    Serial.print(" old volume=");
    Serial.println(volume);
    volume = btAudioVolume;
    m_radio->setVolume(volume);
    m_btaudio->setVolume(volume);
    preferences.putUChar("volume", volume);
  }

#ifdef DEBUG_HEAP
  // one seconds jobs
  if ((curMillis - lastMillisMain) >= 1000)
  {
    check_mem_usage();
    lastMillisMain = curMillis;
  }
#endif
}
