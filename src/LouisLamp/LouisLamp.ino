#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <Espalexa.h>
#include <OneButton.h>

#define ESPALEXA_ASYNC
#define TOP_BUTTON_PIN    D1
#define BOTTOM_BUTTON_PIN D2
#define IR_INPUT_PIN      14//D5
#define LED_PIN           D7
#define NUM_LEDS          22
#define LED_TYPE          WS2812B
#define COLOR_ORDER       GRB
#define LAMP_NAME         "Louis Lamp"

#include "TinyIRReceiver.hpp"
#if !defined(STR_HELPER)
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#endif

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
uint8_t ledBrightness = 10; //10-100
uint8_t ledColourIndex = 0;
uint8_t ledColours[12][3] = {
  {255, 255, 255},  //  0 white
  {255, 0, 0},      //  1 Red
  {254, 166, 0},    //  2 orange
  {255, 212, 0},    //  3 glod
  {0, 255, 0},      //  4 green
  {0, 254, 255},    //  5 cyan
  {0, 0, 255},      //  6 blue
  {171, 35, 255},   //  7 purple
  {255, 0, 254},    //  8 magenta
  {159, 127, 255},  //  9 lavender
  {0, 0, 1},        //  * special: colour reel
  {0, 1, 0}         //  # special:
};
boolean wifiConnected = false;
void colorLightChanged(uint8_t brightness, uint32_t rgb);
uint8_t hue = 0;
volatile struct TinyIRReceiverCallbackDataStruct sCallbackData;
uint32_t lastColourPalleteMillis = 0;
const uint8_t colourPalleteInterval = 50;
uint32_t lastPoliceLightsMillis = 0;
const uint8_t policeLightsInterval = 100;
uint8_t policeLightsPattern[8][6] = {
  {1, 1, 1, 0, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {1, 1, 1, 0, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 1, 1, 1},
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 1, 1, 1},
  {0, 0, 0, 0, 0, 0}
};
uint8_t policeLightsPatternIndex = 0;
uint8_t columnLedStartingIndex[6] = {0, 4, 8, 11, 14, 18};

OneButton topButton(TOP_BUTTON_PIN, true, true);
OneButton bottomButton(BOTTOM_BUTTON_PIN, true, true);
CRGB leds[NUM_LEDS];
EspalexaDevice* louisLamp;
Espalexa espalexa;

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  topButton.attachClick(topButton_Clicked);
  topButton.attachLongPressStart(topButton_Pressed);
  bottomButton.attachClick(bottomButton_Clicked);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(ledBrightness * 255 / 100);

  initPCIInterruptForTinyReceiver();
  Serial.println(F("Ready to receive NEC IR signals at pin " STR(IR_INPUT_PIN)));

  wifiConnected = connectWifi();
  wifiConnectionEffect();

  louisLamp = new EspalexaDevice(LAMP_NAME, colorLightChanged, 32);
  espalexa.addDevice(louisLamp);
  espalexa.begin();

  setLeds(ledBrightness * 255 / 100, ledColours[ledColourIndex][0], ledColours[ledColourIndex][1], ledColours[ledColourIndex][2]);
}

void loop() {
  topButton.tick();
  bottomButton.tick();
  espalexa.loop();

  uint32_t currentMillis = millis();
  if (ledColourIndex == 10) {
    colourPaletteEffect(currentMillis);
  }
  if (ledColourIndex == 11) {
    policeLightsEffect(currentMillis);
  }
}

void wifiConnectionEffect() {
  if (wifiConnected) {
    Serial.println("Showing WIFI connected effect");
    for (uint8_t i = 0; i < 10; i++) {
      setLeds(ledBrightness * 255 / 100, 0, 255, 0);
      delay(50);
      setLeds(ledBrightness * 255 / 100, 0, 0, 0);
      delay(50);
    }
  } else {
    Serial.println("Showing WIFI problem effect");
    for (uint8_t i = 0; i < 5; i++) {
      setLeds(ledBrightness * 255 / 100, 255, 0, 0);
      delay(100);
      setLeds(ledBrightness * 255 / 100, 0, 0, 0);
      delay(100);
    }
  }
}

void colourPaletteEffect(uint32_t currentMillis) {
  if (currentMillis - lastColourPalleteMillis >= colourPalleteInterval) {
    lastColourPalleteMillis = currentMillis;
    hue++;
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
      leds[i] = CHSV(hue, 255, 255);
    }
    FastLED.setBrightness(ledBrightness);
    FastLED.show();
  }
}

void policeLightsEffect(uint32_t currentMillis) {
  if (currentMillis - lastPoliceLightsMillis >= policeLightsInterval) {
    lastPoliceLightsMillis = currentMillis;
    for (uint8_t i = 0; i < sizeof(columnLedStartingIndex); i++) {
      setPoliceLightsColumn(i, policeLightsPattern[policeLightsPatternIndex][i]);
    }
    FastLED.setBrightness(ledBrightness);
    FastLED.show();

    policeLightsPatternIndex++;
    if (policeLightsPatternIndex >= sizeof(policeLightsPattern) / sizeof(policeLightsPattern[0])) {
      policeLightsPatternIndex = 0;
    }
  }
}

void setPoliceLightsColumn(uint8_t columnIndex, uint8_t ledState) {
  uint8_t startLedIndex = columnLedStartingIndex[columnIndex];
  uint8_t endLedIndex = 0;
  if (columnIndex == sizeof(columnLedStartingIndex) / sizeof(columnLedStartingIndex[0]) - 1) {
    //this is the last column
    endLedIndex = NUM_LEDS - 1;
  } else {
    endLedIndex = columnLedStartingIndex[columnIndex + 1] - 1;
  }
  for (uint8_t j = startLedIndex; j <= endLedIndex; j++) {
    if (columnIndex < 3) {
      leds[j] = CRGB(ledState ? 255 : 0, 0, 0);
    } else {
      leds[j] = CRGB(0, 0, ledState ? 255 : 0);
    }
  }
}

void topButton_Clicked() {
  Serial.println("Top button clicked");
  increaseLedColourIndex();
}

void topButton_Pressed() {
  Serial.println("Top button pressed");
  if (ledBrightness == 0) {
    ledColourIndex = 0;
    while (ledBrightness < 10) {
      ledBrightness++;
      setLeds(ledBrightness * 255 / 100, ledColours[ledColourIndex][0], ledColours[ledColourIndex][1], ledColours[ledColourIndex][2]);
      delay(50);
    }
    louisLamp->setPercent(10);
  } else {
    while (ledBrightness > 0) {
      ledBrightness--;
      setLeds(ledBrightness * 255 / 100, ledColours[ledColourIndex][0], ledColours[ledColourIndex][1], ledColours[ledColourIndex][2]);
      delay(20);
    }
    louisLamp->setPercent(0);
  }
}

void bottomButton_Clicked() {
  Serial.println("Bottom button clicked");
  increaseLedBrightness();
}

void increaseLedBrightness() {
  ledBrightness += 10;
  if (ledBrightness > 100) {
    ledBrightness = 100;
  }
  louisLamp->setPercent(ledBrightness);
  setLeds(ledBrightness * 255 / 100, ledColours[ledColourIndex][0], ledColours[ledColourIndex][1], ledColours[ledColourIndex][2]);
}

void decreaseLedBrightness() {
  ledBrightness -= 10;
  if (ledBrightness < 10) {
    ledBrightness = 10;
  }
  louisLamp->setPercent(ledBrightness);
  setLeds(ledBrightness * 255 / 100, ledColours[ledColourIndex][0], ledColours[ledColourIndex][1], ledColours[ledColourIndex][2]);
}

void increaseLedColourIndex() {
  ledColourIndex++;
  if (ledColourIndex >= sizeof(ledColours) / sizeof(ledColours[0])) {
    ledColourIndex = 0;
  }
  louisLamp->setColor(ledColours[ledColourIndex][0], ledColours[ledColourIndex][1], ledColours[ledColourIndex][2]);
  setLeds(ledBrightness * 255 / 100, ledColours[ledColourIndex][0], ledColours[ledColourIndex][1], ledColours[ledColourIndex][2]);
}

void decreaseLedColourIndex() {
  ledColourIndex--;
  if (ledColourIndex < 0) {
    ledColourIndex = sizeof(ledColours) / sizeof(ledColours[0]) - 1;
  }
  louisLamp->setColor(ledColours[ledColourIndex][0], ledColours[ledColourIndex][1], ledColours[ledColourIndex][2]);
  setLeds(ledBrightness * 255 / 100, ledColours[ledColourIndex][0], ledColours[ledColourIndex][1], ledColours[ledColourIndex][2]);
}

void setLedColourIndex(uint8_t i) {
  ledColourIndex = i;
  louisLamp->setColor(ledColours[ledColourIndex][0], ledColours[ledColourIndex][1], ledColours[ledColourIndex][2]);
  setLeds(ledBrightness * 255 / 100, ledColours[ledColourIndex][0], ledColours[ledColourIndex][1], ledColours[ledColourIndex][2]);
}

void colorLightChanged(uint8_t brightness, uint32_t rgb) {
  ledBrightness = brightness * 100 / 255;
  float r = ((rgb >> 16) & 0xFF);
  float g = ((rgb >> 8) & 0xFF);
  float b = (rgb & 0xFF);
  setLeds(brightness, r, g, b);
  Serial.print("brightness:");
  Serial.print(ledBrightness);
  Serial.print("  ");
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

void IRAM_ATTR handleReceivedTinyIRData(uint16_t aAddress, uint8_t aCommand, bool isRepeat) {
  Serial.print(aAddress);
  Serial.print("  ");
  Serial.print(aCommand);
  Serial.print("  ");
  Serial.println(isRepeat);

  if (aAddress != 0 || isRepeat) {
    return;
  }

  switch (aCommand) {
    case 24:  //Up
      increaseLedBrightness();
      break;
    case 82:  //Down
      decreaseLedBrightness();
      break;
    case 90:  //Right
      increaseLedColourIndex();
      break;
    case 8:   //Left
      decreaseLedColourIndex();
      break;
    case 28:  //OK
      topButton_Pressed();
      break;
    case 22:  //Star
      setLedColourIndex(10);
      break;
    case 13:  //Sharp
      setLedColourIndex(11);
      break;
    case 69:  //1
      setLedColourIndex(1);
      break;
    case 70:  //2
      setLedColourIndex(2);
      break;
    case 71:  //3
      setLedColourIndex(3);
      break;
    case 68:  //4
      setLedColourIndex(4);
      break;
    case 64:  //5
      setLedColourIndex(5);
      break;
    case 67:  //6
      setLedColourIndex(6);
      break;
    case 7:   //7
      setLedColourIndex(7);
      break;
    case 21:  //8
      setLedColourIndex(8);
      break;
    case 9:   //9
      setLedColourIndex(9);
      break;
    case 25:  //0
      setLedColourIndex(0);
      break;
  }
}
