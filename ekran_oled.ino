#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h> // Zmiana biblioteki

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Inicjalizacja dla SSD1306
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  Serial.begin(115200);
  
  // ESP32 wymaga zdefiniowania pinów I2C przed display.begin
  Wire.begin(21, 22); 

  // Adres 0x3C, flaga SSD1306_SWITCHCAPVCC działa tak samo
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("Nie znaleziono ekranu SSD1306"));
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println("Hello World!");
  display.display();
}

void loop() {
}