#ifndef RADIO_H
#define RADIO_H

#include <DABShield.h>
#include <Arduino.h>

#include <SPI.h>
class Radio
{
public:
  Radio(SPIClass *spi);
  virtual ~Radio();

  static void serviceData();
  void update();
  void scan();

  void getTime(uint8_t *hour, uint8_t *min);

  char* getEnsamble();
  char* getServiceName();
  char* getServiceData();


  void tuneService(uint8_t freq, uint32_t serviceID, uint32_t compID);

  static DAB *m_dab;

 private:
  void ensembleInfo();
};

#endif