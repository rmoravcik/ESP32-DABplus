#ifndef RADIO_H
#define RADIO_H

#include <DABShield.h>
#include <Arduino.h>

#include <SPI.h>

typedef void (*rdsTextUpdatedType)(String text);
typedef void (*slideShowUpdatedType)(void);
typedef void (*stationFoundType)(uint8_t freqIndex, uint32_t serviceId, uint32_t compId, String label);

class Radio
{
public:
  Radio(SPIClass *spi);
  virtual ~Radio();

  static void serviceData();
  void setRdsTextUpdatedCallback(rdsTextUpdatedType callback);
  void setSlideShowUpdatedCallback(slideShowUpdatedType callback);
  void setStationFoundCallback(stationFoundType callback);
  void update();
  void scan();
  void setVolume(uint8_t vol);

  void getTime(uint8_t *hour, uint8_t *min);
  int8_t getSignalStrength();

  void tuneStation(uint8_t freqIndex, uint32_t serviceId, uint32_t compId);

  static DAB *m_dab;
  static rdsTextUpdatedType m_rdsTextUpdatedCbf;
  static slideShowUpdatedType m_slideShowUpdatedCbf;
  static stationFoundType m_stationFoundCbf;

 private:
  void ensembleInfo();
};

#endif