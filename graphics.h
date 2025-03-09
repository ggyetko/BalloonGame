#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "utils.h"
#include "city.h"
#include "playerData.h"
#include "namedGoods.h"

#define BALLOON_COLOR          VCOL_WHITE
#define BALLOON_OUTLINE_COLOR  VCOL_GREEN
#define CARRIAGE_COLOR         VCOL_BROWN
#define SKY_COLOR              VCOL_LT_BLUE
#define MOUNTAIN_COLOR         VCOL_DARK_GREY
#define CITY_COLOR             VCOL_LT_GREEN
#define RAMP_COLOR             VCOL_BROWN

#define Screen0 ((char *)0x0400)
#define Screen1 ((char *)0x2c00)
#define ScreenWork ((char *)0x3000)
#define ScreenColor ((char *)0xd800)

// Graphics Routines
void clearWorkScreen(void)
{
    for (unsigned char x = 1;x<24;x++) {
        for (unsigned char y = 1;y<19;y++) {
            ScreenWork[x+y*40] = 32;
        }
    }    
}

void drawBox(unsigned char x1,unsigned char y1,unsigned char x2,unsigned char y2,unsigned char col)
{
    ScreenWork[x1+y1*40] = 103;
    ScreenColor[x1+y1*40] = col;
    ScreenWork[x2+y1*40] = 100;
    ScreenColor[x2+y1*40] = col;
    ScreenWork[x2+y2*40] = 101;
    ScreenColor[x2+y2*40] = col;
    ScreenWork[x1+y2*40] = 102;
    ScreenColor[x1+y2*40] = col;
    unsigned char x, y;
    for (x=x1+1;x<x2;x++) {
        ScreenWork[x+y1*40] = 96;
        ScreenColor[x+y1*40] = col;
        ScreenWork[x+y2*40] = 97;
        ScreenColor[x+y2*40] = col;
    }
    for (y=y1+1;y<y2;y++) {
        ScreenWork[x1+y*40] = 99;
        ScreenColor[x1+y*40] = col;
        ScreenWork[x2+y*40] = 98;
        ScreenColor[x2+y*40] = col;
    }
}

unsigned char max(unsigned char a, unsigned char b)
{
    if (a>b) return a;
    return b;
}

void putText(const char* text, unsigned char x, unsigned char y, unsigned char n, unsigned int color)
{
    unsigned int location = x + y*40;
    unsigned char c;
    for (c=0;c<n;c++,location++) {
        ScreenWork[location] = text[c];
        ScreenColor[location] = color;
    }
}

void drawBalloonDockScreen(void)
{
    unsigned char ramptop[4] = {104,106,106,105};
    unsigned char rampbottom[4] = {110,109,108,110};
    unsigned char x, y;
 
    clearWorkScreen();
    // Balloon
    for (y=1;y<8;y++) {
        for (x=1;x<12;x++) {
            unsigned char out = 79;
            ScreenColor[x+y*40] = BALLOON_COLOR;
            if ((x==2) || (x==10)) {
                if (y==6) {
                    out = 63;
                } else {
                    out = 62;
                }
            } else if (x==11) {
                out = 80-y;
            } else if (y==6) {
                out = 61;
            } else {
                out = 79;
            }
            ScreenWork[x+y*40] = out;
        }
    }
    // Wires
    ScreenWork[2+8*40] = 58;
    ScreenWork[2+9*40] = 58;
    ScreenWork[2+10*40] = 58;
    ScreenWork[2+11*40] = 58;
    ScreenWork[10+8*40] = 58;
    ScreenWork[10+9*40] = 60;
    ScreenWork[9+10*40] = 59;
    ScreenWork[9+11*40] = 60;
    // Carriage
    for (y=12;y<17;y++) {
        for (x=1;x<9;x++) {
            if ((y==13) || (y==14)) {
                ScreenWork[x+y*40] = 86 - ((x+y)&1);
            } else {
                ScreenWork[x+y*40] = 79;
            }
            ScreenColor[x+y*40] = CARRIAGE_COLOR;
        }
    }
    // Carriage Door
    ScreenWork[8+13*40] = 88;
    ScreenWork[8+14*40] = 89;
    ScreenWork[8+15*40] = 89;
    ScreenWork[8+16*40] = 89;

    // City ramp    
    for (x=0;x<20;x++) {
        ScreenWork[x+4+40*17] = ramptop[x&0x03];
        if (x>3) {
            ScreenWork[x+4+40*18] = rampbottom[x&0x03];
        }
    }
    ScreenWork[40*18+5] = 107;
    ScreenWork[40*18+6] = 108;
    ScreenWork[40*18+7] = 110;
    
    // City
    unsigned int index = 0;
    char cityArt[49] = {
        32,32,32,111,116,116,
        32,32,113,116,111,116,
        32,32,111,116,116,116,
        32,113,116,116,116,112,
        111,116,111,112,116,116,
        114,115,115,118,118,118,
        117,32,32,79,79,79
    };
    for (y=10;y<17;y++) {
        for (x=18;x<24;x++) {
            ScreenWork[x+y*40] = cityArt[index];
            ScreenColor[x+y*40] = CITY_COLOR;
            index ++;
        }
    }
}

void showWorkPassengers(Passenger *psgData) 
{
    clearWorkScreen();
    ScreenWork[1+40*1] = s'c';
    ScreenColor[1+40*1] = VCOL_DARK_GREY;
    putText(s"name",3,1,4,VCOL_DARK_GREY);
    putText(s"terminus",14,1,8,VCOL_DARK_GREY);
    ScreenWork[1+40*2] = 106;
    ScreenColor[1+40*2] = VCOL_DARK_GREY;
    for (unsigned char x = 0; x<10; x++) {
        ScreenWork[3+x+40*2] = 106;
        ScreenColor[3+x+40*2] = VCOL_DARK_GREY;
        ScreenWork[14+x+40*2] = 106;
        ScreenColor[14+x+40*2] = VCOL_DARK_GREY;
    }
    for (unsigned char x = 0; x<MAX_PASSENGERS; x++) {
        CityCode *code = &(psgData[x].destination);
        if (code->code == DESTINATION_CODE_DAMAGED_CABIN) {
            putText(s"damaged   ", 3, x+3, 10, VCOL_RED);
        } else if (code->code == DESTINATION_CODE_NO_PASSENGER) {
            ScreenWork[1+40*(x+3)] = (code->code & 0x80) ? 83 : 94; // CROWN or PERSON
            ScreenColor[1+40*(x+3)] = VCOL_BLACK;
            putText(s"empty     ", 3, x+3, 10, VCOL_BLACK);
        } else {
            unsigned char ct = CityCode_getCityNum(*code);
            unsigned char mp = CityCode_getMapNum(*code);
            ScreenWork[1+40*(x+3)] = (code->code & 0x80) ? 83 : 94; // CROWN or PERSON
            putText(psgData[x].name, 3, x+3, 10, VCOL_WHITE);
            putText(cities[mp][ct-1].name, 14, x+3, 10, VCOL_WHITE);
        }
    }
}

void showWorkCargo(PlayerData *data)
{
    clearWorkScreen();
    putText(s"cargo",3,1,5,VCOL_DARK_GREY);
    putText(s"count",14,1,5,VCOL_DARK_GREY);
    for (unsigned char x = 0; x<10; x++) {
        ScreenWork[3+x+40*2] = 106;
        ScreenColor[3+x+40*2] = VCOL_DARK_GREY;
        ScreenWork[14+x+40*2] = 106;
        ScreenColor[14+x+40*2] = VCOL_DARK_GREY;
    }
    unsigned char goodsIndexList[MAX_CARGO_SPACE];
    unsigned char goodsCountList[MAX_CARGO_SPACE];
    unsigned char listLength = makeShortCargoList(data, goodsIndexList, goodsCountList);
    for (unsigned char x = 0; x<listLength; x++) {
        putText(goods[goodsIndexList[x]].name, 3, x+3, 10, VCOL_WHITE);
        char num[3];
        ucharToString(goodsCountList[x], num);
        putText(num, 14, x+3, 3, VCOL_WHITE);
    }
}

// Takes an array of text options [10] * num
// return 0-based player's choice
// navigates with w-up, s-down, ENTER-select
const unsigned char maxDisplayedChoices = 10;
unsigned char getMenuChoice(unsigned char num, unsigned char initChoice, const char text[][10], bool doCosts, const unsigned int costs[])
{
    unsigned char currHome = 0;
    unsigned char currSelect = (initChoice < num) ? initChoice : initChoice-1;
    
    for (unsigned char x = 27; x < 39; x++) {
        for (unsigned char y = 6; y < 19; y++) {
            ScreenWork[x+y*40] = 32;
        }
    }
    
    for (;;) {
        for (unsigned char y=currHome; y<currHome+maxDisplayedChoices; y++) {
            if (y < num) {
                putText(text[y], 27, 6+y-currHome, 10, currSelect == y ? VCOL_WHITE : VCOL_DARK_GREY);
                if (doCosts && (y==currSelect)) {
                    if (costs[y]) {
                        char output[5];
                        uint16ToString(costs[y], output);
                        putText(s"cost ",27,18,5,VCOL_BLACK);
                        putText(output,32,18,5,VCOL_WHITE);
                    } else {
                        putText(s"           ",27,18,11,VCOL_BLACK);
                    }
                }
            } else {
                putText("          ", 27, 6+y-currHome, 10, VCOL_DARK_GREY);
            }
        }
        for (;;) {
            if (kbhit()){
                char ch = getch();
                ScreenWork[19] = ch/10 +48;
                ScreenWork[20] = ch%10 +48;
                if (ch == 'W') {  
                    if (currSelect) {
                        currSelect --;
                        if (currSelect < currHome) {
                            currHome = currSelect;
                        }
                        break;
                    }
                } else if (ch == 'S') {
                    if (currSelect < num-1) {
                        currSelect ++;
                        if (currSelect >= currHome + maxDisplayedChoices) {
                            currHome = currSelect - maxDisplayedChoices + 1;
                        }
                        break;
                    }
                } else if (ch == 10) {
                    return currSelect;
                }
            }
        }
    }
    
}

#endif