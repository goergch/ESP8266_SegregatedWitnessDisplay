//copyright 2017 Christian Görg
#pragma once

#include <JsonListener.h>
#include <JsonStreamingParser.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#define MAX_VALUE_COUNT 250

class SegwitClient: public JsonListener {
private:
  int counter = 0;
  String currentKey = "";
  String currentArrayKey = "";
  float lastPercentage = 0;

  int valueCount = 0;
  float minValue = 100;
  float maxValue = 0;
  float values[MAX_VALUE_COUNT];

public:
  SegwitClient();
  void updateData();
  float getCurrentPercentage();
  int getValueCount();
  float getMinValue();
  float getMaxValue();
  float getValue(int index);

  virtual void whitespace(char c);

  virtual void startDocument();

  virtual void key(String key);

  virtual void value(String value);

  virtual void endArray();

  virtual void endObject();

  virtual void endDocument();

  virtual void startArray();

  virtual void startObject();
};
