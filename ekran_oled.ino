#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
//definiujemy piny I2C, do których podłączony został wyświetlacz
#define OLED_SDA 21
#define OLED_SCL 22

Adafruit_SH1106 display(21, 22);

void setup()   {
  Serial.begin(115200);
  //definiujemy rodzaj użytego wyświetlacza oraz adres I2C
  display.begin(SH1106_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();
  //ustawiamy rozmiar czcionki, kolor, położenie kursora oraz wyświetlany tekst
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Hello World!");
  display.display();
}

void loop() {

}