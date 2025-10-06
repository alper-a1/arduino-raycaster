#include <Arduino.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>


#ifdef DEBUG
    #define BAUD 9600

    
    #ifdef SERIAL_DEBUG
        #define DEBUG_PRINT(x) Serial.print(x)
        #define DEBUG_PRINT_VAR(name, val) Serial.print(name); Serial.print(": "); Serial.print(val); Serial.print("\n");
    #else
        #define DEBUG_PRINT(x) 
        #define DEBUG_PRINT_VAR(name, val)
    #endif
    
    #ifdef PROFILING
        #define PROFILE_START(label) uint32_t label##Start = micros();
        #define PROFILE_END(label)   \
        uint32_t label##End = micros(); \
        Serial.print("Time (" #label "): "); Serial.print(label##End - label##Start); Serial.print("us\n");
    #else
        #define PROFILE_START(label)
        #define PROFILE_END(label)
    #endif
#else
    #define DEBUG_PRINT(x) 
    #define DEBUG_PRINT_VAR(name, val)
    #define PROFILE_START(label)
    #define PROFILE_END(label)
#endif




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
    #ifdef DEBUG
        Serial.begin(BAUD);
    #endif

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
// check helper/lineHeightOptimisation.py for how this table was generated
constexpr int16_t lhTableApprox[] = {30840,16384,10923,8192,6554,5461,4681,4096,3641,3277,2979,2731};

inline uint8_t getLineHeightApprox(const fixed7_8 perpWallDist) {
    uint8_t pwdIntPart = perpWallDist >> 8; 
    uint8_t pwdFracPart = perpWallDist & 0xFF; 

    if (pwdIntPart == 0) {
        return 128; // we are close enough to the wall that the max line height should be displayed
    } else if (pwdIntPart >= 12) {
        return 0; // clamp to minimum line height
    } 

    int16_t base_val = lhTableApprox[pwdIntPart - 1];

    if (pwdFracPart == 0) {
        return base_val >> 8; // no fractional part, return directly
    } 

    // now we interpolate to get a better approximation
    // ie for 2.3, we get the value of 2, and 30% of the value of 3 and add
    int16_t next_val = lhTableApprox[pwdIntPart];

    return (base_val + (((int32_t)(next_val - base_val) * pwdFracPart) >> 8)) >> 8;
}

constexpr int8_t cameraXtable[] = {0,2,3,5,6,8,10,11,13,14,16,18,19,21,22,24,26,27,29,30,32,34,35,37,38,40,42,43,45,46,48,50,51,53,54,56,58,59,61,62,64,66,67,69,70,72,74,75,77,78,80,82,83,85,86,88,90,91,93,94,96,98,99,101,102,104,106,107,109,110,112,114,115,117,118,120,122,123,125,126,127};
constexpr int16_t deltaDistTable[] = {32767,16384,10923,8192,6554,5461,4681,4096,3641,3277,2979,2731,2521,2341,2185,2048,1928,1820,1725,1638,1560,1489,1425,1365,1311,1260,1214,1170,1130,1092,1057,1024,993,964,936,910,886,862,840,819,799,780,762,745,728,712,697,683,669,655,643,630,618,607,596,585,575,565,555,546,537,529,520,512,504,496,489,482,475,468,462,455,449,443,437,431,426,420,415,410,405,400,395,390,386,381,377,372,368,364,360,356,352,349,345,341,338,334,331,328,324,321,318,315,312,309,306,303,301,298,295,293,290,287,285,282,280,278,275,273,271,269,266,264,262,260,258,256};
fixed7_8 posX = 2 * 256;
// fixed7_8 posY = 8 * 256;
fixed7_8 posY = 1900;

// movement & rotation deal with later.
constexpr fixed7_8 moveStep = 13; // 0.05 in Q7.8
constexpr uint8_t moveDelay = 50; // ms
unsigned long lastMoveTime = 0;

constexpr fixed0_7 rosin = 4;
constexpr fixed0_7 rocos = 127;
constexpr uint8_t FOV_SCALE = 171; // 0.66667 in 0.8

fixed0_7 dirX = 127; 
fixed0_7 dirY = 0; 

// Q0.7 fix later
fixed0_7 planeX = (-dirY * (int16_t)FOV_SCALE) >> 8;; 
fixed0_7 planeY = (dirX * (int16_t)FOV_SCALE) >> 8;; // 0.66667 in Q0.7


uint8_t currentScreenX = 0;
void loop() {
    // PROFILE_START(mathSetup)
    
    DEBUG_PRINT_VAR("screenX", (int)currentScreenX);

    // Q0.7
    fixed0_7 cameraX = (currentScreenX < screenWidth / 2) ? -cameraXtable[(screenWidth / 2) - currentScreenX] : cameraXtable[currentScreenX - (screenWidth / 2)];
    // DEBUG_PRINT_VAR("cameraX", (int)cameraX);
    
    // Q7.8          dirX to Q0.8  :  planeX * cameraX = Q1.14 >> 6 = Q1.8 (Q1.8 can be casted to Q7.8 safely)
    fixed7_8 rayDirX = ((int16_t)dirX << 1) + (((int16_t)planeX * cameraX) >> 6);
    fixed7_8 rayDirY = ((int16_t)dirY << 1) + (((int16_t)planeY * cameraX) >> 6);
    // DEBUG_PRINT_VAR("rayDirX", (int)rayDirX);
    // DEBUG_PRINT_VAR("rayDirY", (int)rayDirY);

    // we now map rayDirX & Y into a 128 scale to use deltadistX/Y table
    // map div by 0 into largest value we have -- close enough
    fixed7_8 deltaDistX = (rayDirX == 0) ? deltaDistTable[0] : deltaDistTable[abs((rayDirX << 7) + 128) >> 8];
    fixed7_8 deltaDistY = (rayDirY == 0) ? deltaDistTable[0] : deltaDistTable[abs((rayDirY << 7) + 128) >> 8];
    // DEBUG_PRINT_VAR("deltaDistX", (int)deltaDistX);
    // DEBUG_PRINT_VAR("deltaDistY", (int)deltaDistY);

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

    // PROFILE_END(mathSetup)
    // PROFILE_START(ddaLoop)

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

    // PROFILE_END(ddaLoop)

    PROFILE_START(lineMath)
    // this is same as calculating ((mapX - posX + (1 - stepX) / 2) / rayDirX) but can be simplified due to scaling of sidedist and deltadist by raydir magnitude
    fixed7_8 perpwallDist = (side == 0) ? sideDistX - deltaDistX : sideDistY - deltaDistY;

    // drawing the actual raycaster line
    // the following code approximates this: 
    // uint8_t lineHeight = 32768 / perpwallDist;
    uint8_t lineHeight = getLineHeightApprox(perpwallDist);

    int8_t lineStart = (-lineHeight >> 1) + (screenHeight >> 1);
    // if (lineStart < 0) {
    //     lineStart = 0;
    // }

    PROFILE_END(lineMath)
    // PROFILE_START(gfx)

    uint16_t color = (side == 0) ? ST7735_GREEN : ST7735_BLUE;
    tft.drawFastVLine(currentScreenX, 0, lineStart, ST7735_BLACK);
    tft.drawFastVLine(currentScreenX, lineStart, lineHeight, color);
    tft.drawFastVLine(currentScreenX, lineStart + lineHeight, screenHeight - (lineStart + lineHeight), ST7735_BLACK);
    
    // PROFILE_END(gfx)

    if (millis() - lastMoveTime > moveDelay) {
        lastMoveTime = millis();

        // if (digitalRead(BTN_FWRD) == HIGH) {
        //     if (worldMap[(posX + dirX * 10 * moveStep).toInt()][posY.toInt()] == 0) {
        //         posX += dirX * moveStep;
        //     }
        //     if (worldMap[posX.toInt()][(posY + dirY * 10 * moveStep).toInt()] == 0) {
        //         posY += dirY * moveStep;
        //     }

        // } else if (digitalRead(BTN_BWRD) == HIGH) {
        //     if (worldMap[(posX - dirX * 10 * moveStep).toInt()][posY.toInt()] == 0) {
        //         posX -= dirX * moveStep;
        //     }
        //     if (worldMap[posX.toInt()][(posY - dirY * 10 * moveStep).toInt()] == 0) {
        //         posY -= dirY * moveStep;
        //     }
        // }
        

        // TODO: ROTATION LUT to prevent fixed point drift.

        // if (digitalRead(BTN_LFT) == HIGH) {
        //     fixed0_7 oldDirX = dirX;
        //     fixed0_7 oldDirY = dirY;

            
        //     dirX = (((int16_t)oldDirX * rocos) - ((int16_t)oldDirY * rosin)) >> 7;
        //     dirY = (((int16_t)oldDirX * rosin) + ((int16_t)oldDirY * rocos)) >> 7;

        //     // FOVSCALE IN Q0.8 NOT Q0.7
        //     planeX = (-dirY * (int16_t)FOV_SCALE) >> 8;
        //     planeY = (dirX * (int16_t)FOV_SCALE) >> 8;

        // } else if (digitalRead(BTN_RGHT) == HIGH) {
        //     fixed0_7 oldDirX = dirX;
        //     fixed0_7 oldDirY = dirY;

        //     // Perform rotation using 32-bit integers to prevent overflow
        //     dirX = oldDirX * rocos + oldDirY * rosin;
        //     dirY = -oldDirX * rosin + oldDirY * rocos;

        //     planeX = -dirY * FOV_SCALE;
        //     planeY = dirX * FOV_SCALE;
        // }
    }

    currentScreenX++;
    if (currentScreenX >= screenWidth) {
        currentScreenX = 0;

    }
}