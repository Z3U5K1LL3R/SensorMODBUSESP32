#include <Arduino.h>

#include "ModbusClientRTU.h"
#include "Logging.h"

#define RXPIN 16
#define TXPIN 17
#define REDPIN 2
#define BAUDRATE 9600
#define FIRST_REGISTER 0
#define NUM_VALUES 21
#define READ_INTERVAL 10000

bool data_ready = false;
int values[NUM_VALUES];
uint32_t request_time;

ModbusClientRTU MB(REDPIN);

void handleData(ModbusMessage response, uint32_t token)
{
  uint16_t offs = 3;
  for (uint8_t i = 0; i < NUM_VALUES; ++i)
  {
    offs = response.get(offs, values[i]);
  }
  request_time = token;
  data_ready = true;
}

void handleError(Error error, uint32_t token)
{
  ModbusError me(error);
  LOG_E("Error response: %02X-%s\n", (int)me, (const char *)me);
}

void setup()
{
  Serial.begin(115200);
  while (!Serial)
  {
  }
  Serial.println("__OK__");

  RTUutils::prepareHardwareSerial(Serial2);
  Serial2.begin(BAUDRATE, SERIAL_8N1, RXPIN, TXPIN);

  MB.onDataHandler(&handleData);
  MB.onErrorHandler(&handleError);
  MB.setTimeout(2000);
  MB.begin(Serial2);
}

void loop()
{
  static unsigned long next_request = millis();
  if (millis() - next_request > READ_INTERVAL)
  {
    data_ready = false;
    Error err = MB.addRequest((uint32_t)millis(), 1, READ_HOLD_REGISTER, FIRST_REGISTER, 2);
    if (err != SUCCESS)
    {
      ModbusError e(err);
      LOG_E("Error creating request: %02X-%S\n", (int)e, (const char *)e);
    }
    next_request = millis();
  }
  else
  {
    if (data_ready)
    {
      Serial.printf("Requested at %8.3fs:\n", request_time / 1000.0);
      for (uint8_t i = 0; i < NUM_VALUES; ++i)
      {
        Serial.printf("   %04X:%02d\n", i * 2 + FIRST_REGISTER, values[i]);
      }
      Serial.printf("----------\n\n");
      data_ready = false;
    }
  }
}