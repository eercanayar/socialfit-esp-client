#include <HX711.h>
#include <MFRC522.h>
#include<LiquidCrystal.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

SoftwareSerial toESP(A1, A2); //rx, tx

#define SS_PIN 10
#define RST_PIN A0

// HX711.DOUT  - pin 2
// HX711.PD_SCK - pin 3
HX711 scale(2, 3);


MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

MFRC522::MIFARE_Key key;

//int burak[] = {18,219,115,213}; //This is the stored UID
//int emir[]= {131,88,152,244};
//int liste[2][4] = {{18, 219, 115, 213}, {131, 88, 152, 245}};
String uidString;
boolean Authorized = false;

unsigned long previousMillis = 0;        // will store last time LED was updated
unsigned long previousMillis2 = 0;        // will store last time LED was updated
const long interval = 250;           // interval at which to blink (milliseconds)
unsigned long LastActive = 0;       // En son sıfırdan büyük çüm zamanı.
const unsigned long MaxTime = 10 * 1000; //10saniye boyunca kimse ölçülmezse yetkiyi sıfırla.

long raw;
float weight = 0;
float weight_Store[5] = {0, 0, 0, 0, 0};
int count = 0;

String userName;

void setup() {
  lcd.begin(16, 2);
  Serial.begin(9600);
  toESP.begin(2400);
  toESP.setTimeout(1000);
  Serial.setTimeout(1000);
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522
  Weight();

}

void loop() {
  if (Authorized) {
    Update();
    lcd.clear();
    lcd.print("Hi! " + userName);
  } else {
    lcd.clear();
    lcd.print("Place your card");
  }
  lcd.setCursor(0, 1);
  lcd.print(weight);
  lcd.print(" Kg");
  Weight();

  if (  rfid.PICC_IsNewCardPresent())
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Please wait...");
    readRFID();
  }
}

void readRFID()
{

  rfid.PICC_ReadCardSerial();
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    lcd.clear();
    lcd.print("Your tas is not of");
    lcd.setCursor(0, 1);
    lcd.print("type MIFARE Classic");
    delay(2000);
    return;
  }


  uidString = String(rfid.uid.uidByte[0]) + String(rfid.uid.uidByte[1]) + String(rfid.uid.uidByte[2]) + String(rfid.uid.uidByte[3]);
  Serial.print("Card ID :");
  Serial.println(uidString);
  boolean statue = true;
  int i = 0;
  do {
    toESP.println("{\"do\":\"namebycard\",\"RFID\":\"" + uidString + "\"}");
    Serial.print("Sending to ESP: ");
    Serial.println("{\"do\":\"namebycard\",\"RFID\":\"" + uidString + "\"}");
    //root.printTo(Serial);
    delay(500);

    while (1) {
      if (toESP.available() > 0) {
        break;
      }
      delay(100);
    }
    String json = toESP.readStringUntil('\r\n');
    Serial.print("Recieved from ESP : ");
    Serial.println(json);
    char charBuf[200];

    json.toCharArray(charBuf, 200);

    StaticJsonBuffer<200> jsonBufferincoming;

    JsonObject& root2 = jsonBufferincoming.parseObject(json);

    if (!root2.success()) {
      statue = true;


      Serial.println("Result: parseObject() failed");

    } else {
      statue = false;
      boolean successParam = root2["success"];

      if (successParam == true) {
        Authorized = true;
        lcd.clear();
        String userName2 = root2["name"];
        userName = userName2;
        LastActive = millis();
        lcd.setCursor(0, 0);
        lcd.print("Welcome,");//uidString);
        lcd.setCursor(0, 1);
        lcd.print(userName);//uidString);
        Serial.print("Logged in username: ");
        Serial.println(userName);
        delay(2000);
      } else {
        Authorized = false;
        if (root2["error"] == "USER_NOT_FOUND") {
          lcd.clear();
          lcd.print("Please sign up");
          lcd.setCursor(0, 1);
          lcd.print("via web");
          Serial.println("Result: New Card");
          delay(2000);
          lcd.clear();
        } else if (root2["error"] == "ESP_PARSE_ERROR") {
          statue = true;

        } else if (root2["error"] == "ESP_WIFI_ERROR") {
          statue = false;
          Serial.println("Result: No wifi at ESP");
          lcd.clear();
          lcd.print("Error :");
          lcd.setCursor(0, 1);
          lcd.print("WIFI Connection");
          delay(2000);
          lcd.clear();
        }
      }
    }
    json = "";
    charBuf[0] = '\0';
    i++;
  } while (statue && i < 5);

  if (i == 5) {
    lcd.clear();
    lcd.print("Connection Error");
    Serial.println("Result: Too many try");
    delay(1000);
  }



  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}

float Weight() {

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    raw = scale.read_average(5);
    weight = (raw - 116888) / 15391.0;
  }
  if (weight < 1.0) {
    weight = 0;
    if (Authorized) {
      if (currentMillis - LastActive > MaxTime) {
        Authorized = false;
        Serial.println("Timeout. User logged off");
      }
    }
  } else {
    LastActive = currentMillis;
  }
  return weight;
}

void Update() {

  unsigned long currentMillis2 = millis();

  if (currentMillis2 - previousMillis2 >= interval) {
    // save the last time you blinked the LED
    previousMillis2 = currentMillis2;
    boolean error = false;
    float sum = 0;
    if (weight > 0) {
      for (int i = 4; i > 0; i--) {
        weight_Store[i] = weight_Store[i - 1];
        sum += weight_Store[i];
      }
      weight_Store[0] = weight;
      sum += weight_Store[0];
      count++;
      if (count > 4) {
        float weight_Average = sum / 5;
        boolean is_OK = true;
        for (int i = 4; i > 0; i--) {
          if ((weight_Store[i] / weight_Average) > 1.01 || (weight_Store[i] / weight_Average) < 0.91) {
            is_OK = false;
            break;
          }
        }
        if (is_OK) {
          lcd.clear();
          lcd.print("Saving...");
          lcd.setCursor(0, 1);
          lcd.print(weight_Average);
          lcd.print(" Kg");

          boolean statue = true;
          int i = 0;
          do {

            toESP.println("{\"do\":\"measurement\",\"RFID\":\"" + uidString + "\",\"value\":" + weight_Average + "}");
            Serial.print("Sending to ESP: ");
            Serial.println("{\"do\":\"measurement\",\"RFID\":\"" + uidString + "\",\"value\":" + weight_Average + "}");
            delay(500);
            while (1) {
              if (toESP.available() > 0) {
                break;
              }
              delay(100);
            }
            String json = toESP.readStringUntil('\r\n');
            Serial.print("Received from ESP: ");
            Serial.println(json);
            char charBuf[200];

            json.toCharArray(charBuf, 200);

            StaticJsonBuffer<200> jsonBufferincoming;


            JsonObject& root2 = jsonBufferincoming.parseObject(json);

            if (!root2.success())

            {

              Serial.println("Result: parseObject() failed");
              statue = true;
              json = "";
              charBuf[0] = '\0';
              continue;

            }
            boolean successParam = root2["success"];
            Serial.print("Success status: ");
            Serial.println(successParam);
            if (successParam == true) {
              statue = false;
            } else {
              if (root2["error"] == "ESP_PARSE_ERROR") {
                statue = true;

              } else if (root2["error"] == "ESP_WIFI_ERROR") {
                statue = false;
                error = true;
                lcd.clear();
                Serial.println("Result: No wifi at ESP");
                lcd.print("Error :");
                lcd.setCursor(0, 1);
                lcd.print("WIFI Connection");
                delay(2000);
                lcd.clear();
              }
            }

            json = "";
            charBuf[0] = '\0';
            i++;

          } while (statue && (i < 5));
          if (error == false) {
            if (i == 5) {
              lcd.clear();
              lcd.print("Connection Error");
              Serial.println("Result: Too many try");
            } else {
              lcd.setCursor(0, 0);
              lcd.print("Your data has");
              lcd.setCursor(0,1);
              lcd.print("been saved!");
              Authorized = false;
              is_OK = false;
            }
          } else {
            is_OK = false;
          }
          delay(2000);
        }
        count = 0;
      }
    } else {
      for (int i = 4; i > 0; i--) {
        weight_Store[i] = 0;
      }
    }
  }
  return;
}


