#include <string.h>
#include "bt_audio.h"

#define c3_frequency  130.81

BtScanner *BtAudio::m_btscanner = NULL;
BluetoothA2DPSource *BtAudio::m_a2dp_src = NULL;

BtAudio::BtAudio(BtScanner *btscanner)
{
  m_btscanner = btscanner;
  m_a2dp_src = new BluetoothA2DPSource();

  m_a2dp_src->set_ssid_callback(isValid);
  m_a2dp_src->set_auto_reconnect(false);
  m_a2dp_src->set_on_connection_state_changed(connection_state_changed);
  m_a2dp_src->set_data_callback_in_frames(get_data_frames);
  m_a2dp_src->set_volume(30);

  m_a2dp_src->start("B13");
//  m_a2dp_src->set_connected(true);
}

BtAudio::~BtAudio()
{
}

void BtAudio::update()
{
}

// The supported audio codec in ESP32 A2DP is SBC. SBC audio stream is encoded
// from PCM data normally formatted as 44.1kHz sampling rate, two-channel 16-bit sample data
int32_t BtAudio::get_data_frames(Frame *frame, int32_t frame_count)
{
    static float m_time = 0.0;
    float m_amplitude = 10000.0;  // -32,768 to 32,767
    float m_deltaTime = 1.0 / 44100.0;
    float m_phase = 0.0;
    float pi_2 = PI * 2.0;
    // fill the channel data
    for (int sample = 0; sample < frame_count; ++sample) {
        float angle = pi_2 * c3_frequency * m_time + m_phase;
        frame[sample].channel1 = m_amplitude * sin(angle);
        frame[sample].channel2 = frame[sample].channel1;
        m_time += m_deltaTime;
    }

    return frame_count;
}

// for esp_a2d_connection_state_t see https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/bluetooth/esp_a2dp.html#_CPPv426esp_a2d_connection_state_t
void BtAudio::connection_state_changed(esp_a2d_connection_state_t state, void *ptr)
{
  Serial.println(m_a2dp_src->to_str(state));
}

// Return true to connect, false will continue scanning: You can can use this
// callback to build a list.
bool BtAudio::isValid(const char* ssid, esp_bd_addr_t address, int rssi)
{
  m_btscanner->insert(ssid);
  return false;
}
