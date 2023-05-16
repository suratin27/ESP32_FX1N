#ifndef ESP32_FX1N_H
#define ESP32_FX1N_H
#include <Arduino.h>
#include <SPIFFS.h>
#include <StreamUtils.h>
#include <ArduinoJson.h>

//--------------------------- Main Function Parts
void     InitPLC();
void     InitPLC(uint16_t boardver);
bool     getM(uint16_t addr);
void     setM(uint16_t addr,uint8_t pos);
void     clearM(uint16_t addr,uint8_t pos);
uint8_t  getU8D(uint16_t addr);
uint16_t getU16D(uint16_t addr);
uint32_t getU32D(uint16_t addr);
uint16_t getT(uint16_t _t);
uint16_t getC(uint16_t _t);
float    getFD(uint16_t addr);
void     setU8D(uint16_t addr,uint8_t val);
void     setU16D(uint16_t addr,uint16_t val);
void     setU32D(uint16_t addr,uint32_t val);
void     setFD(uint16_t addr,float val);

#endif