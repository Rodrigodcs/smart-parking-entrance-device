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
      rfidReader("/check-in");
    }else if(option == 1){
      rfidReader("/check-out");
    }else if(option == 2){
      rfidReader("/clients/tag");
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
  if (rfid.PICC_IsNewCardPresent()) { // New tag present
    if (rfid.PICC_ReadCardSerial()) { // NUID readed
      
      buzzerSound("read");
      lcdClear();
      lcdWrite(0,"Enviando tag...");

      MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);

      String id = "";
      for (int i = 0; i < rfid.uid.size; i++) {
        id+=rfid.uid.uidByte[i] < 0x10 ? "0" : "";
        id+=String(rfid.uid.uidByte[i], HEX);
      }
      id.toUpperCase();
      Serial.println();
      Serial.println("NUID_Hex: " + id);

      rfid.PICC_HaltA(); // halt PICC
      rfid.PCD_StopCrypto1(); // stop encryption on PCD
      
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
      Serial.println("httpCODE: " + httpCode);
      Serial.println("PAYLOAD: " + payload);

      buzzerSound(httpCode==200 || httpCode==201 || httpCode==202?"sucess":"error");
      
      lcdClear();
      if(option == 0){
        switch(httpCode){
          case 200: lcdWrite(1,"BEM VINDO");
          break;
          case 409: lcdWrite(1,"Não cadastrado");
          break;
          case 401: lcdWrite(1,"Em débito");
          break;
          case 402: lcdWrite(1,"Fazer check out");
          break;
        }
      }else if(option == 1){
        switch(httpCode){
          case 200: lcdWrite(1,"VOLTE SEMPRE");
          break;
          case 409: lcdWrite(1,"Não cadastrado");
          break;
          case 402: lcdWrite(1,"Fazer check in");
          break;
        }
      }else if(option == 2){
        switch(httpCode){
          case 200: lcdWrite(1,"VER CLIENTE");
          break;
          case 201: lcdWrite(1,"TAG CADASTRADA");
          break;
          case 202: lcdWrite(1,"CRED ATUALIZADO");
          break;
          case 408: lcdWrite(1,"Já cadastrada");
          break;
          case 409: lcdWrite(1,"Não cadastrado");
          break;
          case 405: lcdWrite(1,"Falha cliente");
          break;
        }
      }

    }else{
      Serial.println("Error on HTTP request");
    }
    http.end(); //Free the resources
  }else{
    Serial.println("Lost internet connection");
  }
  delay(3000);
  lcdClear();
  lcdWrite(1,"AGUARDANDO TAG");
}

void lcdPrintMode(String message){
  lcd.setCursor(15,0);
  lcd.print(message);
}

void lcdWrite(int line,String message){
  lcd.setCursor(0,line);
  lcd.print(message);
}

void lcdClear(){
  lcd.setCursor(0,0);
  lcd.print("               ");
  lcd.setCursor(0,1);
  lcd.print("               ");
}

void buzzerSound(String type){
  if(type=="sucess"){
    digitalWrite(LED_GREEN,HIGH);
    digitalWrite(BUZZER,HIGH);
    delay(75);
    digitalWrite(BUZZER,LOW);
    delay(50);
    digitalWrite(BUZZER,HIGH);
    delay(75);
    digitalWrite(BUZZER,LOW);
    delay(200);
    digitalWrite(LED_GREEN,LOW);
  }
  if(type=="error"){
    digitalWrite(LED_RED,HIGH);
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
    digitalWrite(LED_RED,LOW);
  }
  if(type=="read"){
    digitalWrite(LED_GREEN,HIGH);
    digitalWrite(BUZZER,HIGH);
    delay(300);
    digitalWrite(BUZZER,LOW);
    digitalWrite(LED_GREEN,LOW);
  }
}
