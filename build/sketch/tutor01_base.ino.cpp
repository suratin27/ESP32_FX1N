#include <Arduino.h>
#line 1 "d:\\Product\\2023_Product\\Git\\ESP32_FX1N\\ESP32_FX1N\\examples\\tutor01_base\\tutor01_base.ino"
/*
  หลังจากโหลดลงไฟล์นี้ลงไปใน ESP32 Control 2.0RXO แล้วสามารถ Monitor โดยใช้ GXWork2 ได้เลย 
*/

#include "ESP32_FX1N.h"

long lastTime;

void setup(){
  InitPLC();
}

void loop(){
  if(millis() - lastTime > 500){    //- Random value and assing to D100
    setU16D(100,random(0,999));
  }
}

