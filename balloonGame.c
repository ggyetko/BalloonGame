#include <conio.h>
#include <stdio.h>
#include <c64/vic.h>
#include <c64/rasterirq.h>
#include <c64/memmap.h>
#include <c64/cia.h>
#include <string.h>

#define SKY_COLOR       VCOL_LT_BLUE
#define MOUNTAIN_COLOR  VCOL_DARK_GREY
#define CITY_COLOR      VCOL_LT_GREEN
#define RAMP_COLOR      VCOL_BROWN

#define Screen0 ((char *)0x0400)
#define Screen1 ((char *)0x2c00)
#define ScreenWork ((char *)0x3000)
#define ScreenColor ((char *)0xd800)

#include "graphics.h"
#include "city.h"

// Screen is        0x0400 to 0x7ff
// Weird stuff from 0x0800 to 0x09ff

// make space until 0x2000 for code and data
#pragma region( lower, 0x0a00, 0x2000, , , {code, data} )

// My font goes here
#pragma section( charset, 0)
#pragma region( charset, 0x2000, 0x2800, , , {charset} )

// My one sprite (for now) goes here
#pragma section( spriteset, 0)
#pragma region( spriteset, 0x2800, 0x29c0, , , {spriteset} )

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
__export const char spriteset[448] = {
	#embed "balloon.bin"
    #embed "balloonDecel.bin"
    #embed "balloonThrust.bin"
    #embed "balloonThrust2.bin"
    #embed "balloonFlame.bin"
    #embed "citySprite.bin"
    #embed "cityBridge.bin"
};

#pragma data(data)

// Control variables for the interrupt

// .... ...? - Are we scrolling(1) or paused(0)
// .... ..?. - is there a city in Sprite #4?
// .... .?.. - is the city ramp out?
volatile unsigned char status;
enum {
    STATUS_SCROLLING = 0x01,
    STATUS_CITY_VIS  = 0x02,
    STATUS_CITY_RAMP = 0x04
};

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
volatile unsigned int cityXPos;
volatile unsigned char cityYPos;
volatile unsigned char cityNum;

volatile char currMap;   // what world are you in
volatile char mapXCoord; // where are you in the world (right edge of screen)

/* ??...... : info - (0=nothing 1,2,3:city number)
 * ..???... : stalagtite length
 * .....??? : stalagmite length
 */

const struct Goods goods[4] =
{
    {"Rice        ", 75, 3, 0, 0},
    {"Wheat       ", 75, 3, 0, 0},
    {"Corn        ", 75, 3, 0, 0},
};
 
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
    {s"cloud city", CITY_RESPECT_NONE, {0,5}, {1,1,CITY_RESPECT_NONE,2} },
    {s"floria    ", CITY_RESPECT_NONE, {1,5}, {2,1,CITY_RESPECT_NONE,2} },
    {s"sirenia   ", CITY_RESPECT_NONE, {2,5}, {0,1,CITY_RESPECT_NONE,2} }
};

const char decelPattern[8] =  {2,3,4,5,6,7,8,16};
const char mountainHeight[8] = {0,1,2,3,4,6,8,10};

// SPRITE LIST
// #1 - Balloon
// #2 - Back thrust
// #3 - Up thrust
// #4 - City
// #5 - Ramp

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

__interrupt void prepScreen(void)
{
    if (currScreen == 0) {
        vic.memptr = 0x10 | (vic.memptr & 0x0f); // point to screen0
    } else {
        vic.memptr = 0xb0 | (vic.memptr & 0x0f); // point to screen1
    }
    setScrollAmnt (xScroll);
}

__interrupt void lowerStatBar(void)
{
    vic.color_border -= 1;
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
    vic.color_border += 1;
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
                    Screen0[0x03f8] = 0xa0; // 0xa0 * 0x64 = 0x2800 (sprite #1 data location)
                    Screen1[0x03f8] = 0xa0;
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

// Two raster interrupts (for now)
RIRQCode	spmux[4];
void setupRasterIrqs(void)
{
    rirq_stop();
    rirq_build(spmux, 1);
    rirq_call(spmux, 0, prepScreen);
    rirq_set(0, 1, spmux);

    rirq_build(spmux+1, 1);
    rirq_call(spmux+1, 0, lowerStatBar);
    rirq_set(1, 210, spmux+1);

    rirq_build(spmux+2, 1);
    rirq_call(spmux+2, 0, scrollLeft);
    rirq_set(2, 250, spmux+2);

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
    rirq_sort();
    rirq_start();
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

struct Passenger {
    char name[10];
    unsigned char fare;
};

struct Cargo {
    unsigned char psgrSpace; // reduced by damage
    Passenger psgr[8];
    unsigned char cargoSpace; // reduced by damage, max 16
    int cargo[16];
};

struct PlayerData {
    unsigned int fuel;  // fuel max out of 65535
    unsigned int money;
    unsigned char balloonHealth; // value out of 8
    Cargo cargo;
};

void balloonDamage(PlayerData *data){
    if (data->balloonHealth) {
        data->balloonHealth--;
    }
}
void carriageDamage(PlayerData *data) {
    unsigned int rnd = rand() & 0x03;
    if (rnd) {  
        // 3/4 of the time, it's cargo damage
        if (data->cargo.cargoSpace) {
            data->cargo.cargoSpace--;
        } 
    } else if (data->cargo.psgrSpace) {
        data->cargo.psgrSpace--;
    }
}

void invokeDecel(char cycles)
{
    if (holdCount) {
        return;
    }
    vic.spr_enable |= 0x02;        // turn on sprite #2
    vic_sprxy(0, 80, (yPos>>8)+24); // colocate sprite #2 with sprite #1
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
    Screen0[0x03f8] = 0xa0;
    Screen1[0x03f8] = 0xa0;    
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

enum CityMenuStates {
    CITY_MENU_MAIN = 0,
    CITY_MENU_BUY,
    CITY_MENU_SELL,
    CITY_MENU_MAYOR,
    CITY_MENU_REPAIR,  // repairs to cabins, balloon, cargo space
    CITY_MENU_UPGRADE  // upgrade cabins to 1st class, kitchen, ???
};
const unsigned char MAIN_MENU_SIZE = 6;
const char main_menu_options[6][10] = {
    s"buy       ",
    s"sell      ",
    s"mayor     ",
    s"repair    ",
    s"upgrade   ",
    s"exit      "
};

void cityMenu(void) 
{
    unsigned char menuState = CITY_MENU_MAIN;
    unsigned char homeItem = 0;
    
    for (;;) {
        unsigned char response = getMenuChoice(6, main_menu_options);
        
        if (response == 5) {
            break;
        }
    }
}

void landingOccurred(void)
{
    clearRasterIrqs();
    // turn off sidescrolling
    clearMovement();
    // turn off ALL sprites
    vic.spr_enable = 0x00;
    // go to Screen Work
    vic.memptr = 0xc0 | (vic.memptr & 0x0f);
    vic.ctrl2 = 0xc8; // 40 columns, no scroll
    vic.color_back = SKY_COLOR;
    
    drawBox(0,0,24,19,VCOL_YELLOW);
    drawBox(0,20,39,24,VCOL_YELLOW);
    drawBox(25,0,39,4,VCOL_BLACK);
    drawBox(25,5,39,19,VCOL_BLACK);

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

    clearKeyboardCache();
    cityMenu();
    
    if (currScreen == 0) {
        vic.memptr = 0x10 | (vic.memptr & 0x0f);
    } else {
        vic.memptr = 0xb0 | (vic.memptr & 0x0f);
    }
    setAveragePosition();
    vic.spr_enable = 0x01;
    vic.ctrl2 = 0xc0; // 38 columns, no scroll
    setScrollActive(0,150);
    initScreenWithDefaultColors(false);
    setupRasterIrqs();
}

void copyS0toS1(char* roof, char* ground){
    for (int y=0; y<20; y++) {
        for (int x=0;x<39;x++) {
            Screen1[y*40+x] = Screen0[y*40+x+1];
        }
        if (y<roof[1]) {
            Screen1[y*40+39] = 64;
        } else if (y == roof[1]) {
            Screen1[y*40+39] = 65;
        } else if (y == ground[1]) {
            Screen1[y*40+39] = 69;            
        } else if (y > ground[1]) {
            Screen1[y*40+39] = 64;            
        } else {
            Screen1[y*40+39] = 32;            
        }
    }
}

void copyS1toS0(char* roof, char* ground){
    for (int y=0; y<20; y++) {
        for (int x=0;x<39;x++) {
            Screen0[y*40+x] = Screen1[y*40+x+1];
        }
        if (y<roof[1]) {
            Screen0[y*40+39] = 64;
        } else if (y == roof[1]) {
            Screen0[y*40+39] = 65;
        } else if (y == ground[1]) {
            Screen0[y*40+39] = 69;            
        } else if (y > ground[1]) {
            Screen0[y*40+39] = 64;            
        } else {
            Screen0[y*40+39] = 32;            
        }
    }
}

void uint16ToString(unsigned int in, char* out)
{
    const unsigned int tens[5] = {10000, 1000, 100, 10, 1};
    unsigned int remainder;

    for (char x=0;x<5;x++) {
        char digit = in/tens[x];  // rounds down, so we're fine
        out[x] = digit + 48;
        in -= digit * tens[x];
    }
}

void showScoreBoard(struct PlayerData* data) {
    for (char x=0;x<40;x++){
        Screen0[800+x] = 91;
        Screen1[800+x] = 91;
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
    for (x=0;x<16;x++) {
        if (x < data->cargo.cargoSpace) {
            Screen0[921+5+x] = 78;
            ScreenColor[921+5+x] = VCOL_WHITE;
        } else {
            Screen0[921+5+x] = 78;
            ScreenColor[921+5+x] = VCOL_RED;
        }
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
    playerData.fuel = 65535;
    playerData.money = 5000;
    playerData.balloonHealth = 8;
    playerData.cargo.cargoSpace = 16;
    playerData.cargo.psgrSpace = 8;
    showScoreBoard(&playerData);

	// Setup sprite images
	Screen0[0x03f8] = 0xa0; // 0xa0 * 0x40 = 0x2800 (sprite #1 data location)
	Screen1[0x03f8] = 0xa0;
    // Sprite #2
	Screen0[0x03f8+1] = 0xa2; // 0xa2 * 0x40 = 0x2880 (sprite back thrust data location)
	Screen1[0x03f8+1] = 0xa2;
    // Sprite #3
	Screen0[0x03f8+2] = 0xa4; // 0xa4 * 0x40 = 0x2900 (sprite up thrust data location)
	Screen1[0x03f8+2] = 0xa4;
    // Sprite #4
	Screen0[0x03f8+3] = 0xa5; // 0xa5 * 0x40 = 0x2940 (city sprite)
	Screen1[0x03f8+3] = 0xa5;
    // Sprite #5
	Screen0[0x03f8+4] = 0xa6; // 0xa6 * 0x40 = 0x2980 (city bridge sprite)
	Screen1[0x03f8+4] = 0xa6;
    
    setScrollAmnt(xScroll);

	// Remaining sprite registers
	vic.spr_enable = 0x01;
	vic.spr_multi = 0x00;
	vic.spr_expand_x = 0x00;
	vic.spr_expand_y = 0x00;
    vic.spr_color[0] = VCOL_WHITE;
    vic.spr_color[1] = VCOL_BLACK;
    vic.spr_color[2] = VCOL_RED;
    vic.spr_color[3] = CITY_COLOR;
    vic.spr_color[4] = RAMP_COLOR;
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
            landingOccurred();
        } else if ((sprColl & 0x09) == 0x09) {
            // Collision with City - BAD
            if (terrainCollisionOccurred()) {
                balloonDamage(&playerData);
            } else {
                carriageDamage(&playerData);
            }
            showScoreBoard(&playerData);
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
                vic.spr_enable |= 0x08; // activate Sprite #4
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
