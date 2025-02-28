#ifndef BT_SCANNER_H
#define BT_SCANNER_H

#include <Arduino.h>
#include <BluetoothA2DPSource.h>

enum bt_entry_state {
  BT_ENTRY_STATE_DISCONNECTED = 0,
  BT_ENTRY_STATE_CONNECTING
};

struct bt_entry {
  char *ssid;
  bt_entry_state state;
  unsigned long age;
};

#define BT_SCANNER_LIST_SIZE 16
#define BT_SCANNER_AGE_EXPIRED 3000

class BtScanner
{
public:
  BtScanner();
  virtual ~BtScanner();

  bool insert(const char* ssid);
  void update();
  void printAvailable();

 private:
  struct bt_entry* list[BT_SCANNER_LIST_SIZE];
};

#endif