
/**
 * BasicHTTPClient.ino
 *
 *  Created on: 24.05.2015
 *
 */

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <ArduinoJson.h>

ESP8266WiFiMulti WiFiMulti;

const String endPoint = "http://146.185.179.198/";

void setup() {
    Serial.begin(115200);
    Serial.setTimeout(1000);

    WiFiMulti.addAP("Conn", "hardcore57868266");
    delay(1000);
}


void loop() {
    if((WiFiMulti.run() == WL_CONNECTED)) {
        
        if (Serial.available()>0) {
          String json = Serial.readStringUntil('\r\n');
          char charBuf[200];
          json.toCharArray(charBuf, 200);
          StaticJsonBuffer<200> jsonBuffer;
          
          JsonObject& root = jsonBuffer.parseObject(json);

          if (!root.success())
          {
            Serial.println("parseObject() failed");
            return;
          }

          String doParam = root["do"];

          if(doParam == "api/measurement") {
            HTTPClient http;
            //Serial.print("[HTTP] begin...\n");
            http.begin(endPoint+"api/measurement"); //HTTP
            http.addHeader("Content-Type", "application/json");
            // start connection and send HTTP header
            int httpCode = http.POST("{\"card_no\": \"123123\",\"value\": 99}");
    
            // httpCode will be negative on error
            if(httpCode > 0) {
                // HTTP header has been send and Server response header has been handled
                // Serial.printf("[HTTP] GET... code: %d\n", httpCode);
    
                // file found at server
                if(httpCode == HTTP_CODE_OK) {
                    String payload = http.getString();
                    Serial.println(payload);
                }
            } else {
                // Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
            }
  
             http.end();
           }
        }
    }

    delay(1000);
}

