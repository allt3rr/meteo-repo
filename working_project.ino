/*
  Projekt stacji meteo: RFID RC522 (SPI) + Czujnik BME280 (SPI na CS: 15) + Wyświetlacz OLED SH1106 (I2C)
*/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Biblioteki dla RFID (MFRC522v2)
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>

// Biblioteki dla czujnika BME280
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// Biblioteki dla zewnetrznego czujnika temperatury DS18B20
#include <OneWire.h>
#include <DallasTemperature.h>

// Webserver biblioteki
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <NetworkClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

WiFiMulti wifiMulti;
WebServer server(80);

// Konfiguracja ekranu OLED (I2C)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Wspólne piny magistrali SPI dla RFID oraz BME280
#define SPI_SCK  18
#define SPI_MISO 19
#define SPI_MOSI 23

// Osobne piny CS (Chip Select) dla każdego urządzenia SPI
#define RFID_CS   5  // Pin CS dla czytnika RFID
#define BME_CS   15  // Twój wybrany pin CS dla BME280

// Pin dla OneWire
#define ONE_WIRE_BUS 27

// Inicjalizacja BME280 w trybie Hardware SPI przy użyciu dedykowanego pinu CS
Adafruit_BME280 bme(BME_CS); 

// Inicjalizacja RFID
MFRC522DriverPinSimple ss_pin(RFID_CS);
MFRC522DriverSPI driver{ss_pin};
MFRC522 mfrc522{driver};

// Inicjalizacja OneWire
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

unsigned long prevTime = 0;
const long refreshInterval = 2000;
char temp[1200];

const int led = 13;

void handleRoot() {
  digitalWrite(led, 1);

  sensors.requestTemperatures();
  float outdoorTemp = sensors.getTempCByIndex(0);
  float indoorTemp = bme.readTemperature();
  float hum = bme.readHumidity();
  float pres = bme.readPressure() / 100.0F;

  snprintf(
    temp, 1200,

    "<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <meta charset='UTF-8'>\
    <title>Stacja Meteo</title>\
    <style>\
      body { background-color: #121212; font-family: 'Segoe UI', Arial, sans-serif; color: #e0e0e0; margin: 0; padding: 20px; display: flex; justify-content: center; }\
      .container { max-width: 500px; width: 100%%; background: #1e1e1e; padding: 25px; border-radius: 12px; box-shadow: 0 4px 15px rgba(0,0,0,0.5); }\
      h1 { text-align: center; color: #00adb5; font-size: 24px; margin-bottom: 25px; text-transform: uppercase; letter-spacing: 1px; }\
      .param-box { display: flex; justify-content: space-between; align-items: center; padding: 12px 0; border-bottom: 1px solid #2d2d2d; }\
      .param-box:last-child { border-bottom: none; }\
      .label { font-size: 16px; color: #aaaaaa; }\
      .value { font-size: 18px; font-weight: bold; color: #ffffff; }\
      .rfid { color: #ffb400; font-family: monospace; }\
    </style>\
  </head>\
  <body>\
    <div class='container'>\
      <h1>Stacja Meteo</h1>\
      <div class='param-box'><span class='label'>Temp. Wewnętrzna:</span><span class='value'>%.1f &deg;C</span></div>\
      <div class='param-box'><span class='label'>Temp. Zewnętrzna:</span><span class='value'>%.1f &deg;C</span></div>\
      <div class='param-box'><span class='label'>Wilgotność:</span><span class='value'>%.0f %%</span></div>\
      <div class='param-box'><span class='label'>Ciśnienie:</span><span class='value'>%.0f hPa</span></div>\
    </div>\
  </body>\
</html>",
    indoorTemp, outdoorTemp, hum, pres
  );
  server.send(200, "text/html", temp);
  digitalWrite(led, 0);
}

void handleNotFound() {
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (int i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void setup() {
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  while (!Serial);

  WiFi.mode(WIFI_STA);
  wifiMulti.addAP("ssid", "passwrd");
  wifiMulti.addAP("ssid", "passwrd");
  Serial.println("WiFi Connecting...");

  // Wait for connection
  while (wifiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("WiFi Connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  Wire.begin(21, 22);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("Nie znaleziono ekranu SSD1306"));
    for(;;);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 22);
  display.println("Uruchamianie...");
  display.display();

  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  mfrc522.PCD_Init();

  sensors.begin();

  if (!bme.begin()) {
    display.clearDisplay();
    display.setCursor(0, 10);
    display.println("Brak BME280");
    display.display();
    while (1) delay(10);
  }

  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  if(wifiMulti.run() == WL_CONNECTED){
    server.handleClient();

    unsigned long currentTime = millis();

    if (currentTime - prevTime >= refreshInterval) {
      prevTime = currentTime;
      showMeteo();
    }

    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      
      MFRC522Debug::PICC_DumpToSerial(mfrc522, Serial, &(mfrc522.uid));

      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("WYKRYTO AUTORYZACJE");
      display.drawFastHLine(0, 12, 128, WHITE);

      display.setCursor(0, 25);
      display.print("UID: ");
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        if (mfrc522.uid.uidByte[i] < 0x10) display.print("0");
        display.print(mfrc522.uid.uidByte[i], HEX);
        display.print(" ");
      }
      display.display();

      delay(3000); 
      
      prevTime = millis(); 
    }
  }else{
    Serial.print("Brak połączenia z żadną z sieci!");
  }

  delay(1000);  
}

void showMeteo() {
  sensors.requestTemperatures();
  float outdoorTemp =  sensors.getTempCByIndex(0);
  float temp = bme.readTemperature();
  float hum = bme.readHumidity();
  float pres = bme.readPressure() / 100.0F;

  display.clearDisplay();
  
  // Nagłówek interfejsu
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("STACJA METEO");
  display.drawFastHLine(0, 10, 128, WHITE);

  // Odczyt Temperatury
  display.setCursor(0, 14);
  display.print("Temp Wew.:     ");
  display.print(temp, 1);
  display.write(167); // Znak stopnia °
  display.println("C");

  display.setCursor(0, 22);
  display.print("Temp Zew.:     ");
  display.print(outdoorTemp, 1);
  display.write(167); // Znak stopnia °
  display.println("C");

  // Odczyt Wilgotności
  display.setCursor(0, 32);
  display.print("Wilgoc:   ");
  display.print(hum, 0);
  display.println(" %");

  // Odczyt Ciśnienia
  display.setCursor(0, 46);
  display.print("Cisn:     ");
  display.print(pres, 0);
  display.println(" hPa");

  display.display();
}