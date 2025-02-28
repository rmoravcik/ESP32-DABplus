#ifndef BT_AUDIO_H
#define BT_AUDIO_H

#include <BluetoothA2DPSource.h>
#include <Arduino.h>

#include "bt_scanner.h"

class BtAudio
{
public:
  BtAudio(BtScanner *btscanner);
  virtual ~BtAudio();

  void update();

  static int32_t get_data_frames(Frame *frame, int32_t frame_count);
  static void connection_state_changed(esp_a2d_connection_state_t state, void *ptr);
  static bool isValid(const char* ssid, esp_bd_addr_t address, int rssi);

  static BtScanner *m_btscanner;
  static BluetoothA2DPSource *m_a2dp_src;

 private:
};

#endif