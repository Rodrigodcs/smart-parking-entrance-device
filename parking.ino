/*PARKING PROJECT*/
#include <SPI.h>
#include <MFRC522.h>

#include <LiquidCrystal_I2C.h>

#include <WiFi.h>

#include <HTTPClient.h>
#include <ArduinoJson.h>

//RFID
#define SS_PIN  5  // ESP32 pin GIOP5 
#define RST_PIN 27 // ESP32 pin GIOP27 
MFRC522 rfid(SS_PIN, RST_PIN);

//LCD
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

#define BUZZER 13

#define BUTTON 34

#define LED_GREEN 33
#define LED_RED 32

//WIFI
const char* ssid = "CERTTO-2BDA6";
const char* password = "rdcsmgf1";

//HTTP
const String URL = "https://api-parking.onrender.com";

void setup() {
  Serial.begin(9600);
  delay(3000);

  //RFID SETUP
  SPI.begin(); // init SPI bus
  rfid.PCD_Init(); // init MFRC522

  //LCD SETUP
  lcd.init();                      
  lcd.backlight();

  //BUZZER SETUP
  pinMode(BUZZER,OUTPUT);

  //LEDS
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  //BUTTON
  pinMode(BUTTON, INPUT);

  //WIFI SETUP
  WiFi.begin(ssid,password);
  Serial.print("Connecting to WiFi");
  while(WiFi.status() != WL_CONNECTED){
    delay(300);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to the WiFi network");
  
  lcdPrintMode("A");
}

int timer = 0;
bool alreadyChanged = false;
int option = 2;

int ligado = true;

void loop() {
  if(ligado){
    checkButtonPress();
    if(option == 0){
      rfidReader("/test/esp");
    }else if(option == 1){
      
    }else if(option == 2){
      
    }
  }
}

void checkButtonPress(){
  if(!digitalRead(BUTTON)){
    if(!alreadyChanged){
      delay(100);
      alreadyChanged = true;
      option++;
      if(option == 3){
        option = 0;
      }
      if(option == 0){
        lcdPrintMode("I");
      }else if(option == 1){
        lcdPrintMode("O");
      }else if(option == 2){
        lcdPrintMode("A");
      }
    }
  }else{
    if(alreadyChanged){
      delay(100);
    }
    alreadyChanged = false;
  }
}

void rfidReader(String path){
  String id = "";
  if (rfid.PICC_IsNewCardPresent()) { // new tag is available
    if (rfid.PICC_ReadCardSerial()) { // NUID has been readed
      buzzerSound("read");
      
      lcdClear(0);
      lcdWrite(0,"Enviando id...");
      MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
      //Serial.print("RFID/NFC Tag Type: ");
      //Serial.println(rfid.PICC_GetTypeName(piccType));

      for (int i = 0; i < rfid.uid.size; i++) {
        id+=rfid.uid.uidByte[i] < 0x10 ? "0" : "";
        id+=String(rfid.uid.uidByte[i], HEX);
      }
      Serial.println();

      rfid.PICC_HaltA(); // halt PICC
      rfid.PCD_StopCrypto1(); // stop encryption on PCD
      id.toUpperCase();
      Serial.println("idHex: "+id);
      sendId(path, id);
    }
  }
}

void sendId(String path,String id){
  if ((WiFi.status() == WL_CONNECTED)) {
    HTTPClient http;
    http.begin(URL+path);
    http.addHeader("Content-Type","text/plain");
    
    int httpCode = http.POST(id);
    
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("httpCODE");
      Serial.println(String(httpCode));
      Serial.println("PAYLOAD");
      Serial.println(payload);
      buzzerSound(httpCode==200?"sucess":"error");
    }else{
      Serial.println("Error on HTTP request");
    }
    http.end(); //Free the resources
    lcdClear(0);
  }else{
    Serial.println("Lost internet connection");
    
  }
  delay(5000);
}

void lcdPrintMode(String message){
  lcd.setCursor(15,0);
  lcd.print(message);
}

void lcdWrite(int line,String message){
  lcd.setCursor(0,line);
  lcd.print(message);
}

void lcdClear(int line){
  lcd.setCursor(0,line);
  lcd.print("               ");
}

void buzzerSound(String type){
  if(type=="error"){
    digitalWrite(LED_RED,HIGH);
    digitalWrite(BUZZER,HIGH);
    delay(75);
    digitalWrite(BUZZER,LOW);
    delay(50);
    digitalWrite(BUZZER,HIGH);
    delay(75);
    digitalWrite(BUZZER,LOW);
    delay(200);
    digitalWrite(LED_RED,LOW);
  }
  if(type=="sucess"){
    digitalWrite(LED_GREEN,HIGH);
    digitalWrite(BUZZER,HIGH);
    delay(100);
    digitalWrite(BUZZER,LOW);
    delay(100);
    digitalWrite(BUZZER,HIGH);
    delay(100);
    digitalWrite(BUZZER,LOW);
    delay(100);
    digitalWrite(BUZZER,HIGH);
    delay(100);
    digitalWrite(BUZZER,LOW);
    digitalWrite(LED_GREEN,LOW);
  }
  if(type=="read"){
    digitalWrite(LED_GREEN,HIGH);
    digitalWrite(BUZZER,HIGH);
    delay(600);
    digitalWrite(BUZZER,LOW);
    digitalWrite(LED_GREEN,LOW);
  }
}
