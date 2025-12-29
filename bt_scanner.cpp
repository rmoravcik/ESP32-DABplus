 #include <string.h>

#include "bt_scanner.h"

static SemaphoreHandle_t mutex = NULL;

BtScanner::BtScanner()
{
  memset(list, 0, sizeof(list));
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
//        Serial.print("i=");
//        Serial.print(i);
//        Serial.print(" ssid=");
//        Serial.print(ssid);
//        Serial.println(" update");
        list[i]->age = millis();
        xSemaphoreGive(mutex);
        return true;
      }
    }

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

      Serial.print("i=");
      Serial.print(i);
      Serial.print(" ssid=");
      Serial.print(ssid);
      Serial.println(" insert");

      xSemaphoreGive(mutex);
      return true;
    }
  }

  xSemaphoreGive(mutex);
  return false;
}

void BtScanner::update()
{
  xSemaphoreTake(mutex, portMAX_DELAY);
  for (uint8_t i = 0; i < BT_SCANNER_LIST_SIZE; i++)
  {
    if (list[i] != NULL)
    {
      if ((millis() - list[i]->age) > BT_SCANNER_AGE_EXPIRED)
      {
//        Serial.print("i=");
//        Serial.print(i);
//        Serial.print(" ssid=");
//        Serial.print(list[i]->ssid);
//        Serial.println(" remove");

        if (list[i]->ssid != NULL)
        {
          free(list[i]->ssid);
        }
        free(list[i]);
        list[i] = NULL;
      }
    }
  }
  xSemaphoreGive(mutex);
}

void BtScanner::printAvailable()
{
  Serial.println(F("Available:"));
  for (uint8_t i = 0; i < BT_SCANNER_LIST_SIZE; i++)
  {
    if (list[i] != NULL)
    {
      Serial.println(list[i]->ssid);
    }
  }
}