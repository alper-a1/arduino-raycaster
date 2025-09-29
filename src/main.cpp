#include <Arduino.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

#include <fixedpoint.h>

// tft screen init
constexpr uint8_t TFT_CS = 10, TFT_DC = 8, TFT_RST = 9;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// movement buttons init:
constexpr uint8_t BTN_FWRD = 6, BTN_BWRD = 7;

void setup() {
    Serial.begin(9600);
    Serial.print("Hello! ST7735 TFT Test");
    // tft.initB()
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

fixed_q7_8 posX = 4 << Q7_8_SHIFT;
fixed_q7_8 posY = 4 << Q7_8_SHIFT;  

fixed_q7_8 dirX = -1 << Q7_8_SHIFT; 
fixed_q7_8 dirY = 0 << Q7_8_SHIFT;

fixed_q7_8 planeX = 0;
fixed_q7_8 planeY = double_to_q7_8(0.66);

uint8_t currentScreenX = 0;
void loop() {
    fixed_q7_8 cameraX = q7_8_div_32b(((int32_t)2 * ((int32_t)currentScreenX << Q7_8_SHIFT)), ((screenWidth - 1)) << Q7_8_SHIFT) - Q7_8_ONE;    
    // Serial.println("x: ");
    // Serial.println(currentScreenX);
    // Serial.println("cameraX: ");
    // Serial.println(cameraX);

    fixed_q7_8 rayDirX = dirX + q7_8_mult(planeX, cameraX);
    fixed_q7_8 rayDirY = dirY + q7_8_mult(planeY, cameraX);

    int16_t mapX = posX >> Q7_8_SHIFT;
    int16_t mapY = posY >> Q7_8_SHIFT;


    // DDA setup
    
    fixed_q7_8 deltaDistX = (rayDirX == 0) ? Q7_8_MAX : abs(q7_8_div_abs(Q7_8_ONE, rayDirX));
    fixed_q7_8 deltaDistY = (rayDirY == 0) ? Q7_8_MAX : abs(q7_8_div_abs(Q7_8_ONE, rayDirY));
    
    fixed_q7_8 sideDistX;
    fixed_q7_8 sideDistY;
    
    // dda step direction
    int8_t stepX;
    int8_t stepY;

    uint8_t hit = 0;
    uint8_t side;

    // sidedist is the distance to get to an int coordinate on the map after which we will start DDA with deltadist in step direction
    if (rayDirX < 0) {
        stepX = -1;
        sideDistX = q7_8_mult((posX - (mapX << Q7_8_SHIFT)), deltaDistX);
    } else {
        stepX = 1;
        sideDistX = q7_8_mult((((mapX + 1) << Q7_8_SHIFT) - posX), deltaDistX);
    }
    if (rayDirY < 0) {
        stepY = -1;
        sideDistY = q7_8_mult((posY - (mapY << Q7_8_SHIFT)), deltaDistY);
    } else {
        stepY = 1;
        sideDistY = q7_8_mult((((mapY + 1) << Q7_8_SHIFT) - posY), deltaDistY);
    }

    uint8_t outOfBounds = 0;
    // finally start DDA loop
    while (hit == 0) {
        if (sideDistX < sideDistY) {
            sideDistX += deltaDistX;
            mapX += stepX;
            side = 0;
        } else {
            sideDistY += deltaDistY;
            mapY += stepY;
            side = 1;
        }


        if (mapX < 0 || mapX >= 10 || mapY < 0 || mapY >= 10) {
            hit = 1; // Stop the ray if it goes out of bounds
            // Make perpwallDist very large so no wall is drawn
            outOfBounds = 1;
        } else if (worldMap[mapX][mapY] > 0) {
        hit = 1;
        }
    }

    fixed_q7_8 perpwallDist;
    // this is same as calculating ((mapX - posX + (1 - stepX) / 2) / rayDirX) but can be simplified due to scaling of sidedist and deltadist by raydir magnitude
    if (side == 0) {
        perpwallDist = sideDistX - deltaDistX;
    } else {
        perpwallDist = sideDistY - deltaDistY;
    }

    if (perpwallDist == 0) perpwallDist = 1; // avoid division by zero 
    if (outOfBounds) perpwallDist = Q7_8_MAX; // if out of bounds, set very far distance

    // drawing the actual raycaster line
    // height of the screen is 128, which is the max we can hold in a fixed16_8
    int16_t lineHeight = q7_8_div(Q7_8_MAX, perpwallDist) >> Q7_8_SHIFT;

    int16_t lineStart = (-lineHeight / 2) + (screenHeight / 2);
    if (lineStart < 0) lineStart = 0; 
    // uint8_t lineEnd = (lineHeight >> 1) + (screenHeight >> 1);
    // if (lineEnd >= screenHeight) lineEnd = screenHeight - 1;
    uint16_t color = (side == 0) ? ST7735_GREEN : ST7735_BLUE;
    tft.drawFastVLine(currentScreenX, 0, lineStart, ST7735_BLACK);
    tft.drawFastVLine(currentScreenX, lineStart, lineHeight, color);
    tft.drawFastVLine(currentScreenX, lineStart + lineHeight, screenHeight - (lineStart + lineHeight), ST7735_BLACK);

    if (digitalRead(BTN_FWRD) == HIGH) {
        fixed_q7_8 moveStep = double_to_q7_8(0.05);
        if (worldMap[(posX + q7_8_mult(dirX, moveStep)) >> Q7_8_SHIFT][posY >> Q7_8_SHIFT] == 0) {
            posX += q7_8_mult(dirX, moveStep);
        }
        if (worldMap[posX >> Q7_8_SHIFT][(posY + q7_8_mult(dirY, moveStep)) >> Q7_8_SHIFT] == 0) {
            posY += q7_8_mult(dirY, moveStep);
        }
        // fixed_q7_8 oldDirX = dirX;
        // dirX = q7_8_mult(oldDirX, cos_q7_8(-Q7_8_ONE)) - q7_8_mult(dirY, sin_q7_8(-Q7_8_ONE));
        // dirY = q7_8_mult(oldDirX, sin_q7_8(-Q7_8_ONE)) + q7_8_mult(dirY, cos_q7_8(-Q7_8_ONE));
        // fixed_q7_8 oldPlaneX = planeX;
        // planeX = q7_8_mult(planeX, cos_q7_8(-Q7_8_ONE)) - q7_8_mult(planeY, sin_q7_8(-Q7_8_ONE));
        // planeY = q7_8_mult(oldPlaneX, sin_q7_8(-Q7_8_ONE)) + q7_8_mult(planeY, cos_q7_8(-Q7_8_ONE));        
    
    } else if (digitalRead(BTN_BWRD) == HIGH) {
        fixed_q7_8 moveStep = double_to_q7_8(0.05);
        if (worldMap[(posX - q7_8_mult(dirX, moveStep)) >> Q7_8_SHIFT][posY >> Q7_8_SHIFT] == 0) {
            posX -= q7_8_mult(dirX, moveStep);
        }
        if (worldMap[posX >> Q7_8_SHIFT][(posY - q7_8_mult(dirY, moveStep)) >> Q7_8_SHIFT] == 0) {
            posY -= q7_8_mult(dirY, moveStep);
        }
    }



    currentScreenX++;
    if (currentScreenX >= screenWidth) {
        currentScreenX = 0;
    }
}

