 #include <string.h>

#include "bt_scanner.h"

#define LIST_DEBUG
// #define LIST_DEBUG_UPDATE

static SemaphoreHandle_t mutex = NULL;

BtScanner::BtScanner()
{
  memset(list, 0, sizeof(list));
  count = 0;
  mutex = xSemaphoreCreateMutex();
}

BtScanner::~BtScanner()
{

}

bool BtScanner::insert(const char* ssid)
{
  xSemaphoreTake(mutex, portMAX_DELAY);

  for (uint8_t i = 0; i < BT_SCANNER_LIST_SIZE; i++)
  {
    // update age time if entry already exists
    if (list[i] != NULL)
    {
      if (strcmp(list[i]->ssid, ssid) == 0)
      {
#ifdef LIST_DEBUG_UPDATE
        Serial.print("i=");
        Serial.print(i);
        Serial.print(" ssid=");
        Serial.print(ssid);
        Serial.println(" update");
#endif

        list[i]->age = millis();
        xSemaphoreGive(mutex);
        return true;
      }
    }
  }

  for (uint8_t i = 0; i < BT_SCANNER_LIST_SIZE; i++)
  {
    // insert new entry
    if (list[i] == NULL)
    {
      struct bt_entry *entry = (struct bt_entry *) malloc(sizeof (struct bt_entry));
      if (entry == NULL)
      {
        xSemaphoreGive(mutex);
        return false;
      }

      entry->ssid = (char *) malloc(strlen(ssid) + 1);
      if (entry->ssid == NULL)
      {
        free(entry);
        xSemaphoreGive(mutex);
        return false;
      }

      strcpy(entry->ssid, ssid);
      entry->state = BT_ENTRY_STATE_DISCONNECTED;
      entry->age = millis();

      list[i] = entry;
      count++;

#ifdef LIST_DEBUG
      Serial.print("i=");
      Serial.print(i);
      Serial.print(" ssid=");
      Serial.print(ssid[0], HEX);
      Serial.print(ssid[1], HEX);
      Serial.print(ssid[2], HEX);
      Serial.print(ssid[3], HEX);
      Serial.print(" ssid=");
      Serial.print(ssid);
      Serial.println(" insert");
#endif

      xSemaphoreGive(mutex);
      return true;
    }
  }

  xSemaphoreGive(mutex);
  return false;
}

void BtScanner::setState(const char* ssid, bt_entry_state state)
{
  xSemaphoreTake(mutex, portMAX_DELAY);

  for (uint8_t i = 0; i < BT_SCANNER_LIST_SIZE; i++)
  {
    // update age time if entry already exists
    if (list[i] != NULL)
    {
      if (strcmp(list[i]->ssid, ssid) == 0)
      {
        list[i]->state = state;
        break;
      }
    }
  }

  xSemaphoreGive(mutex);
}

void BtScanner::update()
{
  xSemaphoreTake(mutex, portMAX_DELAY);
  for (uint8_t i = 0; i < BT_SCANNER_LIST_SIZE; i++)
  {
    if (list[i] != NULL)
    {
      if ((list[i]->state == BT_ENTRY_STATE_DISCONNECTED) &&
          ((millis() - list[i]->age) > BT_SCANNER_AGE_EXPIRED))
      {
#ifdef LIST_DEBUG
        Serial.print("i=");
        Serial.print(i);
        Serial.print(" ssid=");
        Serial.print(list[i]->ssid);
        Serial.println(" remove");
#endif

        if (list[i]->ssid != NULL)
        {
          free(list[i]->ssid);
        }
        free(list[i]);
        list[i] = NULL;
        count--;
      }
    }
  }
  xSemaphoreGive(mutex);
}

struct bt_entry** BtScanner::getList()
{
  return list;
}

void BtScanner::lockList()
{
  xSemaphoreTake(mutex, portMAX_DELAY);
}

void BtScanner::unlockList()
{
  xSemaphoreGive(mutex);
}

void BtScanner::printAvailable()
{
  Serial.println(F("ID | SSID                             | State        | Age"));
  Serial.println(F("---+----------------------------------+--------------+------"));

  for (uint8_t i = 0; i < BT_SCANNER_LIST_SIZE; i++)
  {
    if (list[i] != NULL)
    {
      char strBuf[64];
      sprintf(strBuf, "%2u | %32s | %12s | %5u", i + 1, list[i]->ssid, stateToString(list[i]->state), 
              list[i]->state == BT_ENTRY_STATE_DISCONNECTED ? millis() - list[i]->age : 0);
      Serial.println(strBuf);
    }
  }
}

const char * BtScanner::stateToString(bt_entry_state state)
{
  switch (state)
  {
    case BT_ENTRY_STATE_DISCONNECTED:
      return "Disconnected";
    case BT_ENTRY_STATE_CONNECTING:
      return "Connecting";
    case BT_ENTRY_STATE_CONNECTED:
      return "Connected";
    default:
      break;
  }
  return "Unknown";
}
