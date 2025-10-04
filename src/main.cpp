#include <Arduino.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// tft screen init
constexpr uint8_t TFT_CS = 10, TFT_DC = 8, TFT_RST = 9;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);


// movement buttons init:
constexpr uint8_t BTN_FWRD = 6, BTN_BWRD = 7, BTN_LFT = 4, BTN_RGHT = 5;

// fixed point types. using QM,F format, exclusive of the sign bit.
constexpr uint8_t Q7_8_SHIFT = 8;
using fixed7_8 = int16_t; // Q7.8 format
using fixed0_7 = int8_t;  // Q0.7 format

void setup() {
    Serial.begin(9600);

    tft.initR(INITR_GREENTAB);
    tft.fillScreen(ST7735_BLACK); 
    tft.setRotation(1); // landscape mode
}

constexpr uint8_t screenWidth = 160;
constexpr uint8_t screenHeight = 128;
constexpr uint8_t worldMap[10][10] = 
{
  {1,1,1,1,1,1,1,1,1,1},
  {1,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,1,1,1,0,1},
  {1,0,0,0,0,0,1,0,0,1},
  {1,0,0,0,0,1,1,0,0,1},
  {1,0,0,0,0,0,0,0,0,1},
  {1,1,1,1,1,1,1,1,1,1}
};

constexpr fixed0_7 cameraXtable[] = {0,2,3,5,6,8,10,11,13,14,16,18,19,21,22,24,26,27,29,30,32,34,35,37,38,40,42,43,45,46,48,50,51,53,54,56,58,59,61,62,64,66,67,69,70,72,74,75,77,78,80,82,83,85,86,88,90,91,93,94,96,98,99,101,102,104,106,107,109,110,112,114,115,117,118,120,122,123,125,126,127};
constexpr int16_t deltaDistTable[] = {32512,16256,10837,8128,6502,5419,4645,4064,3612,3251,2956,2709,2501,2322,2167,2032,1912,1806,1711,1626,1548,1478,1414,1355,1300,1250,1204,1161,1121,1084,1049,1016,985,956,929,903,879,856,834,813,793,774,756,739,722,707,692,677,664,650,637,625,613,602,591,581,570,561,551,542,533,524,516,508,500,493,485,478,471,464,458,452,445,439,433,428,422,417,412,406,401,396,392,387,382,378,374,369,365,361,357,353,350,346,342,339,335,332,328,325,322,319,316,313,310,307,304,301,298,296,293,290,288,285,283,280,278,276,273,271,269,266,264,262,260,258,256};
fixed7_8 posX = 6 << Q7_8_SHIFT;
fixed7_8 posY = 2 << Q7_8_SHIFT;

// movement & rotation deal with later.
// const Fixed16_16 moveStep(0.05);
// constexpr uint8_t moveDelay = 50; // ms
// unsigned long lastMoveTime = 0;

// const Fixed16_16 rosin = sin_q7_8(Fixed16_16(2));
// const Fixed16_16 rocos = cos_q7_8(Fixed16_16(2));
// const Fixed16_16 FOV_SCALE = Fixed16_16(0.66667);

int8_t dirX = -128; 
int8_t dirY = 0; 

// Q0.7 fix later
int8_t planeX = 0; 
int8_t planeY = 85; // 0.66667 in Q0.7

uint8_t currentScreenX = 0;
void loop() {
    Serial.print("\n\n\n\n\n\n\n\n");
    // Serial.print(" screenX: "); Serial.print(currentScreenX); Serial.print("\n");

    // Q0.7
    fixed0_7 cameraX = (currentScreenX < screenWidth / 2) ? -cameraXtable[(screenWidth / 2) - currentScreenX] : cameraXtable[currentScreenX - (screenWidth / 2)];
    // Serial.print(" cameraX: "); Serial.print((int)cameraX); Serial.print("\n");
    
    // Q7.8          dirX to Q0.8  :  planeX * cameraX = Q1.14 >> 6 = Q1.8 (Q1.8 can be casted to Q7.8 safely)
    fixed7_8 rayDirX = ((int16_t)dirX << 1) + (((int16_t)planeX * cameraX) >> 6);
    fixed7_8 rayDirY = ((int16_t)dirY << 1) + (((int16_t)planeY * cameraX) >> 6);
    // Serial.print(" rayDirX: "); Serial.print((int)rayDirX); Serial.print("\n");
    // Serial.print(" rayDirY: "); Serial.print((int)rayDirY); Serial.print("\n");

    // we now map rayDirX & Y into a 128 scale to use deltadistX/Y table
    // convert raydir to unsigned range [0, 65,535] and truncate to 128 bit table range.
    // map div by 0 into largest value we have -- close enough
    fixed7_8 deltaDistX = (rayDirX == 0) ? deltaDistTable[0] : deltaDistTable[abs(rayDirX * 127 + 128) >> 8];
    fixed7_8 deltaDistY = (rayDirY == 0) ? deltaDistTable[0] : deltaDistTable[abs(rayDirY * 127 + 128) >> 8];

    // Serial.print(" deltaDistX: "); Serial.print((int)deltaDistX); Serial.print("\n");
    // Serial.print(" deltaDistY: "); Serial.print((int)deltaDistY); Serial.print("\n");

    // dda step direction
    int8_t stepX = (rayDirX < 0) ? -1 : 1;
    int8_t stepY = (rayDirY < 0) ? -1 : 1;
    
    uint8_t hit = 0;
    uint8_t side;

    // convert to int but keep in Q7.8 for easier sideDist calculation
    fixed7_8 mapX = posX & 0xFF00; 
    fixed7_8 mapY = posY & 0xFF00;

    // Q7.8
    fixed7_8 sideDistX = (rayDirX < 0) ? ((posX - mapX) >> Q7_8_SHIFT) * (int32_t)deltaDistX : (((mapX + 0x100) - posX) >> Q7_8_SHIFT) * (int32_t)deltaDistX;
    fixed7_8 sideDistY = (rayDirY < 0) ? ((posY - mapY) >> Q7_8_SHIFT) * (int32_t)deltaDistY : (((mapY + 0x100) - posY) >> Q7_8_SHIFT) * (int32_t)deltaDistY;

    // bring back to normal int for dda loop math
    int8_t mapXi = mapX >> Q7_8_SHIFT;
    int8_t mapYi = mapY >> Q7_8_SHIFT;


    // dda loop
    while (hit == 0) {
        if (sideDistX < sideDistY) {
            sideDistX += deltaDistX;
            mapXi += stepX;
            side = 0;
        } else {
            sideDistY += deltaDistY;
            mapYi += stepY;
            side = 1;
        }

        if (worldMap[mapXi][mapYi] > 0) {
            hit = 1;
        }
    }

    // this is same as calculating ((mapX - posX + (1 - stepX) / 2) / rayDirX) but can be simplified due to scaling of sidedist and deltadist by raydir magnitude
    fixed7_8 perpwallDist = (side == 0) ? sideDistX - deltaDistX : sideDistY - deltaDistY;

    // drawing the actual raycaster line
    // height of the screen is 128, which is the max we can hold in a fixed16_8
    // convert screenHeight to Q7.8
    int16_t lineHeight = (8388607 / perpwallDist) >> 8;
    if (lineHeight > screenHeight) {
        lineHeight = screenHeight; // clamp to screen height
    }

    int8_t lineStart = (-lineHeight / 2) + (screenHeight / 2);
    if (lineStart < 0) {
        lineStart = 0;
    }

    uint16_t color = (side == 0) ? ST7735_GREEN : ST7735_BLUE;
    tft.drawFastVLine(currentScreenX, 0, lineStart, ST7735_BLACK);
    tft.drawFastVLine(currentScreenX, lineStart, lineHeight, color);
    tft.drawFastVLine(currentScreenX, lineStart + lineHeight, screenHeight - (lineStart + lineHeight), ST7735_BLACK);


    currentScreenX++;
    if (currentScreenX >= screenWidth) {
        currentScreenX = 0;

    }
}