#include "AudioTools/AudioLibs/A2DPStream.h"

#include <string.h>
#include "ESP32-DABplus.h"
#include "bt_audio.h"

const int BYTES_PER_FRAME = 4;

BtScanner *BtAudio::m_btscanner = NULL;
BluetoothA2DPSource *BtAudio::m_a2dp_src = NULL;
I2SStream *BtAudio::m_i2s = NULL;

String BtAudio::m_ssid = "";
uint8_t BtAudio::m_volume = 0;

BtAudio::BtAudio(BtScanner *btscanner)
{
  // AudioToolsLogger.begin(Serial, AudioToolsLogLevel::Info);
  AudioInfo info16(44100, 2, 16);

  m_btscanner = btscanner;
  m_a2dp_src = new BluetoothA2DPSource();

  m_i2s = new I2SStream();

  auto cfg = m_i2s->defaultConfig(RX_MODE);

  cfg.i2s_format = I2S_STD_FORMAT;
  cfg.pin_bck = GPIO_SI468X_DCLK;
  cfg.pin_ws = GPIO_SI468X_DFS;
  cfg.pin_data = GPIO_SI468X_DOUT;
  cfg.copyFrom(info16);
  cfg.is_master = true;

  m_i2s->begin(cfg);

  m_a2dp_src->set_ssid_callback(isValid);
  m_a2dp_src->set_auto_reconnect(false);
  m_a2dp_src->set_on_connection_state_changed(connection_state_changed);
  m_a2dp_src->set_data_callback_in_frames(get_data_frames);
  m_a2dp_src->set_avrc_passthru_command_callback(button_handler);

  m_a2dp_src->start();
}

BtAudio::~BtAudio()
{
}

void BtAudio::update()
{

}

int32_t BtAudio::get_data_frames(Frame *frame, int32_t frame_count)
{
  return m_i2s->readBytes((uint8_t*)frame, frame_count * BYTES_PER_FRAME) / BYTES_PER_FRAME;
}

bt_audio_state BtAudio::getState()
{
  switch (m_a2dp_src->get_connection_state())
  {
    case ESP_A2D_CONNECTION_STATE_CONNECTING:
      return BT_AUDIO_STATE_CONNECTING;
    case ESP_A2D_CONNECTION_STATE_CONNECTED:
      return BT_AUDIO_STATE_CONNECTED;
    default:
      return BT_AUDIO_STATE_DISCONNECTED;
  }
}

// for esp_a2d_connection_state_t see https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/bluetooth/esp_a2dp.html#_CPPv426esp_a2d_connection_state_t
void BtAudio::connection_state_changed(esp_a2d_connection_state_t state, void *ptr)
{
  Serial.print(F("BT Connection state: "));
  Serial.println(m_a2dp_src->to_str(state));

  switch (state) {
    case ESP_A2D_CONNECTION_STATE_CONNECTING:
      m_btscanner->insert(m_ssid.c_str());
      m_btscanner->setState(m_ssid.c_str(), BT_ENTRY_STATE_CONNECTING);
      break;
    case ESP_A2D_CONNECTION_STATE_CONNECTED:
      m_btscanner->setState(m_ssid.c_str(), BT_ENTRY_STATE_CONNECTED);
      m_a2dp_src->set_volume(m_volume);
      break;
    case ESP_A2D_CONNECTION_STATE_DISCONNECTED:
    default:
      m_btscanner->setState(m_ssid.c_str(), BT_ENTRY_STATE_DISCONNECTED);
      break;
  }
}

// Return true to connect, false will continue scanning: You can can use this
// callback to build a list.
bool BtAudio::isValid(const char *ssid, esp_bd_addr_t address, int rssi)
{
  if (m_ssid.equals(ssid)) {
    Serial.print(F("BT Found SSID: "));
    Serial.println(m_ssid);
    return true;
  }

//  Serial.print("Found SSID: ");
//  Serial.println(ssid);

  m_btscanner->insert(ssid);
  return false;
}

// gets called when button on bluetooth speaker is pressed
void BtAudio::button_handler(uint8_t id, bool isReleased)
{
  if (isReleased)
  {
    Serial.print("Button ID=");
    Serial.print(id);
    Serial.println(" released");
  }
}

void BtAudio::setVolume(uint8_t vol)
{
  if (vol > VOLUME_MAX)
  {
    vol = VOLUME_MAX;
  }
  m_volume = vol * 2;

  m_a2dp_src->set_volume(m_volume);
}

uint8_t BtAudio::getVolume()
{
  return m_a2dp_src->get_volume() / 2;
}

void BtAudio::connectTo(String ssid)
{
  m_ssid = ssid;
  m_a2dp_src->start(ssid.c_str());
  m_a2dp_src->set_connected(true);
}

void BtAudio::disconnect()
{
  m_ssid = "";
  m_a2dp_src->set_connected(false);
}
