#include <string.h>
#include <SPI.h>
#include "ESP32-DABplus.h"
#include "radio.h"

#include "soc/spi_reg.h"
#include "driver/spi_master.h"

DAB *Radio::m_dab = NULL;
static SPIClass *m_spi = NULL;

Radio::Radio(SPIClass *spi)
{
  Serial.println(F("Initializing radio"));

  m_spi = spi;

  m_dab = new DAB();
  m_dab->setCallback(serviceData);
  m_dab->begin(0, GPIO_SI468X_INTB, GPIO_SI468X_RSTB, -1);

  if (m_dab->error != 0)
  {
    Serial.print(F("ERROR: "));
    Serial.println(m_dab->error);
  }
  else
  {
    Serial.print(F("PartNo = Si"));
    Serial.println(m_dab->PartNo);

    Serial.print(F("Firmware Version = "));
    Serial.print(m_dab->VerMajor);
    Serial.print(F("."));
    Serial.print(m_dab->VerMinor);
    Serial.print(F("."));
    Serial.println(m_dab->VerBuild);
  }
}

Radio::~Radio()
{

}

void Radio::update()
{
  m_dab->task();
}

void Radio::serviceData()
{
  Serial.print(m_dab->ServiceData);
  Serial.print(F("\n"));
}

void Radio::scan(void)
{     
  uint8_t freq_index;
  char freqstring[32];

  for (freq_index = 0; freq_index < DAB_FREQS; freq_index++)
  {
    Serial.print(F("\nScanning Freq "));
    sprintf(freqstring, "%02d\t %03d.", freq_index, (uint16_t)(m_dab->freq_khz(freq_index) / 1000));
    Serial.print(freqstring);
    sprintf(freqstring, "%03d MHz", (uint16_t)(m_dab->freq_khz(freq_index) % 1000));
    Serial.print(freqstring);
    m_dab->tune(freq_index);
    if(m_dab->servicevalid() == true)
    {
      ensembleInfo();
    }
  }
  Serial.print(F("\n\n"));
}
void Radio::ensembleInfo(void)
{
  char freqstring[32];
  uint8_t i;

  Serial.print(F("\n\nEnsemble Freq "));
  sprintf(freqstring, "%02d\t %03d.", m_dab->freq_index, (uint16_t)(m_dab->freq_khz(m_dab->freq_index) / 1000));
  Serial.print(freqstring);
  sprintf(freqstring, "%03d MHz", (uint16_t)(m_dab->freq_khz(m_dab->freq_index) % 1000));
  Serial.print(freqstring);

  Serial.print(F("\n"));
  Serial.print(m_dab->Ensemble);
  Serial.print(F("\n"));

  Serial.print(F("\nServices: \n"));
  Serial.print(F("ID\tName\n\n"));

  for (i = 0; i < m_dab->numberofservices; i++)
  {
    Serial.print(i);
    Serial.print(F(":\t"));
    Serial.print(m_dab->service[i].Label);
    m_dab->status(m_dab->service[i].ServiceID, m_dab->service[i].CompID);
    if(m_dab->type == SERVICE_AUDIO)
    {
      Serial.print(F("\t dab"));
      if(m_dab->dabplus == true)
      {
        Serial.print(F("+"));
      }
    }
    else if(m_dab->type == SERVICE_DATA)
    {
      Serial.print(F("\t data"));
    }
    Serial.print(F("\n"));
  }
  Serial.print(F("\n"));
}

void Radio::tuneService(uint8_t freq, uint32_t serviceID, uint32_t compID)
{
  m_dab->tune(freq);
  m_dab->set_service(serviceID);
}

void Radio::getTime(uint8_t *hour, uint8_t *min)
{
  if ((m_dab->Hours > 23) || (m_dab->Minutes > 59))
  {
    return;
  }

  *hour = m_dab->Hours;
  *min = m_dab->Minutes;
}

char* Radio::getEnsamble()
{
  return m_dab->Ensemble;
}

char* Radio::getServiceName()
{
  return m_dab->service[0].Label;
}

char* Radio::getServiceData()
{
  return m_dab->ServiceData;
}

void DABSpiMsg(unsigned char *data, uint32_t len)
{
  m_spi->beginTransaction(SPISettings(SI4684_SPI_FREQ, MSBFIRST, SPI_MODE0));
  digitalWrite (GPIO_SI468X_SSBSI, LOW);
  m_spi->transfer(data, len);
  digitalWrite (GPIO_SI468X_SSBSI, HIGH);
  m_spi->endTransaction();
}
