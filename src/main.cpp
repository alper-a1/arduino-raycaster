#include <Arduino.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

const uint8_t TFT_CS = 10, TFT_DC = 8, TFT_RST = 9;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

void setup() {
    Serial.begin(9600);
    Serial.print("Hello! ST7735 TFT Test");
    // tft.initB()
    tft.initR(INITR_GREENTAB);
    // tft.fillScreen(ST7735_BLUE); 
    tft.setRotation(1); // landscape mode
}

uint8_t linxpos = 0;
void loop() {
    // put your main code here, to run repeatedly:
    // tft.fillRect(10, 10, 50, 50, ST7735_BLACK);
    // tft.fillScreen(ST7735_WHITE);

    // tft.drawFastVLine(linxpos, 50, 20, ST7735_BLACK);
    // tft.drawFastVLine(linxpos + 2, 50, 60, ST7735_BLUE);
     

    // delay(500);
    linxpos++;
    if (linxpos >= 180) {
        linxpos = 0;
    }
}

