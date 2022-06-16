#include <FastLED.h>
#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#define ESPALEXA_ASYNC
#include <Espalexa.h>
#include <OneButton.h>

#define TOP_BUTTON_PIN    D1
#define BOTTOM_BUTTON_PIN D2
#define INFRARED_PIN      D3
#define LED_PIN           D7
#define NUM_LEDS          22
#define LED_TYPE          WS2812B
#define COLOR_ORDER       GRB
#define LAMP_NAME         "Demo Lamp"

const char* ssid = "COMGIC";
const char* password = "";
uint8_t ledBrightness = 10; //10-100
uint8_t ledColourIndex = 0;
uint8_t ledColourCount = 12;
uint8_t ledColours[ledColourCount][3] = {
  {255, 192, 140}, //white
  {255, 228, 205}, //cool white
  {255, 0, 0}, //Red
  {254, 166, 0}, //orange
  {255, 212, 0}, //glod
  {254, 255, 0}, //yellow
  {0, 255, 0}, //green
  {0, 254, 255}, //cyan
  {0, 0, 255}, //blue
  {171, 35, 255}, //purple
  {255, 0, 254}, //magenta
  {159, 127, 255}, //lavender
};
boolean wifiConnected = false;
//boolean connectWifi();
void colorLightChanged(uint8_t brightness, uint32_t rgb);

OneButton topButton(TOP_BUTTON_PIN, true, true);
OneButton bottomButton(BOTTOM_BUTTON_PIN, true, true);
CRGB leds[NUM_LEDS];
EspalexaDevice* louisLamp;
Espalexa espalexa;

void setup()
{
  Serial.begin(115200);

  topButton.attachClick(topButton_Clicked);
  topButton.attachDuringLongPress(topButton_Pressed);
  bottomButton.attachClick(bottomButton_Clicked);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(ledBrightness*255/100);

  wifiConnected = connectWifi();
  wifiConnectionEffect();
  
  louisLamp = new EspalexaDevice(LAMP_NAME, colorLightChanged, 32);
  espalexa.addDevice(louisLamp);
  espalexa.begin();

  systemReadyEffect();
}

void loop() {
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

void wifiConnectionEffect(){
  if(wifiConnected){
    Serial.println("Showing WIFI connected effect");
  }else{
    Serial.println("Showing WIFI problem effect");
  }
}

void systemReadyEffect() {
    Serial.println("Showing system ready effect");
}

void topButton_Clicked() {
  Serial.println("Top button clicked");
  ledColourIndex++;
  if(ledColourIndex >= ledColourCount){
    ledColourIndex = 0;
  }
  louisLamp->setColor(ledColours[ledColourIndex][0], ledColours[ledColourIndex][1], ledColours[ledColourIndex][2]);
  setLeds(ledBrightness*255/100, ledColours[ledColourIndex][0], ledColours[ledColourIndex][1], ledColours[ledColourIndex][2]);
}

void topButton_Pressed() {
  Serial.println("Top button pressed");
}

void bottomButton_Clicked() {
  Serial.println("Bottom button clicked");
  ledBrightness += 10;
  if (ledBrightness > 100) {
    ledBrightness = 10;
  }
  louisLamp->setPercent(ledBrightness);
  setLeds(ledBrightness*255/100, ledColours[ledColourIndex][0], ledColours[ledColourIndex][1], ledColours[ledColourIndex][2]);
}

void colorLightChanged(uint8_t brightness, uint32_t rgb) {
  float r = ((rgb >> 16) & 0xFF);
  float g = ((rgb >> 8) & 0xFF);
  float b = (rgb & 0xFF);
  setLeds(brightness, r, g, b);
  Serial.print("r:");
  Serial.print(r);
  Serial.print("  ");
  Serial.print("g:");
  Serial.print(g);
  Serial.print("  ");
  Serial.print("b:");
  Serial.println(b);
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
      state = false;
      break;
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

void setLeds(uint8_t brightness, uint8_t r, uint8_t g, uint8_t b) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB(r, g, b);
  }
  FastLED.setBrightness(brightness);
  FastLED.show();
}
