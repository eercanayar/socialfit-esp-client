
/**
   BasicHTTPClient.ino

    Created on: 24.05.2015

*/

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <ArduinoJson.h>

#include <SoftwareSerial.h>

SoftwareSerial TO_SERIAL(5, 4); // RX, TX

ESP8266WiFiMulti WiFiMulti;

const String endPoint = "http://146.185.179.198/api/";

void setup() {
  TO_SERIAL.begin(2400);
  TO_SERIAL.setTimeout(1000);

  Serial.begin(2400);
  Serial.setTimeout(1000);

  WiFiMulti.addAP("Emir", "emir123123");
  delay(1000);
}

void loop() {

  if (TO_SERIAL.available() > 0) {
    String json = TO_SERIAL.readStringUntil('\r\n');
    Serial.println(json);
    char charBuf[200];
    json.toCharArray(charBuf, 200);
    StaticJsonBuffer<200> jsonBuffer;

    JsonObject& root = jsonBuffer.parseObject(json);

    if (!root.success())
    {
      TO_SERIAL.println("{\"success\":false,\"error\":\"ESP_PARSE_ERROR\"}");
      return;
    }

    if ((WiFiMulti.run() == WL_CONNECTED)) {

      String doParam = root["do"];

      if (doParam == "namebycard") {
        HTTPClient http;

        String RFIDParam = root["RFID"];

        http.begin(endPoint + "namebycard?card_no=" + RFIDParam);
        int httpCode = http.GET();

        if (httpCode > 0) {
          if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            TO_SERIAL.println(payload);
            Serial.println(payload);
          }
        } else {
          TO_SERIAL.printf("{\"success\":false, \"error\": \"%s\"}\r\n", http.errorToString(httpCode).c_str());
        }

        http.end();
      } else if (doParam == "measurement") {

        HTTPClient http;

        String RFIDParam = root["RFID"];
        String valueParam = root["value"];

        http.begin(endPoint + "measurement");
        http.addHeader("Content-Type", "application/json");
        int httpCode = http.POST("{\"card_no\": \"" + RFIDParam + "\",\"value\": " + valueParam + "}");

        if (httpCode > 0) {
          if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            TO_SERIAL.println(payload);
            Serial.println(payload);
          }
        } else {
          TO_SERIAL.printf("{\"success\":false, \"error\": \"%s\"}\r\n", http.errorToString(httpCode).c_str());
        }

        http.end();
      }
    } else {
      TO_SERIAL.println("{\"success\":false, \"error\": \"ESP_WIFI_ERROR\"}");
    }
  }

delay(1000);
}

