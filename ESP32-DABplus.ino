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
#define DEBUG_HEAP
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

#define STATION_LIST_SIZE 100

StationInfo stationList[STATION_LIST_SIZE];
uint8_t stationCount = 0;
uint8_t currentStation = 0;

uint8_t volume = 25;

Preferences preferences;

struct list_entry bt_list_entries[BT_SCANNER_LIST_SIZE];
uint8_t bt_list_pages = 0;
uint8_t bt_list_page = 0;

struct list_entry station_list_entries[STATION_LIST_SIZE];
uint8_t station_list_pages = 0;
uint8_t station_list_page = 0;

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
  m_display->drawStationLabel(stationList[index].label, m_state != STATE_RECEIVING ? true : false);
  m_radio->tuneStation(stationList[index].freqIndex, stationList[index].serviceId, stationList[index].compId);
}

void setup()
{
  Serial.begin(115200);
  Serial.print("ESP32-DABplus version ");
  Serial.println(VERSION);

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

void one_sec_job()
{
    uint8_t hour = 0, min = lastMin;

    m_radio->getTime(&hour, &min);
    if (min != lastMin)
    {
      m_display->drawTime(hour, min, m_state != STATE_RECEIVING ? true : false);
      lastMin = min;
    }

    m_display->drawBtIndicator((bt_indicator_state)m_btaudio->getState(), m_state != STATE_RECEIVING ? true : false);
    m_display->drawSignalIndicator(m_radio->getSignalStrength(), m_state != STATE_RECEIVING ? true : false);
}

void ten_sec_job()
{
  if (m_state == STATE_BLUETOOTH_MENU)
  {
    m_btscanner->printAvailable();
  }
}

void state_receiving()
{
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
    one_sec_job();
    lastMillis = curMillis;
  }
}
      
void state_main_menu()
{
  unsigned long curMillis = millis();
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
        m_display->drawRadioMenu();
        m_state = STATE_RADIO_MENU;
      }
      else if ((y > 90) && (y < 160))
      {
        uint8_t index = 0;
        uint8_t selected_index;
        struct bt_entry** bt_list = m_btscanner->getList();

        memset(bt_list_entries, 0, sizeof(struct list_entry) * BT_SCANNER_LIST_SIZE);

        m_btscanner->lockList();
        for (uint8_t i = 0; i < BT_SCANNER_LIST_SIZE; i++)
        {
          if (bt_list[i] != NULL)
          {
            bt_list_entries[index].text = (char *) malloc(strlen(bt_list[i]->ssid) + 1);
            strcpy(bt_list_entries[index].text, bt_list[i]->ssid);
            bt_list_entries[index].icon = "/headphones.png";
            if (bt_list[i]->state == BT_ENTRY_STATE_DISCONNECTED)
            {
              bt_list_entries[index].is_selected = false;
            }
            else
            {
              bt_list_entries[index].is_selected = true;
              selected_index = index;
            }
            index++;
          }
        }
        m_btscanner->unlockList();

        bt_list_pages = 1;
        if (index > 0)
        {
          bt_list_pages = ((index - 1) / 3) + 1;
        }

        bt_list_page = 0;
        if (selected_index > 0)
        {
          bt_list_page = selected_index / 3;
        }

        m_display->drawListMenu();
        m_display->drawListMenuEntries(bt_list_entries, bt_list_page, bt_list_pages);
        lastMillis = 0;
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

  // one second jobs
  if ((curMillis - lastMillis) >= 1000)
  {
    one_sec_job();
    lastMillis = curMillis;
  }
}

void state_radio_menu()
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
      if ((y > 20) && (y < 90))
      {
        uint16_t index = 0;
        uint16_t selected_index = 0;

        memset(station_list_entries, 0, sizeof(struct list_entry) * STATION_LIST_SIZE);

        for (uint16_t i = 0; i < stationCount; i++)
        {
          station_list_entries[index].text = (char *) malloc(strlen(stationList[i].label.c_str()) + 1);
          strcpy(station_list_entries[index].text, stationList[i].label.c_str());
          station_list_entries[index].icon = "/radio.png";
          if (i == currentStation)
          {
            station_list_entries[index].is_selected = true;
            selected_index = index;
          }
          else
          {
            station_list_entries[index].is_selected = false;
          }
          index++;
        }

        station_list_pages = 1;
        if (index > 0)
        {
          station_list_pages = ((index - 1) / 3) + 1;
        }

        station_list_page = 0;
        if (selected_index > 0)
        {
          station_list_page = selected_index / 3;
        }

        m_display->drawListMenu();
        m_display->drawListMenuEntries(station_list_entries, station_list_page, station_list_pages);
        m_state = STATE_STATION_LIST;
      }
      else if ((y > 90) && (y < 160))
      {
        m_state = STATE_SCANNING;
      }
      else if ((y > 230) && (y < 300))
      {
        // exit
        m_display->drawMainMenu();
        m_state = STATE_MAIN_MENU;
      }
    }
  }

  // one second jobs
  if ((curMillis - lastMillis) >= 1000)
  {
    one_sec_job();
    lastMillis = curMillis;
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
  uint8_t secCounter = 0;
  uint16_t x;
  uint16_t y;

  m_btaudio->update();
  m_btscanner->update();

  if (m_display->getTouch(&x, &y))
  {
    if ((x > 40) && (x < 376))
    {
      if (x > 50)
      {
        uint8_t index = 0xFF;
        if ((y > 30) && (y < 100))
        {
          index = 3 * bt_list_page;
        }
        else if ((y > 100) && (y < 170))
        {
          index = 3 * bt_list_page + 1;
        }
        else if ((y > 170) && (y < 240))
        {
          index = 3 * bt_list_page + 2;
        }

        if (index < BT_SCANNER_LIST_SIZE)
        {
          if (bt_list_entries[index].text)
          {
            if (bt_list_entries[index].is_selected)
            {
              bt_list_entries[index].is_selected = false;
              m_btaudio->disconnect();
            }
            else
            {
              // unselect previous entry
              for (uint8_t i = 0; i < BT_SCANNER_LIST_SIZE; i++)
              {
                if (bt_list_entries[i].is_selected == true)
                {
                  bt_list_entries[i].is_selected = false;
                  m_btaudio->disconnect();
                }
              }

              bt_list_entries[index].is_selected = true;
              m_btaudio->connectTo(bt_list_entries[index].text);
            }

            m_display->drawListMenuEntries(bt_list_entries, bt_list_page, bt_list_pages);
          }
        }
      }

      if ((y > 240) && (y < 300))
      {
        // back
        for (uint8_t i = 0; i < BT_SCANNER_LIST_SIZE; i++)
        {
            if (bt_list_entries[i].text)
            {
              free(bt_list_entries[i].text);
            }
        }

        m_display->drawMainMenu();
        m_state = STATE_MAIN_MENU;
      }
    }
    else if ((x > 386) && (x < 430))
    {
      if ((y > 30) && (y < 74))
      {
        if (bt_list_page > 0)
        {
          bt_list_page--;
          m_display->drawListMenuEntries(bt_list_entries, bt_list_page, bt_list_pages);
        }
      }
      else if ((y > 196) && (y < 240))
      {
        if (bt_list_page < (bt_list_pages - 1))
        {
          bt_list_page++;
          m_display->drawListMenuEntries(bt_list_entries, bt_list_page, bt_list_pages);
        }
      }
    }
  }  

  // one seconds jobs
  if ((curMillis - lastMillis) >= 1000)
  {
    one_sec_job();

    secCounter++;
    if (secCounter >= 10) {
      secCounter = 0;
      ten_sec_job();
    }

    lastMillis = curMillis;
  }
}

void state_station_menu()
{
  unsigned long curMillis = millis();
  uint16_t x;
  uint16_t y;

  m_btaudio->update();
  m_btscanner->update();

  if (m_display->getTouch(&x, &y))
  {
    if ((x > 40) && (x < 376))
    {
      if (x > 50)
      {
        uint8_t index = 0xFF;
        if ((y > 30) && (y < 100))
        {
          index = 3 * station_list_page;
        }
        else if ((y > 100) && (y < 170))
        {
          index = 3 * station_list_page + 1;
        }
        else if ((y > 170) && (y < 240))
        {
          index = 3 * station_list_page + 2;
        }

        if (index < stationCount)
        {
          if (station_list_entries[index].text)
          {
            if (station_list_entries[index].is_selected == false)
            {
              // unselect previous entry
              for (uint16_t i = 0; i < stationCount; i++)
              {
                if (station_list_entries[i].is_selected == true)
                {
                  station_list_entries[i].is_selected = false;
                }
              }

              station_list_entries[index].is_selected = true;

              currentStation = index;
              tuneStation(currentStation);
              preferences.putUChar("currentStation", currentStation);
            }

            m_display->drawListMenuEntries(station_list_entries, station_list_page, station_list_pages);
          }
        }
      }

      if ((y > 240) && (y < 300))
      {
        // back
        for (uint16_t i = 0; i < stationCount; i++)
        {
            if (station_list_entries[i].text)
            {
              free(station_list_entries[i].text);
            }
        }

        m_display->drawRadioMenu();
        m_state = STATE_RADIO_MENU;
      }
    }
    else if ((x > 386) && (x < 430))
    {
      if ((y > 30) && (y < 74))
      {
        if (station_list_page > 0)
        {
          station_list_page--;
          m_display->drawListMenuEntries(station_list_entries, station_list_page, station_list_pages);
        }
      }
      else if ((y > 196) && (y < 240))
      {
        if (station_list_page < (station_list_pages - 1))
        {
          station_list_page++;
          m_display->drawListMenuEntries(station_list_entries, station_list_page, station_list_pages);
        }
      }
    }
  }

  // one second jobs
  if ((curMillis - lastMillis) >= 1000)
  {
    one_sec_job();
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
    case STATE_RADIO_MENU:
      state_radio_menu();
      break;
    case STATE_STATION_LIST:
      state_station_menu();
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
