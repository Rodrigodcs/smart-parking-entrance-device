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
const String URL = "https://smart-parking-api.onrender.com";

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
  lcdClear();
  lcdWrite("CONECTANDO...","");
  while(WiFi.status() != WL_CONNECTED){
    delay(300);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to the WiFi network");
  lcdClear();
  lcdWrite("CONEXAO","ESTABELECIDA");
  delay(2000);
  lcdClear();
  lcdPrintMode("A");
  lcdWrite("AGUARDANDO TAG","");
}

int timer = 0;
bool alreadyChanged = false;
int option = 2;

int ligado = true;

void loop() {
  if(ligado){
    checkButtonPress();
    if(option == 0){
      rfidReader("/esp/checkIn");
    }else if(option == 1){
      rfidReader("/esp/checkOut");
    }else if(option == 2){
      rfidReader("/admin/esp/tag");
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
      lcdWrite("Enviando tag...","");

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
      
      lcdClear();
      if(option == 0){
        switch(httpCode){
          case 200: lcdWrite("BEM VINDO","");
          break;
          case 403: lcdWrite("TAG NAO","CADASTRADA");
          break;
          case 422: lcdWrite("TAG NAO","CADASTRADA");
          break;
          case 401: lcdWrite("CREDITOS","INSUFICIENTES");
          break;
          case 405: lcdWrite("E PRECISO FAZER","CHECK OUT");
          break;
          case 406: lcdWrite("VAGAS","ESGOTADAS");
          break;
          case 500: lcdWrite("ERRO NO","SERVIDOR");
          break;
        }
      }else if(option == 1){
        switch(httpCode){
          case 200: lcdWrite("VOLTE SEMPRE","");
          break;
          case 403: lcdWrite("TAG NAO","CADASTRADA");
          break;
          case 422: lcdWrite("TAG NAO","CADASTRADA");
          break;
          case 405: lcdWrite("E PRECISO FAZER","CHECK IN");
          break;
          case 500: lcdWrite("ERRO NO","SERVIDOR");
          break;
        }
      }else if(option == 2){
        switch(httpCode){
          case 200: lcdWrite("SUCESSO","");
          break;
          case 201: lcdWrite("REGISTRO DE TAG","EFETUADO");
          break;
          case 422: lcdWrite("TAG NAO","CADASTRADA");
          break;
          case 409: lcdWrite("TAG REGISTRADA","POR OUTRO");
          break;
          case 401: lcdWrite("FALHA NO PEDIDO","PELO ADMIN");
          break;
          case 500: lcdWrite("ERRO NO","SERVIDOR");
          break;
        }
      }
      
      buzzerSound(httpCode==200 || httpCode==201 || httpCode==202?"sucess":"error");

    }else{
      Serial.println("Error on HTTP request");
    }
    http.end(); //Free the resources
  }else{
    Serial.println("Lost internet connection");
  }
  delay(3000);
  lcdClear();
  lcdWrite("AGUARDANDO TAG","");
}

void lcdPrintMode(String message){
  lcd.setCursor(15,1);
  lcd.print(message);
}

void lcdWrite(String firstLine,String secondLine){
  lcd.setCursor(0,0);
  lcd.print(firstLine);
  lcd.setCursor(0,1);
  lcd.print(secondLine);
}

void lcdClear(){
  lcd.setCursor(0,0);
  lcd.print("                ");
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
