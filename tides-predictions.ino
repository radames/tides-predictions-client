#include "parameters.h"
#include "WiFiPass.h"
#include <ESP8266WiFi.h>
#include <NTPClient.h> //NTPClient by Arduino
#include <WiFiUdp.h>
#include <RTClib.h> //RTClib by Adafruit
#include <ArduinoJson.h>

#include <WiFiClientSecure.h>

WiFiUDP ntpUDP;
// You can specify the time server pool and the offset, (in seconds)
// additionaly you can specify the update interval (in milliseconds).
NTPClient timeClient(ntpUDP, NTP_SERVER, GMT_TIME_ZONE * 3600 , 60000);

int timeUpdated = 0;
long lastPrintTime = 0;

enum EventState {EVENT1, EVENT2, AMBIENT, RESET, WAITING};
EventState nState = RESET;


void setup() {
  Serial.begin(9600);

  WiFi.begin(ssid, password);
  delay(10000);
  timeClient.begin();
  timeClient.update();
  timeClient.update();

  logDateTime();

  WiFiClientSecure client;
  Serial.print("connecting to ");
  Serial.println(host);
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return;
  }
  if (client.verify(fingerprint, host)) {
    Serial.println("certificate matches");
  } else {
    Serial.println("certificate doesn't match");
  }

  Serial.print("requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");

  Serial.println("request sent");
  int iContentLength = 0;
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    Serial.println(line);
    if (line.substring(0, 15) == "Content-Length:") {
      Serial.print("CONTENT: ");
      String cnt = line.substring(15);
      iContentLength = cnt.toInt();
    }
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  String data = "";
  if (iContentLength > 0) {
    while (iContentLength-- > 0) {
      data += (char)client.read();
    }
  }
  Serial.println("DATA");
  Serial.println(data);

  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(data);
  //
  if (!root.success()) {
    Serial.println("parseObject() failed");
    return;
  }
  //
  //
  for (int i = 0; i < root["predictions"].size(); i++) {
    const char* t = root["predictions"][i]["t"];
    const char* v = root["predictions"][i]["v"];
    const char* type = root["predictions"][i]["type"];
    Serial.print("ITEM: ");
    Serial.println(i);
    Serial.println(t); // time
    Serial.println(v); // amplitude
    Serial.println(type); // type 
  }
}


void loop () {


#ifdef DEBUG_MODE
  if (millis() - lastPrintTime > 1000) { //po
    lastPrintTime = millis();
    logDateTime();
  }
#endif

}




void logDateTime(void) {
  timeClient.update();
  DateTime now = (DateTime)timeClient.getEpochTime();
  Serial.print(now.year(), DEC); Serial.print('/');
  Serial.print(now.month(), DEC); Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print("  ");
  Serial.print(now.hour(), DEC); Serial.print(':');
  Serial.print(now.minute(), DEC); Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.print("  ");
  Serial.println(now.unixtime());
}

