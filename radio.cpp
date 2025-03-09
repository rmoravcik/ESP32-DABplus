#include <string.h>
#include <SPI.h>
#include "ESP32-DABplus.h"
#include "radio.h"

#include "soc/spi_reg.h"
#include "driver/spi_master.h"

DAB *Radio::m_dab = NULL;
static SPIClass *m_spi = NULL;
rdsTextUpdatedType Radio::m_rdsTextUpdatedCbf = NULL;
slideShowUpdatedType Radio::m_slideShowUpdatedCbf = NULL;
stationFoundType Radio::m_stationFoundCbf = NULL;

uint16_t rdsCharConverter(const uint8_t ch)
{
  switch (ch)
  {
    case 0x5E: return '―';
    case 0x5F: return '_';
    case 0x60: return '`';
    case 0x7E: return '¯';
    case 0x7F: return ' ';
    case 0x80: return 'á';
    case 0x81: return 'à';
    case 0x82: return 'é';
    case 0x83: return 'è';
    case 0x84: return 'í';
    case 0x85: return 'ì';
    case 0x86: return 'ó';
    case 0x87: return 'ò';
    case 0x88: return 'ú';
    case 0x89: return 'ù';
    case 0x8A: return 'Ñ';
    case 0x8B: return 'Ç';
    case 0x8C: return 'Ş';
    case 0x8D: return 'β';
    case 0x8E: return '¡';
    case 0x8F: return 'Ĳ';
    case 0x90: return 'â';
    case 0x91: return 'ä';
    case 0x92: return 'ê';
    case 0x93: return 'ë';
    case 0x94: return 'î';
    case 0x95: return 'ï';
    case 0x96: return 'ô';
    case 0x97: return 'ö';
    case 0x98: return 'û';
    case 0x99: return 'ü';
    case 0x9A: return 'ñ';
    case 0x9B: return 'ç';
    case 0x9C: return 'ş';
    case 0x9D: return 'ǧ';
    case 0x9E: return 'ı';
    case 0x9F: return 'ĳ';
    case 0xA0: return 'ª';
    case 0xA1: return 'α';
    case 0xA2: return '©';
    case 0xA3: return '‰';
    case 0xA4: return 'Ǧ';
    case 0xA5: return 'ě';
    case 0xA6: return 'ň';
    case 0xA7: return 'ő';
    case 0xA8: return 'π';
    case 0xA9: return '€';
    case 0xAA: return '£';
    case 0xAB: return '$';
    case 0xAC: return '←';
    case 0xAD: return '↑';
    case 0xAE: return '→';
    case 0xAF: return '↓';
    case 0xB0: return 'º';
    case 0xB1: return '¹';
    case 0xB2: return '²';
    case 0xB3: return '³';
    case 0xB4: return '±';
    case 0xB5: return 'İ';
    case 0xB6: return 'ń';
    case 0xB7: return 'ű';
    case 0xB8: return 'µ';
    case 0xB9: return '¿';
    case 0xBA: return '÷';
    case 0xBB: return '°';
    case 0xBC: return '¼';
    case 0xBD: return '½';
    case 0xBE: return '¾';
    case 0xBF: return '§';
    case 0xC0: return 'Á';
    case 0xC1: return 'À';
    case 0xC2: return 'É';
    case 0xC3: return 'È';
    case 0xC4: return 'Í';
    case 0xC5: return 'Ì';
    case 0xC6: return 'Ó';
    case 0xC7: return 'Ò';
    case 0xC8: return 'Ú';
    case 0xC9: return 'Ù';
    case 0xCA: return 'Ř';
    case 0xCB: return 'Č';
    case 0xCC: return 'Š';
    case 0xCD: return 'Ž';
    case 0xCE: return 'Ð';
    case 0xCF: return 'Ŀ';
    case 0xD0: return 'Â';
    case 0xD1: return 'Ä';
    case 0xD2: return 'Ê';
    case 0xD3: return 'Ë';
    case 0xD4: return 'Î';
    case 0xD5: return 'Ï';
    case 0xD6: return 'Ô';
    case 0xD7: return 'Ö';
    case 0xD8: return 'Û';
    case 0xD9: return 'Ü';
    case 0xDA: return 'ř';
    case 0xDB: return 'č';
    case 0xDC: return 'š';
    case 0xDD: return 'ž';
    case 0xDE: return 'đ';
    case 0xDF: return 'ŀ';
    case 0xE0: return 'Ã';
    case 0xE1: return 'Å';
    case 0xE2: return 'Æ';
    case 0xE3: return 'Œ';
    case 0xE4: return 'ŷ';
    case 0xE5: return 'Ý';
    case 0xE6: return 'Õ';
    case 0xE7: return 'Ø';
    case 0xE8: return 'Þ';
    case 0xE9: return 'Ŋ';
    case 0xEA: return 'Ŕ';
    case 0xEB: return 'Ć';
    case 0xEC: return 'Ś';
    case 0xED: return 'Ź';
    case 0xEE: return 'Ŧ';
    case 0xEF: return 'ð';
    case 0xF0: return 'ã';
    case 0xF1: return 'å';
    case 0xF2: return 'æ';
    case 0xF3: return 'œ';
    case 0xF4: return 'ŵ';
    case 0xF5: return 'ý';
    case 0xF6: return 'õ';
    case 0xF7: return 'ø';
    case 0xF8: return 'þ';
    case 0xF9: return 'ŋ';
    case 0xFA: return 'ŕ';
    case 0xFB: return 'ć';
    case 0xFC: return 'ś';
    case 0xFD: return 'ź';
    case 0xFE: return 'ŧ';
    case 0xFF: return ' ';
    default: break;
  }

  return 0;
}

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
  if (m_dab->rdstextvalid())
  {
    char rdsText[256];
    uint8_t i = 0, j = 0;
    uint8_t rdsTextLen = strlen(m_dab->ServiceData);

    for (i = j = 0; i < rdsTextLen; i++)
    {
      uint16_t newChar = rdsCharConverter(m_dab->ServiceData[i]);

      if (newChar != 0)
      {
        uint8_t *newCharUtf8 = (uint8_t *)&newChar;

        rdsText[j]     = newCharUtf8[1];
        rdsText[j + 1] = newCharUtf8[0];

        j++;
      }
      else
      {
        rdsText[j] = m_dab->ServiceData[i];
      }
      j++;
    }
    rdsText[j] = '\0';

//    Serial.println(rdsText);
//    for (uint8_t i = 0; i < strlen(rdsText); i++)
//    {
//      Serial.print(" 0x");
//      Serial.print(rdsText[i], HEX);
//    }
//    Serial.print("\n");

    if (m_rdsTextUpdatedCbf)
    {
      m_rdsTextUpdatedCbf(String(rdsText));
    }
  }

  if (m_dab->slideshowvalid())
  {
    if (m_slideShowUpdatedCbf)
    {
      m_slideShowUpdatedCbf();
    }
  }
}

void Radio::setRdsTextUpdatedCallback(rdsTextUpdatedType callback)
{
  m_rdsTextUpdatedCbf = callback;
}

void Radio::setSlideShowUpdatedCallback(slideShowUpdatedType callback)
{
  m_slideShowUpdatedCbf = callback;
}

void Radio::setStationFoundCallback(stationFoundType callback)
{
  m_stationFoundCbf = callback;
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

    if (m_stationFoundCbf)
    {
      String label = m_dab->service[i].Label;
      label.trim();

      m_stationFoundCbf(m_dab->freq_index, m_dab->service[i].ServiceID, label);
    }

  }
  Serial.print(F("\n"));
}

void Radio::tuneStation(uint8_t freqIndex, uint32_t serviceId)
{
  m_dab->tune(freqIndex);
  if (m_dab->servicevalid())
  {
    for (uint8_t i = 0; i < m_dab->numberofservices; i++)
    {
      if (m_dab->service[i].ServiceID == serviceId)
      {
        m_dab->set_service(i);
        break;
      }
    }
  }
}

void Radio::getTime(uint8_t *hour, uint8_t *min)
{
  DABTime dabTime;
 
  if (m_dab->time(&dabTime) == 0)
  {
    *hour = dabTime.Hours;
    *min = dabTime.Minutes;
  }
}

int8_t Radio::getSignalStrength()
{
  m_dab->status();
  return m_dab->signalstrength;
}

void DABSpiMsg(unsigned char *data, uint32_t len)
{
  m_spi->beginTransaction(SPISettings(SI4684_SPI_FREQ, MSBFIRST, SPI_MODE0));
  digitalWrite (GPIO_SI468X_SSBSI, LOW);
  m_spi->transfer(data, len);
  digitalWrite (GPIO_SI468X_SSBSI, HIGH);
  m_spi->endTransaction();
}
