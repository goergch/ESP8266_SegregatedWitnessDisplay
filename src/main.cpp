//copyright 2017 Christian Görg
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <JsonListener.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include "SegwitClient.h"
#include "SSD1306.h"
#include "OLEDDisplayUi.h"
#include "Wire.h"

#include "TimeClient.h"
#include "NTPClient.h"

#define HOSTNAME "ESP8266-OTA-"

const int UTC_OFFSET = 2;
const int I2C_DISPLAY_ADDRESS = 0x3c;
const int SDA_PIN = D3;
const int SDC_PIN = D4;
SSD1306     display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN);
OLEDDisplayUi   ui( &display );
TimeClient timeClient(UTC_OFFSET);
NTPClient ntpClient("ptbtime1.ptb.de",UTC_OFFSET);
SegwitClient segwitClient;

void drawOtaProgress(unsigned int, unsigned int);
void configModeCallback (WiFiManager *myWiFiManager);
void draw1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void draw2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void draw3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void checkReadyForUpdate();
void updateData();
char* remainingTimeString = "xxx Tage";

const long segwitEpoch = 1510747200;

FrameCallback frames[] = { draw1, draw2, draw3 };
int numberOfFrames = 3;

const int UPDATE_INTERVAL = 600;
bool readyForUpdate = false;
Ticker ticker;

long lastUpdate = 0;



void setup() {
    // Turn On VCC
  // pinMode(D4, OUTPUT);
  // digitalWrite(D4, HIGH);
  Serial.begin(115200);

  // initialize dispaly
  display.init();
  display.clear();
  display.display();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setContrast(255);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  // Uncomment for testing wifi manager
  //wifiManager.resetSettings();
  wifiManager.setAPCallback(configModeCallback);

  //or use this for auto generated name ESP + ChipID
  wifiManager.autoConnect();

  //Manual Wifi
  // WiFi.begin("@home4us", "IZj0EYxouSeH");
  String hostname(HOSTNAME);
  hostname += String(ESP.getChipId(), HEX);
  WiFi.hostname(hostname);


  int counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    switch(WiFi.status() ){
      case WL_CONNECTED:
        Serial.print("WL_CONNECTED");
        break;
      case WL_IDLE_STATUS:
        Serial.print("WL_IDLE_STATUS");
        break;
      case WL_NO_SSID_AVAIL:
        Serial.print("WL_NO_SSID_AVAIL");
        break;
      case WL_SCAN_COMPLETED:
        Serial.print("WL_SCAN_COMPLETED");
        break;
      case WL_CONNECT_FAILED:
        Serial.print("WL_CONNECT_FAILED");
        break;
      case WL_CONNECTION_LOST:
        Serial.print("WL_CONNECT_FAILED");
        break;
      case WL_DISCONNECTED:
        Serial.print("WL_CONNECT_FAILED");
        break;
    }
    Serial.print(".");
    display.clear();
    display.drawString(64, 10, "Connecting to WiFi");
    // display.drawXbm(46, 30, 8, 8, counter % 3 == 0 ? activeSymbol : inactiveSymbol);
    // display.drawXbm(60, 30, 8, 8, counter % 3 == 1 ? activeSymbol : inactiveSymbol);
    // display.drawXbm(74, 30, 8, 8, counter % 3 == 2 ? activeSymbol : inactiveSymbol);
    display.display();

  }
  ui.setTargetFPS(30);

  //Hack until disableIndicator works:
  //Set an empty symbol
  // ui.setActiveSymbol(emptySymbol);
  // ui.setInactiveSymbol(emptySymbol);
  ui.disableAllIndicators();
  // ui.setIndicatorPosition(BOTTOM);
  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_TOP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_LEFT);

  ui.setFrames(frames, numberOfFrames);

  // ui.setOverlays(overlays, numberOfOverlays);
  // Inital UI takes care of initalising the display too.

  ui.init();
  display.flipScreenVertically();

  // Setup OTA
  Serial.println("Hostname: " + hostname);
  ArduinoOTA.setHostname((const char *)hostname.c_str());
  ArduinoOTA.onProgress(drawOtaProgress);
  ArduinoOTA.begin();
  ntpClient.begin();
  updateData();

  //Check every second
  ticker.attach(UPDATE_INTERVAL, checkReadyForUpdate);

}

void loop() {
  // If there are airplanes query often
  // if (adsbClient.getNumberOfVisibleAircrafts() == 0) {
  //   currentUpdateInterval = UPDATE_INTERVAL_SECS_LONG;
  // } else {
  //   currentUpdateInterval = UPDATE_INTERVAL_SECS_SHORT;
  // }

  if (readyForUpdate && ui.getUiState()->frameState == FIXED) {
    updateData();
  }

  int remainingTimeBudget = ui.update();

  if (remainingTimeBudget > 0) {
    // You can do some work here
    // Don't do stuff if you are below your
    // time budget.
    ArduinoOTA.handle();
    delay(remainingTimeBudget);
  }


}


void drawOtaProgress(unsigned int progress, unsigned int total) {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 10, "OTA Update");
  display.drawProgressBar(2, 28, 124, 10, progress / (total / 100));
  display.display();
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 10, "Wifi Manager");
  display.drawString(64, 20, "Please connect to AP");
  display.drawString(64, 30, myWiFiManager->getConfigPortalSSID());
  display.drawString(64, 40, "To setup Wifi Configuration");
  display.display();
}

void draw1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_24);

  display->drawString(0 + x, 0 + y, "Pro Segwit");
  display->drawString(0 + x, 30 + y, String(segwitClient.getCurrentPercentage(),5) + "%");
}
void draw3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_24);


  display->drawString(0 + x, 0 + y, remainingTimeString);
  display->drawString(0 + x, 26 + y, "verbleibend");
}

void drawDiagram(OLEDDisplay *display,int16_t x, int16_t y, int width, int height){
  display->drawLine(0+x,63+y,127+x,63+y);
  display->drawLine(0+x,63+y,0+x,0+y);

  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(width + x, 0 + y, "10-Tage-Verlauf");
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(1 + x, 0 + y, String(segwitClient.getMaxValue())+ "%");
  display->drawString(1 + x, height - 12 + y, String(segwitClient.getMinValue())+ "%");

  int count  = segwitClient.getValueCount();

  float valuesPerPix = (float)count / (float)(width-1);
  float percPerPix = (segwitClient.getMaxValue()-segwitClient.getMinValue()) / (float)(height-1);
  // Serial.println("minVal " + String(segwitClient.getMinValue()));
  // Serial.println("maxVal " + String(segwitClient.getMaxValue()));
  if(valuesPerPix > 1){
    float valueBefore = segwitClient.getValue(0);
    for(int i = 1; i<(width-1);i++){
      float currentValue = 0;
      int currentCount = 0;
      for(int j = i*valuesPerPix; j < (i+1)*valuesPerPix;j++ ){
        currentValue += segwitClient.getValue(j);
        currentCount++;
      }
      currentValue /= currentCount;
      // Serial.println(currentValue);
      int y1 = y + (height-1) - (valueBefore-segwitClient.getMinValue())/percPerPix ;
      int y2 = y + (height-1) - (currentValue-segwitClient.getMinValue())/percPerPix ;

      display->drawLine(x+(i-1),y1, x+(i),y2 );
      valueBefore = currentValue;
    }


  }


}

void draw2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {

  drawDiagram(display,x, y, 128, 64);
}

void checkReadyForUpdate() {
  // Only do light work in ticker callback
  readyForUpdate = true;
}

void updateData() {
  readyForUpdate = false;
  timeClient.updateTime();
  ntpClient.update();

  long diff = segwitEpoch - ntpClient.getRawTime();
  // sprintf(remainingTimeString,"%d Tage",(diff/60/60/24));
  sprintf(remainingTimeString,"%d Tage",diff / 3600 / 24 );

  segwitClient.updateData();

  lastUpdate = millis();
}
