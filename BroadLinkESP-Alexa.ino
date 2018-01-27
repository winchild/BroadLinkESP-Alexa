#include <fauxmoESP.h>

#include <AESLib.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <BroadLinkESP.h>
#include "fauxmoESP.h"

AESLib aesLib;
BroadLinkESP besp(BROADLINK_DEV_MP1);

#define WIFI_SSID "WINCHILD-HOME 2.4"
#define WIFI_PASS "xxxxxxxx"
#define TOADDR "192.168.0.111"		//broadlink mp1 ip
#define SERIAL_BAUDRATE    115200 

byte g_mac[6]={0x34,0xEA,0x34,0xC6,0xE4,0xF8};//boardlink mp1 mac
fauxmoESP fauxmo;

/*
static const uint8_t D0 = 16;
static const uint8_t D1 = 5;
static const uint8_t D2 = 4;
static const uint8_t D3 = 0;
static const uint8_t D4 = 2;
static const uint8_t D5 = 14;
static const uint8_t D6 = 12;
static const uint8_t D7 = 13;
static const uint8_t D8 = 15;
static const uint8_t D9 = 3;
static const uint8_t D10 = 1;
*/
// Set Relay Pins
#define LED      2 // D4 for indicate
#define PIN14_D5 14
#define PIN15_D6 12

void setup() 
{
  pinMode(LED, OUTPUT);    // for indicate
  digitalWrite(LED, HIGH);
  digitalWrite(PIN14_D5, LOW);
  digitalWrite(PIN15_D6, LOW);
  
  Serial.begin(SERIAL_BAUDRATE);
  Serial.println("\nBooting...");  

  WiFi.mode(WIFI_STA);

  // Connect
  Serial.printf("[WIFI] Connecting to %s ", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // Wait
  while (WiFi.status() != WL_CONNECTED) 
  {
    Serial.print(".");
    delay(200);
  }
  // Connected!
  Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

  besp.setDebug(1);
  besp.setDestIP(TOADDR);
  //Serial.printf("after setDestIP\n");
  besp.setDestMAC(g_mac);
  //Serial.printf("after setDestMAC\n");
  besp.preparePacketAuth();
  //Serial.printf("after preparePacketAuth\n");
  besp.sendPacket();
  //Serial.printf("after sendPacket\n");

  pinMode(PIN14_D5, OUTPUT);    // assinged to 'small room light'
  digitalWrite(PIN14_D5, HIGH);
  pinMode(PIN15_D6, OUTPUT);    // assigned to 'outlet 1'
  digitalWrite(PIN15_d6, HIGH); 

  fauxmo.addDevice("broadlink s1");  // device #0
  fauxmo.addDevice("broadlink s2");  // device #1
  fauxmo.addDevice("broadlink s3");  // device #2
  fauxmo.addDevice("broadlink s4");  // device #3
  fauxmo.addDevice("small room light");  // device #4
  fauxmo.addDevice("outlet");  // device #5

  fauxmo.onMessage([](unsigned char device_id, const char * device_name, bool state) {
    Serial.printf("[MAIN] Device #%d (%s) state: %s\n", device_id, device_name, state ? "ON" : "OFF");

    if (device_id == 4) {
          digitalWrite(PIN14_D5, !state);
    }
    else if (device_id == 5) {
          digitalWrite(PIN15_d6, !state);
    } 
    else {
      if(besp.isReady())
      {
        Serial.printf("SWITCH %s (%d) on %d\n", device_name, device_id+1, state);
        besp.preparePacketSetPower(device_id+1, state);
        besp.sendPacket();
      }
    }
  });
}

void loop() 
{
  int packetSize;
  static int led_stat = 0;
  
  packetSize=besp.checkReadPacket();
  if(packetSize>0)
  {
    besp.readPacket(packetSize);
    Serial.printf("readPacket: %d bytes\n", packetSize);
  }

  // Since fauxmoESP 2.0 the library uses the "compatibility" mode by
  // default, this means that it uses WiFiUdp class instead of AsyncUDP.
  // The later requires the Arduino Core for ESP8266 staging version
  // whilst the former works fine with current stable 2.3.0 version.
  // But, since it's not "async" anymore we have to manually poll for UDP
  // packets
  fauxmo.handle();

  static unsigned long last = millis();
  if (millis() - last > 1000) {
      last = millis();
      Serial.printf("[MAIN] Free heap: %d bytes, LED=%d\n", ESP.getFreeHeap(), led_stat);
      digitalWrite(LED, led_stat);
      led_stat = 1 - led_stat;
  }

/*  
  if(Serial.available())
  {
    byte c;
    c=Serial.read();
    if((c>='1') && (c<='4'))
    {
      if(besp.isReady())
      {
        Serial.printf("SWITCH %d %d\n",c,ionoff);
        besp.preparePacketSetPower(c-0x30,ionoff);
        besp.sendPacket();
      }
    }
    else if(c=='a')
      ionoff=1-ionoff;    
  }
*/

}
