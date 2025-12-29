#ifndef BT_AUDIO_H
#define BT_AUDIO_H

#include <AudioLogger.h>
#include <AudioTools.h>
#include <AudioToolsConfig.h>
#include <BluetoothA2DPSource.h>
#include <Arduino.h>

#include "bt_scanner.h"

enum bt_audio_state {
  BT_AUDIO_STATE_DISCONNECTED = 0,
  BT_AUDIO_STATE_CONNECTING,
  BT_AUDIO_STATE_CONNECTED
};

class BtAudio
{
public:
  BtAudio(BtScanner *btscanner);
  virtual ~BtAudio();

  void update();

  void setVolume(uint8_t vol);
  void connectTo(String ssid);
  bt_audio_state getState();

  static int32_t get_data_frames(Frame *frame, int32_t frame_count);
  static void connection_state_changed(esp_a2d_connection_state_t state, void *ptr);
  static bool isValid(const char* ssid, esp_bd_addr_t address, int rssi);

  static BtScanner *m_btscanner;
  static BluetoothA2DPSource *m_a2dp_src;

 private:
  static String m_ssid;
  static uint8_t m_volume;

  static I2SStream *m_i2s;
};

#endif