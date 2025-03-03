#ifndef RADIO_H
#define RADIO_H

#include <DABShield.h>
#include <Arduino.h>

#include <SPI.h>

typedef void (*rdsTextUpdatedType)(String text);

class Radio
{
public:
  Radio(SPIClass *spi);
  virtual ~Radio();

  static void serviceData();
  void setRdsTextUpdatedCallack(rdsTextUpdatedType callback);
  void update();
  void scan();

  void getTime(uint8_t *hour, uint8_t *min);

  char* getEnsamble();
  char* getServiceName();
  int8_t getSignalStrength();

  void tuneService(uint8_t freq, uint32_t serviceID);

  static DAB *m_dab;
  static rdsTextUpdatedType m_rdsTextUpdatedCbf;

 private:
  void ensembleInfo();

  uint32_t m_serviceID;
  static String m_rdsText;
};

#endif