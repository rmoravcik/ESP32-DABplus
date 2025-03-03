#include <string.h>
#include <SPI.h>
#include "ESP32-DABplus.h"
#include "radio.h"

#include "soc/spi_reg.h"
#include "driver/spi_master.h"

DAB *Radio::m_dab = NULL;
static SPIClass *m_spi = NULL;
rdsTextUpdatedType Radio::m_rdsTextUpdatedCbf = NULL;
String Radio::m_rdsText = "Více rádia";

wchar_t rdsCharConverter(const char ch)
{
  switch (ch)
  {
    case 0x20: return L' ';
    case 0x21 ... 0x5D: return ch;
    case 0x5E: return L'―';
    case 0x5F: return L'_';
    case 0x60: return L'`';
    case 0x61 ... 0x7d: return ch;
    case 0x7E: return L'¯';
    case 0x7F: return L' ';
    case 0x80: return L'á';
    case 0x81: return L'à';
    case 0x82: return L'é';
    case 0x83: return L'è';
    case 0x84: return L'í';
    case 0x85: return L'ì';
    case 0x86: return L'ó';
    case 0x87: return L'ò';
    case 0x88: return L'ú';
    case 0x89: return L'ù';
    case 0x8A: return L'Ñ';
    case 0x8B: return L'Ç';
    case 0x8C: return L'Ş';
    case 0x8D: return L'β';
    case 0x8E: return L'¡';
    case 0x8F: return L'Ĳ';
    case 0x90: return L'â';
    case 0x91: return L'ä';
    case 0x92: return L'ê';
    case 0x93: return L'ë';
    case 0x94: return L'î';
    case 0x95: return L'ï';
    case 0x96: return L'ô';
    case 0x97: return L'ö';
    case 0x98: return L'û';
    case 0x99: return L'ü';
    case 0x9A: return L'ñ';
    case 0x9B: return L'ç';
    case 0x9C: return L'ş';
    case 0x9D: return L'ǧ';
    case 0x9E: return L'ı';
    case 0x9F: return L'ĳ';
    case 0xA0: return L'ª';
    case 0xA1: return L'α';
    case 0xA2: return L'©';
    case 0xA3: return L'‰';
    case 0xA4: return L'Ǧ';
    case 0xA5: return L'ě';
    case 0xA6: return L'ň';
    case 0xA7: return L'ő';
    case 0xA8: return L'π';
    case 0xA9: return L'€';
    case 0xAA: return L'£';
    case 0xAB: return L'$';
    case 0xAC: return L'←';
    case 0xAD: return L'↑';
    case 0xAE: return L'→';
    case 0xAF: return L'↓';
    case 0xB0: return L'º';
    case 0xB1: return L'¹';
    case 0xB2: return L'²';
    case 0xB3: return L'³';
    case 0xB4: return L'±';
    case 0xB5: return L'İ';
    case 0xB6: return L'ń';
    case 0xB7: return L'ű';
    case 0xB8: return L'µ';
    case 0xB9: return L'¿';
    case 0xBA: return L'÷';
    case 0xBB: return L'°';
    case 0xBC: return L'¼';
    case 0xBD: return L'½';
    case 0xBE: return L'¾';
    case 0xBF: return L'§';
    case 0xC0: return L'Á';
    case 0xC1: return L'À';
    case 0xC2: return L'É';
    case 0xC3: return L'È';
    case 0xC4: return L'Í';
    case 0xC5: return L'Ì';
    case 0xC6: return L'Ó';
    case 0xC7: return L'Ò';
    case 0xC8: return L'Ú';
    case 0xC9: return L'Ù';
    case 0xCA: return L'Ř';
    case 0xCB: return L'Č';
    case 0xCC: return L'Š';
    case 0xCD: return L'Ž';
    case 0xCE: return L'Ð';
    case 0xCF: return L'Ŀ';
    case 0xD0: return L'Â';
    case 0xD1: return L'Ä';
    case 0xD2: return L'Ê';
    case 0xD3: return L'Ë';
    case 0xD4: return L'Î';
    case 0xD5: return L'Ï';
    case 0xD6: return L'Ô';
    case 0xD7: return L'Ö';
    case 0xD8: return L'Û';
    case 0xD9: return L'Ü';
    case 0xDA: return L'ř';
    case 0xDB: return L'č';
    case 0xDC: return L'š';
    case 0xDD: return L'ž';
    case 0xDE: return L'đ';
    case 0xDF: return L'ŀ';
    case 0xE0: return L'Ã';
    case 0xE1: return L'Å';
    case 0xE2: return L'Æ';
    case 0xE3: return L'Œ';
    case 0xE4: return L'ŷ';
    case 0xE5: return L'Ý';
    case 0xE6: return L'Õ';
    case 0xE7: return L'Ø';
    case 0xE8: return L'Þ';
    case 0xE9: return L'Ŋ';
    case 0xEA: return L'Ŕ';
    case 0xEB: return L'Ć';
    case 0xEC: return L'Ś';
    case 0xED: return L'Ź';
    case 0xEE: return L'Ŧ';
    case 0xEF: return L'ð';
    case 0xF0: return L'ã';
    case 0xF1: return L'å';
    case 0xF2: return L'æ';
    case 0xF3: return L'œ';
    case 0xF4: return L'ŵ';
    case 0xF5: return L'ý';
    case 0xF6: return L'õ';
    case 0xF7: return L'ø';
    case 0xF8: return L'þ';
    case 0xF9: return L'ŋ';
    case 0xFA: return L'ŕ';
    case 0xFB: return L'ć';
    case 0xFC: return L'ś';
    case 0xFD: return L'ź';
    case 0xFE: return L'ŧ';
    case 0xFF: return L' ';
    default: break;
  }
  return ' ';
}

Radio::Radio(SPIClass *spi)
{
  Serial.println(F("Initializing radio"));

  m_spi = spi;
  m_serviceID = 0;

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
  String rdsText(m_dab->ServiceData);

  Serial.println(rdsText);
  for (uint8_t i = 0; i < rdsText.length(); i++)
  {
    Serial.print(" 0x");
    Serial.print(rdsText.charAt(i), HEX);
    rdsText.setCharAt(i, rdsCharConverter(rdsText.charAt(i)));
  }
  Serial.print("\n");

  Serial.println(rdsText.c_str());
  for (uint8_t i = 0; i < rdsText.length(); i++)
  {
    Serial.print(" 0x");
    Serial.print(rdsText.charAt(i), HEX);
  }
  Serial.print("\n");

  if (!m_rdsText.equals(rdsText))
  {
    if (m_rdsTextUpdatedCbf)
    {
      Serial.println("Calling callback");
      m_rdsTextUpdatedCbf(rdsText);
    }
    m_rdsText = rdsText;
  }
}

void Radio::setRdsTextUpdatedCallack(rdsTextUpdatedType callback)
{
  m_rdsTextUpdatedCbf = callback;
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

void Radio::tuneService(uint8_t freq, uint32_t serviceID)
{
  m_dab->tune(freq);
  m_dab->set_service(serviceID);
  m_serviceID = serviceID;
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

char* Radio::getEnsamble()
{
  return m_dab->Ensemble;
}

char* Radio::getServiceName()
{
  return m_dab->service[m_serviceID].Label;
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
