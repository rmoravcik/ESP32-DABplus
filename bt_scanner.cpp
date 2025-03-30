 #include <string.h>

#include "bt_scanner.h"

BtScanner::BtScanner()
{
  memset(list, 0, sizeof(list));
}

BtScanner::~BtScanner()
{

}

bool BtScanner::insert(const char* ssid)
{
  for (uint8_t i = 0; i < BT_SCANNER_LIST_SIZE; i++)
  {
    // update age time if entry already exists
    if (list[i] != NULL)
    {
      if (strcmp(list[i]->ssid, ssid) == 0)
      {
        list[i]->age = millis();
        return true;
      }
    }

    // insert new entry
    Serial.println("inserting...");
    if (list[i] == NULL)
    {
      struct bt_entry *entry = (struct bt_entry *) malloc(sizeof (struct bt_entry));
      if (entry == NULL)
      {
        return false;
      }

      entry->ssid = (char *) malloc(strlen(ssid) + 1);
      if (entry->ssid == NULL)
      {
        free(entry);
        return false;
      }

      strcpy(entry->ssid, ssid);
      entry->state = BT_ENTRY_STATE_DISCONNECTED;
      entry->age = millis();

      list[i] = entry;
      return true;
    }
  }

  return false;
}

void BtScanner::update()
{
  for (uint8_t i = 0; i < BT_SCANNER_LIST_SIZE; i++)
  {
    if (list[i] != NULL)
    {
      if ((millis() - list[i]->age) > BT_SCANNER_AGE_EXPIRED)
      {
        if (list[i]->ssid != NULL)
        {
          free(list[i]->ssid);
        }
        free(list[i]);
        list[i] = NULL;
      }
    }
  }
}

void BtScanner::printAvailable()
{
  Serial.println("Available:");
  for (uint8_t i = 0; i < BT_SCANNER_LIST_SIZE; i++)
  {
    if (list[i] != NULL)
    {
      Serial.println(list[i]->ssid);
    }
  }
}