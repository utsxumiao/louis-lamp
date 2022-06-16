/*
   This is a basic example on how to use Espalexa with RGB color devices.
*/
#include <FastLED.h>
#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#define ESPALEXA_ASYNC
#include <Espalexa.h>

#define LED_PIN     D7
#define NUM_LEDS    22
#define BRIGHTNESS  64
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

// prototypes
boolean connectWifi();

//callback function prototype
void colorLightChanged(uint8_t brightness, uint32_t rgb);

// Change this!!
const char* ssid = "COMGIC";
const char* password = "";

boolean wifiConnected = false;

EspalexaDevice* louisLamp;
Espalexa espalexa;

void setup()
{
  Serial.begin(115200);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  
  // Initialise wifi connection
  wifiConnected = connectWifi();
  if (!wifiConnected) {
    while (1) {
      Serial.println("Cannot connect to WiFi. Please check data and reset the ESP.");
      delay(2500);
    }
  }

  louisLamp = new EspalexaDevice("Louis Lamp", colorLightChanged, 32);
  espalexa.addDevice(louisLamp);
  espalexa.begin();
  
  systemReadyEffect();
}

void loop(){
//  if (Serial.available() > 0) {
//    int incomingByte = 0;
//    incomingByte = Serial.read();
//    if(incomingByte == 49){
//      Serial.println(louisLamp->getValue());
//      louisLamp->setPercent(20);
//      louisLamp->setColor(0,255,0);
//      for (uint8_t i = 0; i < NUM_LEDS; i++) {
//        leds[i] = CRGB(0, 255, 0);
//      }
//      FastLED.setBrightness(round(255*20/100));
//      FastLED.show();
//      // say what you got:
//      Serial.println(incomingByte, DEC);
//    }
//  }
  espalexa.loop();
  delay(1);
}

void systemReadyEffect(){
  
}

void colorLightChanged(uint8_t brightness, uint32_t rgb) {
  Serial.print("rgb: ");
  Serial.println(rgb);
  float r = ((rgb >> 16) & 0xFF);
  float g = ((rgb >> 8) & 0xFF);
  float b = (rgb & 0xFF);
  setLeds(brightness, r, g, b);
}

boolean connectWifi() {
  boolean state = true;
  int i = 0;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.println("Connecting to WiFi");

  // Wait for connection
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (i > 40) {
      state = false; break;
    }
    i++;
  }
  Serial.println("");
  if (state) {
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.println("Connection failed.");
  }
  return state;
}

void setLeds(uint8_t brightness, uint8_t r, uint8_t g, uint8_t b){
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB(r, g, b);
  }
  FastLED.setBrightness(brightness);
  FastLED.show();
}
