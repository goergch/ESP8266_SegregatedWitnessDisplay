//copyright 2017 Christian Görg

#include "SegwitClient.h"


SegwitClient::SegwitClient(){

}

void SegwitClient::updateData(){
  JsonStreamingParser parser;
  parser.setListener(this);
  WiFiClient client;
  const char host[] = "api.blockchain.info";
  String url = "/charts/bip-9-segwit?timespan=10d";

  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  Serial.print("Requesting URL: ");
  Serial.println(url);

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

  int retryCounter = 0;
  while(!client.available()) {
    Serial.println(".");
    delay(1000);
    retryCounter++;
    if (retryCounter > 10) {
      return;
    }
  }

  int pos = 0;
  boolean isBody = false;
  char c;

  int size = 0;
  client.setNoDelay(false);
  while(client.connected()) {
    while((size = client.available()) > 0) {
      c = client.read();

      // Serial.print(c);
      if (c == '{' || c == '[') {
        isBody = true;
      }
      if (isBody) {
        parser.parse(c);
      }
    }
  }
  endDocument();
}

float SegwitClient::getCurrentPercentage(){
  return lastPercentage;
}

int SegwitClient::getValueCount(){
  return valueCount;
}
float SegwitClient::getValue(int index){
  if(index < valueCount){
    return values[index];
  }
  return 0;
}

float SegwitClient::getMinValue(){
  return minValue;
}
float SegwitClient::getMaxValue(){
  return maxValue;
}


void SegwitClient::startDocument(){
  currentArrayKey = "";
  currentKey = "";
  valueCount = 0;
  minValue = 100;
  maxValue = 0;
}

void SegwitClient::endDocument(){

}

void SegwitClient::key(String key){
  currentKey = key;
}

void SegwitClient::value(String value){
  if(currentArrayKey == "values" && currentKey == "y"){
    lastPercentage = value.toFloat();
    if (lastPercentage < minValue){
      minValue = lastPercentage;
    }
    if (lastPercentage > maxValue){
      maxValue = lastPercentage;
    }

    if(valueCount < MAX_VALUE_COUNT){
      values[valueCount] = lastPercentage;
      valueCount++;
    }
  }
}

void SegwitClient::startArray(){
  currentArrayKey = currentKey;
}

void SegwitClient::endArray(){
  currentArrayKey = "";
}

void SegwitClient::startObject(){

}

void SegwitClient::endObject(){

}

void SegwitClient::whitespace(char c) {

}
