/*
  XCSensors http://XCSensors.org

  Copyright (c), PrimalCode (http://www.primalcode.org)

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  any later version. see <http://www.gnu.org/licenses/>
*/
#include <Arduino.h>
#include <Wire.h>
#include "I2cEeprom.h"
#include "config.h"
#include "XCSensors.h"
#if defined(I2CEEPROM)

void i2c_eeprom_write_page( int deviceaddress, unsigned int eeaddresspage, byte* data, byte len )
{

  //TODO: make lenght the actual lenght , devide it by 24 aan read it in loop -1
  // last one is the rest.


  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddresspage >> 8)); // Address High Byte
  Wire.write((int)(eeaddresspage & 0xFF)); // Address Low Byte
  byte c;
  for ( c = 0; c < len; c++)           //TODO: this be where we devide into blocks
    Wire.write(data[c]);
  Wire.endTransmission();
  delay(10);                           // need some delay

}


void i2c_eeprom_read_page( int deviceaddress, unsigned int eeaddress, byte *buffer, int len )
{
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));    // Address High Byte
  Wire.write((int)(eeaddress & 0xFF));  // Address Low Byte
  Wire.endTransmission();
  Wire.requestFrom(deviceaddress, len);
  //int c = 0;
  for ( int c = 0; c < len; c++ ) {
    if (Wire.available()) buffer[c] = Wire.read();
  }
}

/* TODO: Make this readable to normal humans  (or just some proper grammar)
    Data to the EEPROM is written sequentially from a starting address.
   The length of the written data will be unknown to the program trying to read it.
   To keep track, 8 bytes stores info of the data.
   1st 3 bytes will be the eeprom struct version
   2nd 2 bytes the lengt of data data batch
   last 2 bytes a checksum. TODO: implement this
   0-1023 will be reserved for configuration data
   1024 - 10239 for program future fumctions
   10240 - max for logging. logging will be devided in blocks of
   256 bytes (or less, to be determed)

   The size given on these chips are bits not Bytes. So a 512 is actually about 64KB
*/
//Writes the given size
void writeSizeValue(int point, int sizeValue) {
  byte bSize[8];
  bSize[0] = (EEPROMPVERSION >> 16) & 0xFF;
  bSize[1] = (EEPROMPVERSION >> 8) & 0xFF;
  bSize[2] = EEPROMPVERSION & 0xFF;
  bSize[3] = (sizeValue >> 16) & 0xFF;
  bSize[4] = (sizeValue >> 8) & 0xFF;
  bSize[5] = sizeValue & 0xFF;
  bSize[6] = 0x0A;
  bSize[7] = 0x0A;

  i2c_eeprom_write_page(I2CEEPROM, point, bSize, 8);
}


/*
   Get stored size of page at given point
*/
int readSizeValue(int point) {
  byte bSize[8];
  i2c_eeprom_read_page(I2CEEPROM, point, bSize, 8);


  int val = 0;
  val += bSize[0] << 16;
  val += bSize[1] << 8;
  val += bSize[2];
  val = 0;
  if ( val == EEPROMPVERSION) {
    
    val += bSize[3] << 16;
    val += bSize[4] << 8;
    val += bSize[5];
  }

  return val;

}



bool writeI2CBin(const uint8_t id, uint16_t adr, char data[], const uint16_t len, const uint8_t pageSize) {

  uint16_t bk = len;
  bool abort = false;
  uint8_t i;
  uint16_t j = 0;
  uint32_t timeout;
  uint16_t mask = pageSize - 1;
  while ((bk > 0) && !abort) {
    i = 30;
    if (i > bk) i = bk;
    if (((adr) & ~mask) != ((((adr) + i) - 1) & ~mask)) { // over page!
      i = (((adr) | mask) - (adr)) + 1;

    }
    timeout = millis();
    bool ready = false;
    while (!ready && (millis() - timeout < 10)) {
      Wire.beginTransmission((uint8_t)id);
      ready = (Wire.endTransmission() == 0); // wait for device to become ready!
    }
    if (!ready) {
      abort = true;

      break;
    }

    Wire.beginTransmission((uint8_t)id);
    Wire.write((uint8_t)highByte(adr));
    Wire.write((uint8_t)lowByte(adr));

    bk = bk - i;
    adr = (adr) + i;

    while (i > 0) {
      Wire.write((uint8_t)data[j++]);
      i--;
    }

    uint8_t err = Wire.endTransmission();
    if (err != 0) {
      abort = true;
      break;
    }
    // else Serial.println();
  }
  return !abort;
}

bool readI2CBin(const uint8_t id, uint16_t adr, byte *data, const uint16_t len, const uint8_t pageSize) {
  uint16_t bk = len;
  bool abort = false;
  uint8_t i;
  uint16_t j = 0;
  uint32_t timeout;
  uint16_t mask = pageSize - 1;

  while ((bk > 0) && !abort) {
    i = 30;
    if (i > bk) i = bk;
    if (((adr) & ~mask) != ((((adr) + i) - 1) & ~mask)) { // over page!
      i = (((adr) | mask) - (adr)) + 1;
    }
    timeout = millis();
    bool ready = false;
    while (!ready && (millis() - timeout < 10)) {
      Wire.beginTransmission((uint8_t)id);
      ready = (Wire.endTransmission() == 0); // wait for device to become ready!
    }
    if (!ready) {
      abort = true;
      break;
    }

    Wire.beginTransmission((uint8_t)id);
    Wire.write((uint8_t)highByte(adr));
    Wire.write((uint8_t)lowByte(adr));
    uint8_t err = Wire.endTransmission();
    Wire.requestFrom((uint8_t)id, i);
    bk = bk - i;
    adr = (adr) + i;

    while (i > 0) {
      if (Wire.available()) data[j++] = Wire.read();
      i--;
    }

    if (err != 0) {
      abort = true;
      break;
    }
    // else Serial.println();
  }

  return !abort;
}

#endif
