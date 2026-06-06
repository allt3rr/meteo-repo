/*
  Projekt stacji meteo: RFID RC522 (SPI) + Czujnik BME280 (SPI na CS: 15) + Wyświetlacz OLED SH1106 (I2C)
*/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>

// Biblioteki dla RFID (MFRC522v2)
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>

// Biblioteki dla czujnika BME280
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// 1. Konfiguracja ekranu OLED (I2C)
#define OLED_SDA 21
#define OLED_SCL 22
Adafruit_SH1106 display(OLED_SDA, OLED_SCL);

// 2. Wspólne piny magistrali SPI dla RFID oraz BME280
#define SPI_SCK  18
#define SPI_MISO 19
#define SPI_MOSI 23

// 3. Osobne piny CS (Chip Select) dla każdego urządzenia SPI
#define RFID_CS   5  // Pin CS dla czytnika RFID
#define BME_CS   15  // Twój wybrany pin CS dla BME280

// Inicjalizacja BME280 w trybie Hardware SPI przy użyciu dedykowanego pinu CS
Adafruit_BME280 bme(BME_CS); 

// Inicjalizacja RFID
MFRC522DriverPinSimple ss_pin(RFID_CS);
MFRC522DriverSPI driver{ss_pin};
MFRC522 mfrc522{driver};

unsigned long poprzedniCzas = 0;
const long interwalOdswiezania = 2000; // Dane pogodowe odświeżają się co 2 sekundy

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Inicjalizacja ekranu OLED
  display.begin(SH1106_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.println("Uruchamianie...");
  display.display();

  // Inicjalizacja sprzętowej magistrali SPI z jawnym określeniem pinów
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  // Inicjalizacja czytnika RFID
  mfrc522.PCD_Init();

  // Inicjalizacja czujnika BME280 przez Hardware SPI
  if (!bme.begin()) {
    Serial.println("Nie znaleziono czujnika BME280 na pinie CS 15!");
    display.clearDisplay();
    display.setCursor(0, 10);
    display.println("Blad: Brak BME280");
    display.display();
    while (1) delay(10);
  }

  Serial.println("System gotowy. RFID (CS:5) oraz BME280 (CS:15) dzialaja na SPI!");
}

void loop() {
  unsigned long obecnyCzas = millis();

  // Wyświetlanie danych meteo co określony czas w tle
  if (obecnyCzas - poprzedniCzas >= interwalOdswiezania) {
    poprzedniCzas = obecnyCzas;
    wyswietlMeteo();
  }

  // Szybki, nieblokujący nasłuch czytnika RFID
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    
    // Zrzut danych na Serial Monitor
    MFRC522Debug::PICC_DumpToSerial(mfrc522, Serial, &(mfrc522.uid));

    // Przełączenie ekranu OLED na odczyt karty
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

    // Pokazuj ekran z UID przez 3 sekundy
    delay(3000); 
    
    // Wymuś natychmiastowe odświeżenie danych meteo po wyjściu z ekranu RFID
    poprzedniCzas = millis(); 
  }
}

void wyswietlMeteo() {
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
  display.setCursor(0, 18);
  display.print("Temp:     ");
  display.print(temp, 1);
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