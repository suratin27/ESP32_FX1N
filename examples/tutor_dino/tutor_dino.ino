#define DINOPLC   100
#include <SPI.h>
#include <Ethernet.h>
#include <ESP32_FX1N.h>
#include "ModbusServerEthernet.h"
#include <EthernetUdp.h>
#include <TimeLib.h>
#include <WiFi.h>

/*
ต้อง Modify C:\Users\super\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.9\cores\esp32\Server.h
virtual void begin(uint16_t port=0) =0; >>> virtual void begin() =0;
*/


ModbusServerEthernet mTCPServer;

//---------------- Ethernet W5500
#define ETH_RST 22
#define ETH_CS 5
#define ETH_SCLK 18
#define ETH_MISO 23
#define ETH_MOSI 19
#define NET      33

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
bool ethernet_connected;
long ethernet_lastmill;
IPAddress lIP;

void ethernetReset(const uint8_t resetPin){
  pinMode(resetPin, OUTPUT);
  digitalWrite(resetPin, HIGH);
  delay(250);
  digitalWrite(resetPin, LOW);
  delay(50);
  digitalWrite(resetPin, HIGH);
  delay(350);
}

void initEthernet(){
  SPI.begin(ETH_SCLK,ETH_MISO,ETH_MOSI,ETH_CS);
  ethernetReset(ETH_RST);
  Ethernet.init(ETH_CS);
  Ethernet.begin(mac);
  pinMode(NET,OUTPUT);
  delay(2000);
  if(Ethernet.linkStatus() == LinkON){
    lIP = Ethernet.localIP();
    //Serial.printf("My IP address: %u.%u.%u.%u\n", lIP[0], lIP[1], lIP[2], lIP[3]);
  }
}

void updateLink(){
  auto link = Ethernet.linkStatus();
  switch (link) {
    case Unknown:
      //Serial.println("Unknown");
      digitalWrite(NET,HIGH);
      break;
    case LinkON:
      //Serial.println("ON");
      digitalWrite(NET,LOW);
      break;
    case LinkOFF:
      //Serial.println("OFF");
      digitalWrite(NET,HIGH);
      break;
  }
}


//---------------- Modbus TCP server
ModbusMessage FC03(ModbusMessage request){
  ModbusMessage response; // The Modbus message we are going to give back
  uint16_t addr = 0;      // Start address
  uint16_t words = 0;     // # of words requested
  request.get(2, addr);   // read address from request
  request.get(4, words);  // read # of words from request

  // Address overflow?
  if((addr + words) > 20){
    // Yes - send respective error response
    response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
  }
  // Set up response
  response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));
  // Request for FC 0x03?
  if(request.getFunctionCode() == READ_HOLD_REGISTER){
    // Internal CPU temperature >> ADDR 0
    for(uint8_t i = 0; i < words; ++i){
      //- Get D0 - Dxxx
      response.add(getU16D(i));
    }
  }else{
    // No, this is for FC 0x04. Response is random
    for(uint8_t i = 0; i < words; ++i){
      // send increasing data values
      response.add((uint16_t)random(1, 65535));
    }
  }
  // Send response back
  return response;
}

ModbusMessage FC06(ModbusMessage request) {
  uint16_t addr = 0;        // Start address to read
  uint16_t value = 0;       // New value for register
  ModbusMessage response;

  // Get addr and value from data array. Values are MSB-first, getValue() will convert to binary
  request.get(2, addr);
  request.get(4, value);

  // address valid?
  if (addr || addr > 32) {
    // No. Return error response
    response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
    return response;
  }

  // Modbus address is 1..n, memory address 0..n-1
  //addr--;

  // Fake data error - 0x0000 or 0xFFFF will not be accepted
  if (!value || value == 0xFFFF) {
    // No. Return error response
    response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_VALUE);
    return response;
  }

  // Fill in new value.
  //memo[addr] = value;
  setU16D(addr,value);

  // Return the ECHO response
  return ECHO_RESPONSE;
}


//---------------- NTP Server time
unsigned int localPort = 8888;       // local port to listen for UDP packets
const char timeServer[] = "time.nist.gov"; // time.nist.gov NTP server
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
EthernetUDP Udp;
long NTPLasttime;
bool RTCSet;

void initNTPTime(){
  Udp.begin(localPort);
}

// send an NTP request to the time server at the given address
void sendNTPpacket(const char * address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); // NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

void updateNTPTime(){
  if(millis() - NTPLasttime > 5000 && !RTCSet){
    NTPLasttime = millis();
    sendNTPpacket(timeServer); // send an NTP packet to a time server
    delay(1000);
    if(Udp.parsePacket()){
      // We've received a packet, read the data from it
      Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

      // the timestamp starts at byte 40 of the received packet and is four bytes,
      // or two words, long. First, extract the two words:
      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
      // combine the four bytes (two words) into a long integer
      // this is NTP time (seconds since Jan 1 1900):
      unsigned long secsSince1900 = highWord << 16 | lowWord;

      // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
      const unsigned long seventyYears = 2208988800UL;
      unsigned long epoch = secsSince1900 - seventyYears;

      int _day   = day(epoch);
      int _month = month(epoch);
      int _year  = year(epoch);
      int _hour  = hour(epoch);//(epoch % 86400L)/3600*8;
      int _min   = minute(epoch);//(epoch % 3600)/60;
      int _sec   = second(epoch);//(epoch % 60);

      RTCSettime(_sec,_min,_hour+1,_day,_month,_year);
      RTCSet = true;

      //Serial.printf("Date: %d-%d-%d ,",_day,_month,_year);
      //Serial.printf("Time : %d",_hour);
      //Serial.printf(" : %d",_min);
      //Serial.printf(" : %d\n",_sec);
    }
  }
}

//---------------- Main loop
void setup(){
  InitPLC(DINOPLC);
  initEthernet();

  mTCPServer.registerWorker(1, READ_HOLD_REGISTER, &FC03);      // FC=03 for serverID=1
  mTCPServer.registerWorker(1, READ_INPUT_REGISTER, &FC03);     // FC=04 for serverID=1
  mTCPServer.registerWorker(1, WRITE_HOLD_REGISTER, &FC06);     // FC=06 for serverID=1
  mTCPServer.start(502,4,20000);

  initNTPTime();
}

void loop(){
  updateLink();
  updateRTC();

  updateNTPTime();
}