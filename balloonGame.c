#include <conio.h>
#include <stdio.h>
#include <c64/vic.h>
#include <c64/rasterirq.h>
#include <c64/memmap.h>
#include <c64/cia.h>
#include <string.h>

#define Screen0 ((char *)0x0400)
#define Screen1 ((char *)0x2c00)
#define ScreenWork ((char *)0x3000)
#define ScreenColor ((char *)0xd800)
#define BalloonSpriteLocation 0xa0

#include "utils.h"
#include "graphics.h"
#include "city.h"
#include "playerData.h"

// Screen is        0x0400 to 0x7ff
// Weird stuff from 0x0800 to 0x09ff

// make space until 0x2000 for code and data
#pragma region( lower, 0x0a00, 0x2000, , , {code, data} )

// My font goes here
#pragma section( charset, 0)
#pragma region( charset, 0x2000, 0x2800, , , {charset} )

// My sprites go here
#pragma section( spriteset, 0)
#pragma region( spriteset, 0x2800, 0x2ac0, , , {spriteset} )

#pragma section(screen2andWork, 0)
#pragma region (screen2andWork, 0x2c00, 0x33ff, , , {screen2andWork})

// everything beyond will be code, data, bss and heap to the end
#pragma region( main, 0x3400, 0xd000, , , {code, data, bss, heap, stack} )

// My font at fixed location
#pragma data(charset)
__export const char charset[2048] = {
	#embed "myFont.bin"
};

// spriteset at fixed location
#pragma data(spriteset)
__export const char spriteset[704] = {
	#embed "balloon.bin"         // 0xa0
    #embed "balloonDecel.bin"    // 0xa1
    #embed "balloonThrust.bin"   // 0xa2
    #embed "balloonThrust2.bin"  // 0xa3
    #embed "balloonFlame.bin"    // 0xa4
    #embed "citySprite.bin"      // 0xa5
    #embed "cityBridge.bin"      // 0xa6
    #embed "cart.bin"            // 0xa7
    #embed "clouds.bin"          // 0xa8, 0xa9 2 sprites here, outline and backing
    #embed "balloonOutline.bin"  // 0xaa 
};

#pragma data(data)

// Control variables for the interrupt

// .... ...? - Are we scrolling(1) or paused(0)
// .... ..?. - is there a city in Sprite #3?
// .... .?.. - is the city ramp out?
volatile unsigned char status;
enum {
    STATUS_SCROLLING = 0x01,
    STATUS_CITY_VIS  = 0x02,
    STATUS_CITY_RAMP = 0x04,
    STATUS_CARGO_IN  = 0x08,
    STATUS_CARGO_OUT = 0x10,
};

// GLOBALS
volatile unsigned char noActionDelay;  // # of screen refreshes until reactivating status   
volatile unsigned char xScroll;    // current xScroll register setting
volatile unsigned char currScreen; // 0 - Screen0, 1 - Screen1
volatile unsigned char flip;       // flag to copy screens after flip
volatile unsigned char decelIndex; // which index are we at
volatile unsigned char decelCount; // how many have we counted at this index
volatile unsigned char holdCount;
volatile unsigned char flameDelay; // keep internal burner sprite on until this hits zero

volatile unsigned int yPos; // 20*8 = 160 pixel
                            // 256 positions per pixel,
                            // 256 * 160 = 40960 locations 
                            // 0 is the top of screen, 40960 is bottom of 20th char
volatile int yVel;          // signed int, yPos/50th of a second
                            // +ve velocity is downward, -ve is upward
volatile unsigned int cityXPos;   // While in travelling mode, where is the city sprite?
volatile unsigned char cityYPos;  // While in travelling mode, where is the city sprite?
volatile unsigned char cityNum;   // While in travelling mode, what city are we near?

volatile char currMap;   // what world are you in
volatile char mapXCoord; // where are you in the world (right edge of screen)

volatile unsigned char cargoInXPos;
volatile unsigned char cargoInYPos;
volatile unsigned char cargoOutXPos;
volatile unsigned char cargoOutYPos;

#define NUM_CLOUDS 2
volatile unsigned int cloudXPos[NUM_CLOUDS];
volatile unsigned char cloudYPos[NUM_CLOUDS];

volatile unsigned char counter;

/* ??...... : info - (0=nothing 1,2,3:city number)
 * ..???... : stalagtite length (ceiling)
 * .....??? : stalagmite length (floor)
 */
 
#include "namedGoods.h"
 
const char terrain[256] =
{
    011,011,012,023,023,023,014,015,  025,025,025,024,034,043,053,052, 
    051,051,052,043,043,043,033,033,  024,034,044,043,043,032,022,012, 
    011,011,012,023,023,023,014,015,  025,025,025,024,034,043,053,052, 
    051,051,052,041,041,041,0132,032, 032,034,044,043,043,032,022,012, // City #1
    011,011,012,023,024,025,016,017,  027,027,022,022,033,044,043,052, 
    051,041,042,043,043,043,043,042,  041,041,052,061,062,062,071,072, 
    072,063,052,042,023,023,014,015,  025,025,025,024,034,043,053,052, 
    051,051,052,043,043,043,033,033,  024,034,044,043,043,032,022,012, 
    011,011,012,023,023,023,014,015,  025,025,025,024,034,043,053,052, 
    051,051,052,043,043,043,032,032,  022,032,0222,042,042,035,025,014, // City #2
    013,013,013,023,023,023,014,015,  025,025,025,024,034,043,053,052, 
    051,051,052,043,043,043,033,033,  024,034,044,043,043,032,022,012, 
    011,011,012,023,023,023,014,015,  025,025,025,024,034,043,053,052, 
    051,051,052,043,043,043,033,033,  024,034,044,043,043,032,022,012, 
    011,011,012,0323,023,023,014,015,  025,025,025,024,034,043,053,052, // City #3
    051,051,052,043,043,043,033,033,  024,034,044,043,043,032,022,012
};
struct CityData cities[3] = {
    // NAME         RESPECT            BUY     SELL
    {s"cloud city", CITY_RESPECT_LOW, 
        {{0,1},{3,1}},
        {{1,  2, CITY_RESPECT_LOW, 2},  // wheat
         {9,  2, CITY_RESPECT_LOW, 1},  // Bronze
         {11, 2, CITY_RESPECT_MED, 1}, // Iron
         {14, 2, CITY_RESPECT_HIGH,1} // Smithore
        }
    },
    {s"floria    ", CITY_RESPECT_LOW, 
        {{1,1}, {7,1}},
        {{2, 2, CITY_RESPECT_LOW, 2},  // corn
         {3, 2, CITY_RESPECT_LOW, 2},  // spinach
         {19,2, CITY_RESPECT_MED, 2},  // eggs
         {20,2, CITY_RESPECT_HIGH,2}   // quail eggs
        } 
    },
    {s"sirenia   ", CITY_RESPECT_LOW, 
        {{2,1}, {9,1}},
        {{0,2,CITY_RESPECT_LOW,2}, // rice
         {7,2,CITY_RESPECT_LOW,2}, // soy beans
         {21,2,CITY_RESPECT_MED,2}, // bok choy
         {22,2,CITY_RESPECT_HIGH,2} // black beans
        } 
    }
};

const char decelPattern[8] =  {2,3,4,5,6,7,8,16};
const char mountainHeight[8] = {0,1,2,3,4,6,8,10};

// SPRITE LIST
// Travelling            Work Screen
// #0 - Balloon Outline
// #1 - Back thrust
// #2 - Up thrust
// #3 - City
// #4 - Ramp
// #5 - Balloon          Cargo In
// #6 - Cloud Outline    Cargo Out
// #7 - Cloud Background

void setScrollActive(unsigned char active, unsigned char delay)
{
    if (active) {
        status |= STATUS_SCROLLING;
    } else {
        status &= ~STATUS_SCROLLING;
        noActionDelay = delay;
    }
}

void setScrollAmnt(char x)
{
    vic.ctrl2 = (x & 0x7) | 0xc0; // set 38 columns as well
}

__interrupt void prepWorkScreen(void)
{
    vic.memptr = 0xc0 | (vic.memptr & 0x0f);
    vic.color_back = SKY_COLOR;
    
    if (status & STATUS_CARGO_IN) {
        cargoInXPos --;
        if (cargoInXPos <= 60) {
            status &= ~STATUS_CARGO_IN;
            vic.spr_enable &= ~0x20;
        } else {
            vic_sprxy(5,cargoInXPos,cargoInYPos);
        }
    }
    if (status & STATUS_CARGO_OUT) {
        cargoOutXPos ++;
        if (cargoOutXPos >= 195) {
            status &= ~STATUS_CARGO_OUT;
            vic.spr_enable &= ~0x40;
        } else {
            vic_sprxy(6,cargoOutXPos,cargoOutYPos);
        }
    }
}

__interrupt void prepScreen(void)
{
    counter ++;
    if (currScreen == 0) {
        vic.memptr = 0x10 | (vic.memptr & 0x0f); // point to screen0
    } else {
        vic.memptr = 0xb0 | (vic.memptr & 0x0f); // point to screen1
    }
    unsigned char change = 1 - (status & STATUS_SCROLLING) + (counter%3) ? 1 : 0;
    if (cloudXPos[0] > 344) {
        cloudXPos[0] = 512;
    } else {
        cloudXPos[0] += change;
    }
    vic_sprxy(6,cloudXPos[0],cloudYPos[0]);
    vic_sprxy(7,cloudXPos[0],cloudYPos[0]);
    setScrollAmnt (xScroll);
}

__interrupt void midCloudAdjustment(void) 
{
    unsigned char change = 1 - (status & STATUS_SCROLLING) + (counter&1);
    if (cloudXPos[1] > 344) {
        cloudXPos[1] = 512;
    } else {
        cloudXPos[1] += change;
    }
    vic_sprxy(6,cloudXPos[1],cloudYPos[1]);
    vic_sprxy(7,cloudXPos[1],cloudYPos[1]);
}

__interrupt void lowerStatBarWorkScreen(void)
{
    // Score board is black background and Screen 0
    vic.memptr = 0x10 | (vic.memptr & 0x0f);
    vic.color_back = VCOL_BLACK;    
}

__interrupt void lowerStatBar(void)
{
    rand(); // entropy
    // Score board is black background and Screen 0
    vic.memptr = 0x10 | (vic.memptr & 0x0f);
    vic.color_back = VCOL_BLACK;
    // clear the horizontal scroll
    setScrollAmnt(0);
    // Apply gravity, velocity, place the balloon vertically
    if ((yVel < 255) && (status & STATUS_SCROLLING)) {
        yVel += 1;
    }
    yPos += yVel;
    vic_sprxy(0, 80, (yPos>>8) + 24);
    vic_sprxy(1, 80, (yPos>>8) + 24);
    vic_sprxy(2, 80, (yPos>>8) + 24);
    // Toggle thrust sprite
    Screen0[0x03f8+1] ^= 0x01;
	Screen1[0x03f8+1] ^= 0x01;
    if (vic.spr_color[1] = VCOL_RED) {
        vic.spr_color[1] = VCOL_ORANGE;
        vic.spr_color[2] = VCOL_ORANGE;
    } else {
        vic.spr_color[1] = VCOL_YELLOW;
        vic.spr_color[2] = VCOL_YELLOW;
    }
    // Handle upward flame continuity
    if (flameDelay) {
        flameDelay --;
        if (flameDelay == 0) {
            vic.spr_enable &= 251;
        }
    }
    // handle city movement    
    if (cityXPos) {
        vic_sprxy(3, cityXPos, cityYPos);
        if (status & STATUS_CITY_RAMP){
            if (cityXPos < 24) {
                status &= ~STATUS_CITY_RAMP;
                vic.spr_enable &= 0xef;
                cityNum = 0;
            } else {
                vic_sprxy(4, cityXPos-23, cityYPos);
            }
        }
    } 
}

__interrupt void scrollLeft(void)
{
    vic.color_back = SKY_COLOR;
    unsigned char doScroll = status & STATUS_SCROLLING;
    if (!doScroll) {
        if (noActionDelay) {
            noActionDelay--;
            if (!noActionDelay) {
                setScrollActive(1, 0);
            }
        }
        setScrollAmnt(xScroll);
    } else {
        if (holdCount) {
            if (decelIndex < 8) {
                decelCount--;
                if (decelCount) {
                    doScroll = 0;
                } else {
                    decelIndex ++;
                    if (decelIndex < 8) {
                        decelCount = decelPattern[decelIndex];
                    }
                }
            } else {
                holdCount--;
                if (holdCount == 0){
                    Screen0[0x03f8] = BalloonSpriteLocation; // 0xa0 * 0x64 = 0x2800 (sprite #0 data location)
                    Screen1[0x03f8] = BalloonSpriteLocation;
                    vic.spr_enable &= 0xfd;
                }
                doScroll = 0;
            }
        }
        if (doScroll) {
            if (xScroll == 0) {
                xScroll = 0x07;
                currScreen = 1 - currScreen;
                flip = 1;
            } else {
                xScroll -= 1;
            }
            if (status & STATUS_CITY_VIS) {
                cityXPos--;
                if (cityXPos == 0) {
                    status &= ~STATUS_CITY_RAMP;         // turn off city
                    vic.spr_enable &= 0xF7; // turn off city sprite
                }  
            }
        } 
    }
}

RIRQCode spmux[4];
void setupRasterIrqs(void)
{
    rirq_stop();
    rirq_build(spmux, 1);
    rirq_call(spmux, 0, prepScreen);
    rirq_set(0, 1, spmux);

    rirq_build(spmux+1, 1);
    rirq_call(spmux+1, 0, midCloudAdjustment);
    rirq_set(1, 90, spmux+1);

    rirq_build(spmux+2, 1);
    rirq_call(spmux+2, 0, lowerStatBar);
    rirq_set(2, 210, spmux+2);

    rirq_build(spmux+3, 1);
    rirq_call(spmux+3, 0, scrollLeft);
    rirq_set(3, 250, spmux+3);

	// Sort interrupts and start processing
	rirq_sort();
	rirq_start();
}

void setupRasterIrqsWorkScreen(void)
{
    rirq_stop();
    rirq_build(spmux, 1);
    rirq_call(spmux, 0, prepWorkScreen);
    rirq_set(0, 1, spmux);

    rirq_build(spmux+1, 1);
    rirq_call(spmux+1, 0, lowerStatBarWorkScreen);
    rirq_set(1, 210, spmux+1);

	// Sort interrupts and start processing
	rirq_sort();
	rirq_start();
}

void clearRasterIrqs(void)
{
    rirq_stop();
    rirq_clear(0);
    rirq_clear(1);
    rirq_clear(2);
    rirq_clear(3);
    rirq_sort();
    rirq_start();
}

void setupUpCargoSprites(void) {
    // Sprite #5
	ScreenWork[0x03f8+5] = 0xa7; // 0xa7 * 0x40 = 0x29c0 (cart sprite)
	ScreenWork[0x03f8+5] = 0xa7;
    vic.spr_color[5] = VCOL_BROWN;
    // Sprite #6
	ScreenWork[0x03f8+6] = 0xa7; // 0xa7 * 0x40 = 0x29c0 (cart sprite)
	ScreenWork[0x03f8+6] = 0xa7;
    vic.spr_color[6] = VCOL_BROWN;
    
    vic.spr_priority = 0x60; // put sprites #5,#6 behind background
}

void setupTravellingSprites(void) {
    // Setup sprite images
    // Sprite #0
	Screen0[0x03f8] = BalloonSpriteLocation; // 0xa0 * 0x40 = 0x2800 (sprite #1 data location)
	Screen1[0x03f8] = BalloonSpriteLocation;
    // Sprite #1
	Screen0[0x03f8+1] = 0xa2; // 0xa2 * 0x40 = 0x2880 (sprite back thrust data location)
	Screen1[0x03f8+1] = 0xa2;
    // Sprite #2
	Screen0[0x03f8+2] = 0xa4; // 0xa4 * 0x40 = 0x2900 (sprite up thrust data location)
	Screen1[0x03f8+2] = 0xa4;
    // Sprite #3
	Screen0[0x03f8+3] = 0xa5; // 0xa5 * 0x40 = 0x2940 (city sprite)
	Screen1[0x03f8+3] = 0xa5;
    // Sprite #4
	Screen0[0x03f8+4] = 0xa6; // 0xa6 * 0x40 = 0x2980 (city bridge sprite)
	Screen1[0x03f8+4] = 0xa6;
    
    // Sprite #6
	Screen0[0x03f8+6] = 0xa8; // 0xa8 * 0x40 = 0x---- (cloud outline)
	Screen1[0x03f8+6] = 0xa8;
    // Sprite #7
	Screen0[0x03f8+7] = 0xa9; // 0xa9 * 0x40 = 0x29c0 (cloud background)
	Screen1[0x03f8+7] = 0xa9;

    setScrollAmnt(xScroll);

	// Remaining sprite registers
	vic.spr_enable = 0x01;
	vic.spr_multi = 0x00;
	vic.spr_expand_x = 0x00;
	vic.spr_expand_y = 0x00;
    vic.spr_color[0] = BALLOON_COLOR;
    vic.spr_color[1] = VCOL_BLACK;
    vic.spr_color[2] = VCOL_RED;
    vic.spr_color[3] = CITY_COLOR;
    vic.spr_color[4] = RAMP_COLOR;
    vic.spr_color[6] = VCOL_BLUE;
    vic.spr_color[7] = VCOL_WHITE;

}

void clearKeyboardCache(void)
{
    for (;;) {
        if (kbhit()){
            char ch = getch();
        } else {
            break;
        }
    }
}

void showScoreBoard(struct PlayerData* data);

void invokeDecel(char cycles)
{
    if (holdCount) {
        return;
    }
    vic.spr_enable |= 0x02;        // turn on sprite #1
    vic_sprxy(0, 80, (yPos>>8)+24); // colocate sprite #1 with sprite #0
    holdCount = cycles;
    decelIndex = 0;
    decelCount = decelPattern[0];
    Screen0[0x03f8] = 0xa1; // 0xa1 * 0x64 = 0x2840 (balloon bent back data location)
	Screen1[0x03f8] = 0xa1;
}

void invokeInternalFlame(char cycles, PlayerData *data)
{
    flameDelay = cycles;
    vic.spr_enable |= 0x04;
    if (yVel < -205) {
        yVel = -255;
    } else {
        yVel -= 5 * data->balloonHealth + 10;
    }
}

const char SIZEOFSCOREBOARDMAP = 28;
const unsigned char colorMapScoreBoard[SIZEOFSCOREBOARDMAP] = {
    40, VCOL_YELLOW,
    5,  VCOL_WHITE, 17, VCOL_GREEN, 1, VCOL_YELLOW, 4, VCOL_WHITE, 13, VCOL_GREEN,
    5,  VCOL_WHITE, 17, VCOL_YELLOW, 5, VCOL_WHITE, 13, VCOL_GREEN,
    5,  VCOL_ORANGE, 17, VCOL_WHITE, 5, CITY_COLOR, 13, CITY_COLOR
};
void initScreenWithDefaultColors(bool clearScreen) {
    vic.color_border = VCOL_BLACK;
    vic.color_back = SKY_COLOR;
    unsigned int x = 0;
    while (x<20*40) {
        ScreenColor[x] = MOUNTAIN_COLOR;
        if (clearScreen) {
            Screen0[x] = 32;
            Screen1[x] = 32;
            ScreenWork[x] =32;
        }
        x ++;
    }
    for (unsigned char index = 0; index < SIZEOFSCOREBOARDMAP; index += 2) {
        for (unsigned char y = 0; y < colorMapScoreBoard[index]; y++)
        {
            ScreenColor[x] = colorMapScoreBoard[index+1];
            x++;
        }
    }
}

void setAveragePosition(void) 
{
    unsigned char borders =  mapXCoord - 33;
    unsigned char stalactite = mountainHeight[(terrain[borders] >> 3) & 0x07] * 8;
    unsigned char stalacmite = (19 - mountainHeight[(terrain[borders]) & 0x07]) * 8;
    yPos = (stalactite + stalacmite + 24) << 7;    
}

void clearMovement(void)
{
    // turn off flame sprites
    vic.spr_enable &= 0xf9;
    // turn off up flame
    flameDelay = 0;
    // turn off brake flame
    holdCount = 0;
    decelIndex = 0;
    // kill all velocity
    yVel = 0;
    // set sprite to standard shape
    Screen0[0x03f8] = BalloonSpriteLocation;
    Screen1[0x03f8] = BalloonSpriteLocation;    
}

void tenCharCopy(char *dst, char const *src) {
    for (unsigned char x = 0; x<10; x++) {
        dst[x] = src[x];
    }
}

// returns 0 for bottom collision
// returns 1 for top collision
unsigned char terrainCollisionOccurred(void)
{
    // turn off sidescrolling
    setScrollActive(0, 150);
    
    clearMovement();
    // move sprite to midpoint
    unsigned int oldPos = yPos;
    setAveragePosition();
    if (yPos > oldPos) {
        return 1;
    }
    return 0;
}

void cargoInAnimation(void) {
    if ((status & STATUS_CARGO_IN) == 0) {
        status |= STATUS_CARGO_IN;
        cargoInXPos = 194;
        cargoInYPos = 165;
        vic_sprxy(5,cargoInXPos,cargoInYPos);
        vic.spr_enable |= 0x20;
    }
}

void cargoOutAnimation(void) {
    if ((status & STATUS_CARGO_OUT) == 0) {
        status |= STATUS_CARGO_OUT;
        cargoOutXPos = 60;
        cargoOutYPos = 165;
        vic_sprxy(6,cargoInXPos,cargoInYPos);
        vic.spr_enable |= 0x40;
    }
}

const unsigned char MAIN_MENU_SIZE = 7;
const char main_menu_options[MAIN_MENU_SIZE][10] = {
    s"buy       ",
    s"sell      ",
    s"mayor     ",
    s"repair    ",
    s"refuel    ",
    s"upgrade   ",
    s"exit      "
};
#define MENU_OPTION_BUY 0
#define MENU_OPTION_SELL 1
#define MENU_OPTION_MAYOR 2
#define MENU_OPTION_REPAIR 3
#define MENU_OPTION_REFUEL 4
#define MENU_OPTION_UPGRADE 5
#define MENU_OPTION_EXIT 6

void cityMenu(PlayerData *data) 
{
    for (;;) {
        unsigned char response = getMenuChoice(MAIN_MENU_SIZE, main_menu_options, false, nullptr);
        if (response == MENU_OPTION_BUY) {
            for (;;) {
                unsigned char x;
                char buyMenuOptions[MAX_SELL_GOODS+1][10];
                unsigned int buyMenuCosts[MAX_SELL_GOODS+1];
                tenCharCopy(buyMenuOptions[0], s"return    ");
                buyMenuCosts[0] = 0;
                for (x = 1; x < MAX_SELL_GOODS+1; x++){
                    if (cities[cityNum-1].respect >= cities[cityNum-1].sellGoods[x-1].reqRespectRate) {
                        tenCharCopy(
                            buyMenuOptions[x], 
                            goods[cities[cityNum-1].sellGoods[x-1].goodsIndex].name);
                        buyMenuCosts[x] = goods[cities[cityNum-1].sellGoods[x-1].goodsIndex].normalCost;
                        buyMenuCosts[x] /= cities[cityNum-1].sellGoods[x-1].priceAdjustment;
                    } else {
                        // The city's goods list is sorted from lowest to highest respect
                        break;
                    }
                }
                unsigned char responseBuy = getMenuChoice(x, buyMenuOptions, true, buyMenuCosts);
                if (responseBuy == 0) {
                    break;
                } else {
                    if ((buyMenuCosts[responseBuy] < data->money) && 
                        (addCargoIfPossible(data, cities[cityNum-1].sellGoods[responseBuy-1].goodsIndex))) 
                    {
                        data->money -= buyMenuCosts[responseBuy];
                        cargoInAnimation();
                        showScoreBoard(data);
                    }
                }
            }
            
        } else if (response == MENU_OPTION_SELL) {
            for (;;) {
                unsigned char goodsIndexList[MAX_CARGO_SPACE];
                unsigned char listLength = makeShortCargoList(data, goodsIndexList);
                char sellMenuOptions[MAX_CARGO_SPACE+1][10];
                unsigned int sellMenuCosts[MAX_CARGO_SPACE+1];
                unsigned char x;

                tenCharCopy(sellMenuOptions[0], s"return    ");
                sellMenuCosts[0] = 0;
                for (x = 1; x < listLength+1; x++) {
                    tenCharCopy( sellMenuOptions[x], goods[goodsIndexList[x-1]].name);
                    sellMenuCosts[x] = getGoodsPurchasePrice(&cities[cityNum-1], goodsIndexList[x-1], goods[goodsIndexList[x-1]].normalCost);
                }
                unsigned char responseSell = getMenuChoice(x, sellMenuOptions, true, sellMenuCosts);
                if (responseSell == 0) {
                    break;
                } else {
                    cargoOutAnimation();
                    removeCargo(data, goodsIndexList[responseSell-1]);
                    data->money += sellMenuCosts[responseSell];
                    showScoreBoard(data);
                    if (data->cargo.currCargoCount == 0) {
                        break;
                    }
                }
            }
        } else if (response == MENU_OPTION_REFUEL) {
            for (;;) {
                unsigned char fuelList[3][10] = {s"return    ",s"quarter   ",s"full/max  "};
                unsigned int fuelListCosts[3] = {0};
                unsigned int fullCost = (65535 - data->fuel) / FUEL_COST_DIVISOR;
                bool fullMeansMax = false;
                if ((fullCost < FUEL_COST_QUARTER_TANK) || (data->money < FUEL_COST_QUARTER_TANK)) {
                    fuelListCosts[1] = 0;
                } else {
                    fuelListCosts[1] = FUEL_COST_QUARTER_TANK;
                }
                if (data->money < fullCost) {
                    fullMeansMax = true;
                    fuelListCosts[2] = data->money;
                } else {
                    fuelListCosts[2] = fullCost;
                }
                unsigned char responseRefuel = getMenuChoice(3, fuelList, true, fuelListCosts);
                if (responseRefuel == 0) {
                    break;
                } else if (responseRefuel == 1) {
                    if (fuelListCosts[1] == 0) {
                        break;
                    }
                    data->money -= FUEL_COST_QUARTER_TANK;
                    data->fuel += 0x4000;
                    showScoreBoard(data);
                } else if (responseRefuel == 2) {
                    if (fullMeansMax) {
                        data->money = 0;
                        data->fuel += fuelListCosts[2] * FUEL_COST_DIVISOR;
                    } else {
                        data->money -= fuelListCosts[2];
                        data->fuel = 65535;
                    }
                    showScoreBoard(data);
                }
            }
  
        }
        
        else if (response == MENU_OPTION_EXIT) {
            break;
        }
    }
}

void landingOccurred(PlayerData *data)
{
    clearRasterIrqs();
    // turn off sidescrolling
    clearMovement();
    // turn off ALL sprites
    vic.spr_enable = 0x00;
    setupUpCargoSprites();
    // go to Screen Work
    vic.memptr = 0xc0 | (vic.memptr & 0x0f);
    vic.ctrl2 = 0xc8; // 40 columns, no scroll
    vic.color_back = SKY_COLOR;
    
    drawBox(0,0,24,19,VCOL_YELLOW);
    drawBox(0,20,39,24,VCOL_YELLOW);
    drawBox(25,0,39,4,VCOL_BLACK);
    drawBox(25,5,39,19,VCOL_BLACK);
    drawBalloonDockScreen();

    const char respect[7] = s"respect";
    // City Name
    putText(cities[cityNum-1].name, 27, 1, 10, VCOL_WHITE);
    putText(respect, 26, 3, 7, VCOL_DARK_GREY);
    if (cities[cityNum-1].respect == CITY_RESPECT_NONE) {
        putText(s"n/a ", 34, 3, 4, VCOL_BLACK);
    } else if (cities[cityNum-1].respect == CITY_RESPECT_LOW){
        putText(s"low ", 34, 3, 4, VCOL_DARK_GREY);
    } else if (cities[cityNum-1].respect == CITY_RESPECT_MED){
        putText(s"med ", 34, 3, 4, VCOL_BROWN);
    } else {
        putText(s"high", 34, 3, 4, VCOL_YELLOW);
    }

    setupRasterIrqsWorkScreen();
    clearKeyboardCache();
    cityMenu(data);
    
    // return to travelling state
    if (currScreen == 0) {
        vic.memptr = 0x10 | (vic.memptr & 0x0f);
    } else {
        vic.memptr = 0xb0 | (vic.memptr & 0x0f);
    }
    clearRasterIrqs();
    setAveragePosition();
    vic.spr_enable = 0x01;
    vic.ctrl2 = 0xc0; // 38 columns, no scroll
    setScrollActive(0,150);
    initScreenWithDefaultColors(false);
    setupTravellingSprites();
    setupRasterIrqs();
}

void getFinalChars(char const* roof, char const* ground, unsigned char *finalRoofChar, unsigned char *finalGroundChar ){
    *finalRoofChar = 65; // peak
    if ((roof[0] <= roof[1]) && (roof[1] < roof[2])) {
        *finalRoofChar = 67;
    } else if ((roof[0] > roof[1]) && (roof[1] >= roof[2])) {
        *finalRoofChar = 66;
    }
    
    *finalGroundChar = 69; // peak
    if ((ground[0] < ground[1]) && (ground[1] <= ground[2])) {
        *finalGroundChar = 70;
    } else if ((ground[0] >= ground[1]) && (ground[1] > ground[2])) {
        *finalGroundChar = 71;
    }
}

void copyS0toS1(char const* roof, char const* ground){
    for (int y=0; y<20; y++) {
        for (int x=0;x<39;x++) {
            Screen1[y*40+x] = Screen0[y*40+x+1];
        }
        unsigned char finalRoofChar, finalGroundChar;
        getFinalChars(roof, ground, &finalRoofChar, &finalGroundChar);
        
        if (y<roof[1]) {
            Screen1[y*40+39] = 68;
        } else if (y == roof[1]) {
            Screen1[y*40+39] = finalRoofChar;
        } else if (y == ground[1]) {
            Screen1[y*40+39] = finalGroundChar;            
        } else if (y > ground[1]) {
            Screen1[y*40+39] = 68;            
        } else {
            Screen1[y*40+39] = 32;            
        }
    }
}

void copyS1toS0(char const* roof, char const* ground){
    for (int y=0; y<20; y++) {
        for (int x=0;x<39;x++) {
            Screen0[y*40+x] = Screen1[y*40+x+1];
        }
        unsigned char finalRoofChar, finalGroundChar;
        getFinalChars(roof, ground, &finalRoofChar, &finalGroundChar);

        if (y<roof[1]) {
            Screen0[y*40+39] = 68;
        } else if (y == roof[1]) {
            Screen0[y*40+39] = finalRoofChar;
        } else if (y == ground[1]) {
            Screen0[y*40+39] = finalGroundChar;            
        } else if (y > ground[1]) {
            Screen0[y*40+39] = 68;            
        } else {
            Screen0[y*40+39] = 32;            
        }
    }
}

void showScoreBoard(struct PlayerData* data) {
    for (char x=0;x<40;x++){
        Screen0[800+x] = 96;
        Screen1[800+x] = 96;
    }
    
    const char fuel[5] = s"fuel";
    const char cash[5] = s"cash";
    const char cabins[5] = s"cbns";
    const char cargo[5] = s"crgo";
    const char balloonDmg[5] = s"blhp";
    const char cityTxt[5] = s"city";
    for (char x=0;x<4;x++) {
        Screen0[841+x] = fuel[x];
        Screen0[881+x] = cash[x];
        Screen0[921+x] = cargo[x];
        Screen0[863+x] = cabins[x];
        Screen0[903+x] = balloonDmg[x];
        Screen0[943+x] = cityTxt[x];
    }
    // Fuel
    int bigBar = data->fuel >> 12;
    int smallBar = (data->fuel >> 9) & 0x07;
    char x = 0;
    for (;x<16;x++) {
        if (x<bigBar) {
            Screen0[841+5+x] = 79; // Full Block        
        } else if (x==bigBar) {
            Screen0[841+5+x] = 72 + smallBar;
        } else {
            Screen0[841+5+x] = 32;
        }
    }
    Screen0[841+5+16] = 73; 
    // Cash
    char output[5];
    uint16ToString(data->money, output);
    for (char x=0;x<5;x++) {
        Screen0[881+5+x] = output[x];
    }
    // Cargo Space
    for (x=0;x<MAX_CARGO_SPACE;x++) {
        Screen0[921+5+x] = 78;
        unsigned char output = VCOL_GREEN;
        if (data->cargo.cargo[x] == NO_GOODS) {
            output = VCOL_WHITE;
        } else if (data->cargo.cargo[x] == DAMAGED_SLOT) {
            output = VCOL_RED;
        } 
        ScreenColor[921+5+x] = output;
    }
    // Cabins and Balloon Health
    for (x=0;x<8;x++) {
        if (x < data->cargo.psgrSpace) {
            Screen0[863+5+x] = 78;
            ScreenColor[863+5+x] = VCOL_WHITE;
        } else {
            Screen0[863+5+x] = 78;
            ScreenColor[863+5+x] = VCOL_RED;
        }
        if (x < data->balloonHealth) {
            Screen0[903+5+x] = 27;
            ScreenColor[903+5+x] = VCOL_GREEN;
        } else {
            Screen0[903+5+x] = 27;
            ScreenColor[903+5+x] = VCOL_RED;
        }
    }
    // City coming up
    if (status & STATUS_CITY_RAMP) {
        for (x=0;x<10;x++) {
            Screen0[943+5+x] = cities[cityNum-1].name[x];
        }
    } else {
        for (x=0;x<10;x++) {
            Screen0[943+5+x] = 32;
        }
    }
}



int main(void)
{
    mmap_set(MMAP_NO_BASIC);
 	// Keep kernal alive 
	rirq_init(true);
    
    xScroll = 4;    // middle of scroll
    currScreen = 0; // start with Screen 0 (0x0400)
    flip = 1;       // act like flip has happened, this triggers copy to Screen 1
    yPos = 20480;   // internal Y position of balloon, middle of field
    yVel = 0;       // No starting velocity
    
    // set up map
    currMap = 0;
    mapXCoord = 0;
    initScreenWithDefaultColors(true);

    // set up scoreboard
    struct PlayerData playerData;
    playerDataInit(&playerData);
    showScoreBoard(&playerData);

    setupTravellingSprites();
    for (unsigned char cloudNum = 0; cloudNum < NUM_CLOUDS; cloudNum++) {
        cloudXPos[cloudNum] = 512;
    }

    int dummy = vic.spr_backcol;  // clear sprite-bg collisions
    dummy = vic.spr_sprcol;       // clear sprite^2 collisions

    // Two Raster IRQs, one at line 20, one at bottom of screen
    setupRasterIrqs();
    // Set up character memory
    vic.memptr = (vic.memptr & 0xf1) | 0x08; // xxxx100x means $2000 for charmap
    
    // Begin scrolling
    status |= STATUS_SCROLLING;

    for (;;) {
        if (vic.spr_backcol & 0x01) {
            // Check the status. This loop can go around twice and count a collision each time.
            if (status & STATUS_SCROLLING) {
                if (terrainCollisionOccurred()) {
                    balloonDamage(&playerData);
                } else {
                    carriageDamage(&playerData);
                }
                showScoreBoard(&playerData);
            }
        }
        unsigned char sprColl = vic.spr_sprcol;
        if ((sprColl & 0x11) == 0x11) {
            // Collision with Ramp - GOOD
            landingOccurred(&playerData);
        } else if ((sprColl & 0x09) == 0x09) {
            // Collision with City - BAD
            if (terrainCollisionOccurred()) {
                balloonDamage(&playerData);
            } else {
                carriageDamage(&playerData);
            }
            showScoreBoard(&playerData);
        }
        for (unsigned char cloudNum = 0; cloudNum < NUM_CLOUDS; cloudNum++) {
            if (cloudXPos[cloudNum] > 344) {
                cloudXPos[cloudNum] = 0;
                cloudYPos[cloudNum] = (rand()&15) + 45 + 50*cloudNum;
                vic.spr_enable |= 0xc0; 
            }
        }
        if (kbhit()){
            char ch = getch();
            if (status & STATUS_SCROLLING) {
                if (ch == 'A') {
                    invokeDecel(64);
                    playerData.fuel -= 600;
                } else if (ch == 'W') {
                    playerData.fuel -= 200;
                    invokeInternalFlame(25, &playerData);
                } else if (ch == 'X') {
                    break;
                } else if (ch == 'P') {
                    if (status & STATUS_CITY_VIS) {
                        status |= STATUS_CITY_RAMP;
                        vic.spr_enable |= 0x10;
                        showScoreBoard(&playerData);
                    }
                }
            } // throw away key presses while game is frozen
        }
        if (flip) {
            showScoreBoard(&playerData);
            char oldCoord = mapXCoord;
            mapXCoord += 1;
            char currCoord = mapXCoord;
            char nextCoord = mapXCoord + 1;
            char roof[3] = { mountainHeight[(terrain[oldCoord]>>3) & 0x7],
                             mountainHeight[(terrain[currCoord]>>3) & 0x7], 
                             mountainHeight[(terrain[nextCoord]>>3) & 0x7]};
            char ground[3] = {19 - mountainHeight[(terrain[oldCoord] & 0x7)],
                              19 - mountainHeight[(terrain[currCoord] & 0x7)], 
                              19 - mountainHeight[(terrain[nextCoord] & 0x7)]};
            if (currScreen == 0) {
                copyS0toS1(roof,ground);
            } else {
                copyS1toS0(roof,ground);
            }
            unsigned char city = terrain[currCoord]>>6;
            if (city) {
                status |= STATUS_CITY_VIS;
                cityNum = city;
                vic.spr_enable |= 0x08; // activate Sprite #3
                cityXPos = (unsigned int)(256+92);
                cityYPos = 202 - (8*mountainHeight[(terrain[currCoord] & 0x7)]) - 16;
                vic_sprxy(3, cityXPos, 202 - cityYPos);
            }
            flip = 0;
        }
    }

    clearRasterIrqs();

	return 0;
}
