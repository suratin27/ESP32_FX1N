#include <Arduino.h>
#include <ESP32_FX1N.h>  
#include <WiFi.h>
#include <WiFiUdp.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <ESPAsyncWiFiManager.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>

#define OLED_SDA 16
#define OLED_SCL 17
Adafruit_SH1106 display(OLED_SDA, OLED_SCL);

/*-------------------------------------------------------------------
                              WiFi Function
-------------------------------------------------------------------*/
AsyncWebServer WebServer(80);
DNSServer dns;

void initWiFi(){
  display.println("Start WiFi man.");
  AsyncWiFiManager wifiManager(&WebServer,&dns);
  wifiManager.setDebugOutput(false);
  //wifiManager.resetSettings();
  if (!wifiManager.startConfigPortal("ESP32 FX1N PLC")){
    delay(1000);
  }

  //------------ Web setting ------------
  WebServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request){							//- Route for root / web page
    request->send_P(200, "text/html","Hi! I am ESP32.");
  });
  display.clearDisplay();
  display.println("Start ElegantOTA");
  AsyncElegantOTA.begin(&WebServer);
  WebServer.begin();
  display.clearDisplay();
  display.println("WiFi succeed");
  delay(2000);
}

/*-------------------------------------------------------------------
                              OLED Function
-------------------------------------------------------------------*/
void initOLED(){
  display.begin(SH1106_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();
  //set the text size, color, cursor position and displayed text
  display.setTextSize(2);
  display.setTextColor(WHITE);
  //- Display 0
  display.setCursor(6, 0);
  display.println("ESP_FX1N");
  display.setCursor(12, 35);
  display.println("PLC");
  display.display();
  delay(3000);
  display.clearDisplay();
}

/*-------------------------------------------------------------------
                            Global variable
-------------------------------------------------------------------*/
unsigned long lastime = 0;
const int peroid = 200;
uint8_t loopCount = 0;

void setup(){
  Serial2.begin(9600,SERIAL_8N1,22,23);
  InitPLC();
  initOLED();
  Serial.println("PLC Configurations Loaded");
  //initWiFi();
  display.clearDisplay();
}

void loop(){
  if(millis() - lastime > peroid){
    lastime = millis();
    setFD(100,random(1, 401) / 100.0);    //- Set D100 - แบบ float
    display.clearDisplay();
    //- Show T0
    display.setCursor(5,0);
    display.println("T0: ");
    display.setCursor(60,0);
    display.println(getT(0));
    //- Show T1
    display.setCursor(5,20);
    display.println("T1: ");
    display.setCursor(60,20);
    display.println(getT(1));
    //- Show T1
    display.setCursor(5,40);
    display.println("C3: ");
    display.setCursor(60,40);
    display.println(getC(3));

    display.display();

    if(loopCount > 1){
      loopCount = 0;
    }else{
      loopCount++;
    }
  }
}
