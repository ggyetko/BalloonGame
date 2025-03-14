#include <conio.h>
#include <stdio.h>
#include <c64/vic.h>
#include <c64/rasterirq.h>
#include <c64/memmap.h>
#include <c64/cia.h>
#include <string.h>


#include "utils.h"
#include "graphics.h"
#include "city.h"
#include "playerData.h"
#include "quest.h"

// Screen1 is        0x0400 to 0x7ff
// Weird stuff from 0x0800 to 0x09ff, don't touch this region or it goes bad

// make space until 0x2000 for code and data
#pragma region( lower, 0x0a00, 0x2000, , , {code, data} )

// My font goes here
#pragma section( charset, 0)
#pragma region( charset, 0x2000, 0x2800, , , {charset} )

// My sprites go here
#pragma section( spriteset, 0)
#pragma region( spriteset, 0x2800, 0x2c00, , , {spriteset} )

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
__export const char spriteset[1024] = {
	#embed "balloon.bin"         // 0xa0
    #embed "balloonDecel.bin"    // 0xa1
    #embed "balloonThrust.bin"   // 0xa2
    #embed "balloonThrust2.bin"  // 0xa3
    #embed "balloonFlame.bin"    // 0xa4
    #embed "citySprites.bin"     // 0xa5(bg), 0xa6(shade), 0xa7(outline)
    #embed "clouds.bin"          // 0xa8, 0xa9 2 sprites here, outline and backing
    #embed "balloonOutline.bin"  // 0xaa 
    #embed "balloonOutDecl.bin"  // 0xab
    #embed "passengerCarts.bin"  // 0xac, 0xad
    #embed "cityBridge.bin"      // 0xae
    #embed "cart.bin"            // 0xaf
};

#define BalloonSpriteLocation      0xa0
#define BalloonDecelLocation       0xa1
#define BalloonBrakeFire1Location  0xa2
#define BalloonBrakeFire2Location  0xa3
#define BalloonFlameLocation       0xa4
#define BalloonCityBgLocation      0xa5
#define BalloonCityShadeLocation   0xa6
#define BalloonCityOutlLocation    0xa7
#define BalloonCloudOutlLocation   0xa8  // this one needs higher priority so it's in front
#define BalloonCloudLocation       0xa9
#define BalloonOutlineLocation     0xaa  // needs higher priority than BalloonSpriteLocation
#define BalloonDecelOutlLocation   0xab  // needs higher priority than BalloonDecelLocation
#define BalloonPsgrCartLocationLft 0xac
#define BalloonPsgrCartLocationRgt 0xad
#define BalloonBridgeLocation      0xae
#define BalloonCartLocation        0xaf


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
    STATUS_PSGR_IN   = 0x20,
    STATUS_PSGR_OUT  = 0x40,
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
volatile unsigned int  cityXPos;  // While in travelling mode, where is the city sprite?
volatile unsigned char cityYPos;  // While in travelling mode, where is the city sprite?
volatile unsigned char cityNum;   // While in travelling mode, what city are we near?

volatile char currMap;   // what world are you in
volatile char mapXCoord; // where are you in the world (right edge of screen)

volatile unsigned char cargoInXPos;
volatile unsigned char cargoInYPos;
volatile unsigned char cargoOutXPos;
volatile unsigned char cargoOutYPos;
volatile unsigned char psgrInXPos;
volatile unsigned char psgrInYPos;
volatile unsigned char psgrOutXPos;
volatile unsigned char psgrOutYPos;

#define NUM_CLOUDS 2
volatile unsigned int cloudXPos[NUM_CLOUDS];
volatile unsigned char cloudYPos[NUM_CLOUDS];

volatile unsigned char counter; // counter incremented once for every screen refresh (use it for all kinds of stuff)

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
    051,051,051,041,041,041,0133,033, 033,034,044,043,043,032,022,012, // City #1
    011,011,012,023,024,025,016,017,  027,027,022,022,033,044,043,052, 
    051,041,042,043,043,043,043,042,  041,041,052,061,062,062,071,072, 
    072,063,052,042,023,023,014,015,  025,025,025,024,034,043,053,052, 
    051,051,052,043,043,043,033,033,  024,034,044,043,043,032,022,012, 
    011,011,012,023,023,023,014,015,  025,025,025,024,034,043,053,052, 
    051,051,052,043,043,042,031,031, 021,031,0223,043,043,035,025,014, // City #2
    013,013,013,023,023,023,014,015,  025,025,025,024,034,043,053,052, 
    051,051,052,043,043,043,033,033,  024,034,044,043,043,032,022,012, 
    011,011,012,023,023,023,014,015,  025,025,025,024,034,043,053,052, 
    051,051,052,043,043,043,033,033,  024,034,044,043,043,032,022,012, 
    011,011,012,0323,023,023,014,015, 025,025,025,024,034,043,053,052, // City #3
    051,051,052,043,043,043,033,033,  024,034,044,043,043,032,022,012
};

const char decelPattern[8] =  {2,3,4,5,6,7,8,16};
const char mountainHeight[8] = {0,1,2,3,4,6,8,10};

#define TOP_OF_SCREEN_RASTER     50
#define HIGH_CLOUD_RASTER_LIMIT  85
#define MID_CLOUD_RASTER_LIMIT  120
#define LOW_RASTER_LIMIT        210
#define BOTTOM_RASTER_LIMIT     250
// SPRITE LIST
//         050-85        086-120               121-210
//      Travelling Hi    Travelling Medium     Travelling Low     Work Screen
// #0 - Balloon Outline  ----------------------------------->     Cargo In
// #1 - Balloon Backgrnd ----------------------------------->     Cargo Out
// #2 - Back thrust      ----------------------------------->     Psgr In
// #3 - Up Thrust        ----------------------------------->     Psgr Out
// #4 -                                        Ramp
// #5 -                                        City Outline 
// #6 - Cloud Outline    ----------------->    City Shape
// #7 - Cloud Background ----------------->    City BackShade

#define SPRITE_BALLOON_OUTLINE 0
#define SPRITE_BALLOON_BG      1
#define SPRITE_BACK_THRUST     2
#define SPRITE_UP_THRUST       3
//#define SPRITE_UNUSED        4
#define SPRITE_RAMP            5
#define SPRITE_CLOUD_OUTLINE   6
#define SPRITE_CLOUD_BG        7

#define SPRITE_CITY_OUTLINE    4
#define SPRITE_CITY_BG         6
#define SPRITE_CITY_SHADE     7


#define SPRITE_BALLOON_OUTLINE_ENABLE 0x01
#define SPRITE_BALLOON_BG_ENABLE      0x02
#define SPRITE_BACK_THRUST_ENABLE     0x04
#define SPRITE_UP_THRUST_ENABLE       0x08
#define SPRITE_RAMP_ENABLE            0x20
#define SPRITE_CLOUD_OUTLINE_ENABLE   0x40
#define SPRITE_CLOUD_BG_ENABLE        0x80

#define SPRITE_CITY_OUTLINE_ENABLE    0x10
#define SPRITE_CITY_BG_ENABLE         0x40
#define SPRITE_CITY_SHADE_ENABLE      0x80


#define SPRITE_CARGO_IN        0
#define SPRITE_CARGO_OUT       1
#define SPRITE_PSGR_IN         2
#define SPRITE_PSGR_OUT        3

#define SPRITE_CARGO_IN_ENABLE        0x01
#define SPRITE_CARGO_OUT_ENABLE       0x02
#define SPRITE_PSGR_IN_ENABLE         0x04
#define SPRITE_PSGR_OUT_ENABLE        0x08

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
    // work screen is 0xc0
    vic.memptr = 0xc0 | (vic.memptr & 0x0f);
    vic.color_back = SKY_COLOR;
    
    if (status & STATUS_CARGO_IN) {
        cargoInXPos --;
        if (cargoInXPos <= 60) {
            status &= ~STATUS_CARGO_IN;
            vic.spr_enable &= ~SPRITE_CARGO_IN_ENABLE;
        } else {
            vic_sprxy(SPRITE_CARGO_IN,cargoInXPos,cargoInYPos);
        }
    }
    if (status & STATUS_CARGO_OUT) {
        cargoOutXPos ++;
        if (cargoOutXPos >= 195) {
            status &= ~STATUS_CARGO_OUT;
            vic.spr_enable &= ~SPRITE_CARGO_OUT_ENABLE;
        } else {
            vic_sprxy(SPRITE_CARGO_OUT,cargoOutXPos,cargoOutYPos);
        }
    }
    
    if (status & STATUS_PSGR_IN) {
        psgrInXPos --;
        if (psgrInXPos <= 60) {
            status &= ~STATUS_PSGR_IN;
            vic.spr_enable &= ~SPRITE_PSGR_IN_ENABLE;
        } else {
            vic_sprxy(SPRITE_PSGR_IN,psgrInXPos,psgrInYPos);
        }
    }
    if (status & STATUS_PSGR_OUT) {
        psgrOutXPos ++;
        if (psgrOutXPos >= 195) {
            status &= ~STATUS_PSGR_OUT;
            vic.spr_enable &= ~SPRITE_PSGR_OUT_ENABLE;
        } else {
            vic_sprxy(SPRITE_PSGR_OUT,psgrOutXPos,psgrOutYPos);
        }
    }
}

__interrupt void prepScreen(void)
{
    counter ++;
    // Getting this interrupt means we're in travelling mode, set the screen
    if (currScreen == 0) {
        vic.memptr = 0x10 | (vic.memptr & 0x0f); // point to screen0
    } else {
        vic.memptr = 0xb0 | (vic.memptr & 0x0f); // point to screen1
    }
    // set up the cloud sprite pointers
    Screen0[0x03f8+SPRITE_CLOUD_OUTLINE   ] = BalloonCloudOutlLocation;
    Screen1[0x03f8+SPRITE_CLOUD_OUTLINE   ] = BalloonCloudOutlLocation;
    Screen0[0x03f8+SPRITE_CLOUD_BG        ] = BalloonCloudLocation;
    Screen1[0x03f8+SPRITE_CLOUD_BG        ] = BalloonCloudLocation;
    vic.spr_color[SPRITE_CLOUD_OUTLINE] = CLOUD_OUTLINE_COLOR;
    vic.spr_color[SPRITE_CLOUD_BG] = CLOUD_COLOR;
    vic.spr_enable |= SPRITE_CLOUD_OUTLINE_ENABLE | SPRITE_CLOUD_BG_ENABLE;

    // move the high level cloud    
    unsigned char change = 1 - (status & STATUS_SCROLLING) + (counter%3) ? 1 : 0;
    if (cloudXPos[0] > 344) {
        cloudXPos[0] = 512;
    } else {
        cloudXPos[0] += change;
    }
    vic_sprxy(SPRITE_CLOUD_OUTLINE,cloudXPos[0],cloudYPos[0]);
    vic_sprxy(SPRITE_CLOUD_BG,cloudXPos[0],cloudYPos[0]);
    setScrollAmnt (xScroll);
}

// Set up the cloud sprite for the middle level
__interrupt void midCloudAdjustment(void) 
{
    // move the mid level cloud
    unsigned char change = 1 - (status & STATUS_SCROLLING) + (counter&1);
    if (cloudXPos[1] > 344) {
        cloudXPos[1] = 512;
    } else {
        cloudXPos[1] += change;
    }
    vic_sprxy(SPRITE_CLOUD_OUTLINE,cloudXPos[1],cloudYPos[1]);
    vic_sprxy(SPRITE_CLOUD_BG,cloudXPos[1],cloudYPos[1]);
    vic.spr_enable |= SPRITE_CLOUD_OUTLINE_ENABLE | SPRITE_CLOUD_BG_ENABLE;
}

// The low level is where the two cloud sprites might be used as city sprites
__interrupt void lowCloudAdjustment(void) 
{
    if (status & STATUS_CITY_VIS) {
        Screen0[0x03f8+SPRITE_CITY_OUTLINE ] = BalloonCityOutlLocation; 
        Screen1[0x03f8+SPRITE_CITY_OUTLINE ] = BalloonCityOutlLocation;
        vic.spr_color[SPRITE_CITY_OUTLINE] = CITY_OUTLINE_COLOR;
        
        Screen0[0x03f8+SPRITE_CITY_BG ] = BalloonCityBgLocation;
        Screen1[0x03f8+SPRITE_CITY_BG ] = BalloonCityBgLocation;
        vic.spr_color[SPRITE_CITY_BG] = CITY_COLOR;
        
        Screen0[0x03f8+SPRITE_CITY_SHADE] = BalloonCityShadeLocation;
        Screen1[0x03f8+SPRITE_CITY_SHADE] = BalloonCityShadeLocation;
        vic.spr_color[SPRITE_CITY_SHADE] = CITY_SHADE_COLOR;

        vic.spr_enable |= SPRITE_CITY_BG_ENABLE | SPRITE_CITY_OUTLINE_ENABLE | SPRITE_CITY_SHADE_ENABLE;

        vic_sprxy(SPRITE_CITY_OUTLINE, cityXPos, cityYPos);
        vic_sprxy(SPRITE_CITY_BG, cityXPos, cityYPos);
        vic_sprxy(SPRITE_CITY_SHADE, cityXPos, cityYPos);
        
        if (status & STATUS_CITY_RAMP) {
            vic_sprxy(SPRITE_RAMP, cityXPos-23, cityYPos+8);
        }
        
    } else {
        vic.spr_enable &= ~(SPRITE_CITY_BG_ENABLE | SPRITE_CITY_OUTLINE_ENABLE | SPRITE_CITY_SHADE_ENABLE);
    }

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
    vic_sprxy(SPRITE_BALLOON_OUTLINE , 80, (yPos>>8) + 24);
    vic_sprxy(SPRITE_BALLOON_BG      , 80, (yPos>>8) + 24);
    vic_sprxy(SPRITE_BACK_THRUST     , 80, (yPos>>8) + 24);
    if (holdCount) {
        vic_sprxy(SPRITE_UP_THRUST       , 80-4, (yPos>>8) + 22);
    } else {
        vic_sprxy(SPRITE_UP_THRUST       , 80, (yPos>>8) + 24);
    }

    if (counter & 0x02) {
        Screen0[0x03f8+SPRITE_BACK_THRUST] = BalloonBrakeFire1Location;
        Screen1[0x03f8+SPRITE_BACK_THRUST] = BalloonBrakeFire1Location;
    } else {
        Screen0[0x03f8+SPRITE_BACK_THRUST] = BalloonBrakeFire2Location;
        Screen1[0x03f8+SPRITE_BACK_THRUST] = BalloonBrakeFire2Location;
    }
    if (counter & 0x01) {
        vic.spr_color[SPRITE_BACK_THRUST] = VCOL_ORANGE;
        vic.spr_color[SPRITE_UP_THRUST] = VCOL_YELLOW;
    } else {
        vic.spr_color[SPRITE_BACK_THRUST] = VCOL_YELLOW;
        vic.spr_color[SPRITE_UP_THRUST] = VCOL_ORANGE;
    }
    // Handle upward flame continuity
    if (flameDelay) {
        flameDelay --;
        if (flameDelay == 0) {
            vic.spr_enable &= ~SPRITE_UP_THRUST_ENABLE;
        }
    }
    // handle city movement    
    if (cityXPos) {
        if (status & STATUS_CITY_RAMP){
            if (cityXPos < 24) {
                status &= ~STATUS_CITY_RAMP;
                vic.spr_enable &= ~SPRITE_RAMP_ENABLE;
                cityNum = 0;
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
            // step through the gentle deceleration array
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
                // deceleration is done, just hold for the count
                holdCount--;
                if (holdCount == 0){
                    // return to normal balloon shape
                    Screen0[0x03f8+SPRITE_BALLOON_OUTLINE] = BalloonOutlineLocation;
                    Screen1[0x03f8+SPRITE_BALLOON_OUTLINE] = BalloonOutlineLocation;
                    Screen0[0x03f8+SPRITE_BALLOON_BG] = BalloonSpriteLocation;
                    Screen1[0x03f8+SPRITE_BALLOON_BG] = BalloonSpriteLocation;
                    // turn off back thrust sprite
                    vic.spr_enable &= ~SPRITE_BACK_THRUST_ENABLE;
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
                    status &= ~(STATUS_CITY_VIS | STATUS_CITY_RAMP); // turn off city and ramp
                }  
            }
        } 
    }
}

__interrupt
void introScreenInterrupt (void)
{
    vic.color_back = VCOL_BLACK;
    counter ++;
    if (counter & 0x01) {
        vic.spr_color[SPRITE_BACK_THRUST] = VCOL_ORANGE;
    } else {
        vic.spr_color[SPRITE_BACK_THRUST] = VCOL_RED;
    }
    if (counter & 0x02) {
        ScreenWork[0x03f8+SPRITE_BACK_THRUST] = BalloonBrakeFire1Location;
    } else {
        ScreenWork[0x03f8+SPRITE_BACK_THRUST] = BalloonBrakeFire2Location;
    }
    
    for (unsigned char cloud = 0; cloud < 2; cloud ++) {
        cloudXPos[cloud] ++;
        if (cloudXPos[cloud] > 344) {
            cloudXPos[cloud] = 0;
            cloudYPos[cloud] = rand() % 100 + 50 + 100 * cloud;
        }
    }
    vic_sprxy(SPRITE_CLOUD_BG, cloudXPos[0], cloudYPos[0]);
    vic_sprxy(SPRITE_CLOUD_OUTLINE, cloudXPos[0], cloudYPos[0]);
    vic_sprxy(SPRITE_RAMP, cloudXPos[1], cloudYPos[1]);
    vic_sprxy(SPRITE_UP_THRUST, cloudXPos[1], cloudYPos[1]);
}

RIRQCode spmux[5];
void setupRasterIrqs(void)
{
    rirq_stop();
    rirq_build(spmux, 1);
    rirq_call(spmux, 0, prepScreen);
    rirq_set(0, 1, spmux);

    rirq_build(spmux+1, 1);
    rirq_call(spmux+1, 0, midCloudAdjustment);
    rirq_set(1, HIGH_CLOUD_RASTER_LIMIT, spmux+1);

    rirq_build(spmux+2, 1);
    rirq_call(spmux+2, 0, lowCloudAdjustment);
    rirq_set(2, MID_CLOUD_RASTER_LIMIT, spmux+2);

    rirq_build(spmux+3, 1);
    rirq_call(spmux+3, 0, lowerStatBar);
    rirq_set(3, LOW_RASTER_LIMIT, spmux+3);

    rirq_build(spmux+4, 1);
    rirq_call(spmux+4, 0, scrollLeft);
    rirq_set(4, BOTTOM_RASTER_LIMIT, spmux+4);

	// Sort interrupts and start processing
	rirq_sort();
	rirq_start();
}

void setupRasterIrqsIntro1(void)
{
    rirq_stop();
    rirq_build(spmux, 1);
    rirq_call(spmux, 0, introScreenInterrupt);
    rirq_set(0, 250, spmux);

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
    rirq_clear(4);
    rirq_sort();
    rirq_start();
}

void setupUpIntroSprites(void) {
    vic.spr_expand_x = 0xff;
    vic.spr_expand_y = 0xff;
    vic.spr_priority = 0xff;  // all sprite behind text
    ScreenWork[0x03f8+SPRITE_BALLOON_OUTLINE] = BalloonOutlineLocation;
	ScreenWork[0x03f8+SPRITE_BALLOON_BG     ] = BalloonSpriteLocation;
	ScreenWork[0x03f8+SPRITE_BACK_THRUST     ] = BalloonBrakeFire1Location; 

   
	ScreenWork[0x03f8+SPRITE_CLOUD_OUTLINE   ] = BalloonCloudOutlLocation; 
	ScreenWork[0x03f8+SPRITE_CLOUD_BG        ] = BalloonCloudLocation; 
	ScreenWork[0x03f8+SPRITE_UP_THRUST       ] = BalloonCloudOutlLocation; 
	ScreenWork[0x03f8+SPRITE_RAMP            ] = BalloonCloudLocation; 

    vic.spr_color[SPRITE_BALLOON_OUTLINE] = BALLOON_OUTLINE_COLOR;
    vic.spr_color[SPRITE_BALLOON_BG] = BALLOON_COLOR;
    vic.spr_color[SPRITE_BACK_THRUST] = VCOL_RED;
    vic.spr_color[SPRITE_CLOUD_OUTLINE] = CLOUD_OUTLINE_COLOR;
    vic.spr_color[SPRITE_CLOUD_BG] = VCOL_LT_GREY;
    vic.spr_color[SPRITE_UP_THRUST] = CLOUD_OUTLINE_COLOR;
    vic.spr_color[SPRITE_RAMP] = VCOL_LT_GREY;
}

void setupUpCargoSprites(void) {
    vic.spr_expand_x = 0x00;
    vic.spr_expand_y = 0x00;
	ScreenWork[0x03f8+SPRITE_CARGO_IN] = BalloonCartLocation;
    vic.spr_color[SPRITE_CARGO_IN] = VCOL_BROWN;

	ScreenWork[0x03f8+SPRITE_CARGO_OUT] = BalloonCartLocation;
    vic.spr_color[SPRITE_CARGO_OUT] = VCOL_BROWN;
    
	ScreenWork[0x03f8+SPRITE_PSGR_IN] = BalloonPsgrCartLocationLft;
    vic.spr_color[SPRITE_PSGR_IN] = VCOL_BROWN;

	ScreenWork[0x03f8+SPRITE_PSGR_OUT] = BalloonPsgrCartLocationRgt;
    vic.spr_color[SPRITE_PSGR_OUT] = VCOL_BROWN;
    
    vic.spr_priority = SPRITE_CARGO_IN_ENABLE | SPRITE_CARGO_OUT_ENABLE | SPRITE_PSGR_IN_ENABLE | SPRITE_PSGR_OUT_ENABLE; // put sprites behind background
}

void setupTravellingSprites(void) {
    vic.spr_expand_x = 0x00;
    vic.spr_expand_y = 0x00;
    vic.spr_priority = 0x00;  // all sprite in front of text
    // Setup sprite images
	Screen0[0x03f8+SPRITE_BALLOON_OUTLINE] = BalloonOutlineLocation;
	Screen1[0x03f8+SPRITE_BALLOON_OUTLINE] = BalloonOutlineLocation;

	Screen0[0x03f8+SPRITE_BALLOON_BG     ] = BalloonSpriteLocation;
	Screen1[0x03f8+SPRITE_BALLOON_BG     ] = BalloonSpriteLocation;

	Screen0[0x03f8+SPRITE_BACK_THRUST     ] = BalloonBrakeFire1Location; // 0xa2 * 0x40 = 0x2880 (sprite back thrust data location)
	Screen1[0x03f8+SPRITE_BACK_THRUST     ] = BalloonBrakeFire1Location;

	Screen0[0x03f8+SPRITE_UP_THRUST       ] = BalloonFlameLocation; // 0xa4 * 0x40 = 0x2900 (sprite up thrust data location)
	Screen1[0x03f8+SPRITE_UP_THRUST       ] = BalloonFlameLocation;

	Screen0[0x03f8+SPRITE_CITY_OUTLINE    ] = BalloonCityOutlLocation; // 0xa5 * 0x40 = 0x2940 (city sprite)
	Screen1[0x03f8+SPRITE_CITY_OUTLINE    ] = BalloonCityOutlLocation;

	Screen0[0x03f8+SPRITE_RAMP            ] = BalloonBridgeLocation; // 0xa6 * 0x40 = 0x2980 (city bridge sprite)
	Screen1[0x03f8+SPRITE_RAMP            ] = BalloonBridgeLocation;

	Screen0[0x03f8+SPRITE_CLOUD_OUTLINE   ] = BalloonCloudOutlLocation; // 0xa8 * 0x40 = 0x---- (cloud outline)
	Screen1[0x03f8+SPRITE_CLOUD_OUTLINE   ] = BalloonCloudOutlLocation;

	Screen0[0x03f8+SPRITE_CLOUD_BG        ] = BalloonCloudLocation; // 0xa9 * 0x40 = 0x29c0 (cloud background)
	Screen1[0x03f8+SPRITE_CLOUD_BG        ] = BalloonCloudLocation;

    setScrollAmnt(xScroll);

	// Remaining sprite registers
	vic.spr_enable = SPRITE_BALLOON_OUTLINE_ENABLE | SPRITE_BALLOON_BG_ENABLE;
	vic.spr_multi = 0x00;
	vic.spr_expand_x = 0x00;
	vic.spr_expand_y = 0x00;
    vic.spr_color[SPRITE_BALLOON_OUTLINE] = BALLOON_OUTLINE_COLOR;
    vic.spr_color[SPRITE_BALLOON_BG] = BALLOON_COLOR;
    vic.spr_color[SPRITE_BACK_THRUST] = VCOL_BLACK;
    vic.spr_color[SPRITE_UP_THRUST] = VCOL_RED;
    vic.spr_color[SPRITE_CITY_OUTLINE] = CITY_OUTLINE_COLOR;
    vic.spr_color[SPRITE_RAMP] = RAMP_COLOR;
    vic.spr_color[SPRITE_CLOUD_OUTLINE] = CLOUD_OUTLINE_COLOR;
    vic.spr_color[SPRITE_CLOUD_BG] = CLOUD_COLOR;

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
    vic.spr_enable |= SPRITE_BACK_THRUST_ENABLE;        // turn on sprite #1
    vic_sprxy(SPRITE_BACK_THRUST, 80, (yPos>>8)+24); // colocate sprite #1 with sprite #0
    holdCount = cycles;
    decelIndex = 0;
    decelCount = decelPattern[0];
    // set balloon to angled "decelerating" appearance
    Screen0[0x03f8+SPRITE_BALLOON_OUTLINE] = BalloonDecelOutlLocation;
	Screen1[0x03f8+SPRITE_BALLOON_OUTLINE] = BalloonDecelOutlLocation;
    Screen0[0x03f8+SPRITE_BALLOON_BG] = BalloonDecelLocation;
	Screen1[0x03f8+SPRITE_BALLOON_BG] = BalloonDecelLocation;
}

void invokeInternalFlame(char cycles, PlayerData *data)
{
    flameDelay = cycles;
    vic.spr_enable |= SPRITE_UP_THRUST_ENABLE;
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
    vic.spr_enable &= ~(SPRITE_BACK_THRUST_ENABLE | SPRITE_UP_THRUST_ENABLE);
    // turn off up flame
    flameDelay = 0;
    // turn off brake flame
    holdCount = 0;
    decelIndex = 0;
    // kill all velocity
    yVel = 0;
    // set sprite to standard shape
    Screen0[0x03f8+SPRITE_BALLOON_OUTLINE] = BalloonOutlineLocation;
    Screen1[0x03f8+SPRITE_BALLOON_OUTLINE] = BalloonOutlineLocation;    
    Screen0[0x03f8+SPRITE_BALLOON_BG] = BalloonSpriteLocation;
    Screen1[0x03f8+SPRITE_BALLOON_BG] = BalloonSpriteLocation;    
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
        vic_sprxy(SPRITE_CARGO_IN,cargoInXPos,cargoInYPos);
        vic.spr_enable |= SPRITE_CARGO_IN_ENABLE;
    }
}

void cargoOutAnimation(void) {
    if ((status & STATUS_CARGO_OUT) == 0) {
        status |= STATUS_CARGO_OUT;
        cargoOutXPos = 60;
        cargoOutYPos = 165;
        vic_sprxy(SPRITE_CARGO_OUT,cargoOutXPos,cargoOutYPos);
        vic.spr_enable |= SPRITE_CARGO_OUT_ENABLE;
    }
}
void passengerInAnimation(void) {
    if ((status & STATUS_PSGR_IN) == 0) {
        status |= STATUS_PSGR_IN;
        psgrInXPos = 194;
        psgrInYPos = 165;
        vic_sprxy(SPRITE_PSGR_IN,psgrInXPos,psgrInYPos);
        vic.spr_enable |= SPRITE_PSGR_IN_ENABLE;
    }
}
void passengerOutAnimation(void) {
    if ((status & STATUS_PSGR_OUT) == 0) {
        status |= STATUS_PSGR_OUT;
        psgrOutXPos = 60;
        psgrOutYPos = 165;
        vic_sprxy(SPRITE_PSGR_OUT,psgrOutXPos,psgrOutYPos);
        vic.spr_enable |= SPRITE_PSGR_OUT_ENABLE;
    }
}

const unsigned char MAIN_MENU_SIZE = 9;
const char main_menu_options[MAIN_MENU_SIZE][10] = {
    s"buy       ",
    s"sell      ",
    s"passengers",
    s"mayor     ",
    s"repair    ",
    s"refuel    ",
    s"upgrade   ",
    s"inventory ",
    s"exit      "
};
#define MENU_OPTION_BUY 0
#define MENU_OPTION_SELL 1
#define MENU_OPTION_PASSENGER 2
#define MENU_OPTION_MAYOR 3
#define MENU_OPTION_REPAIR 4
#define MENU_OPTION_REFUEL 5
#define MENU_OPTION_UPGRADE 6
#define MENU_OPTION_INVENTORY 7
#define MENU_OPTION_EXIT 8

void cityMenuBuy(PlayerData *data) 
{
    unsigned char lastChoice = 0;
    for (;;) {
        unsigned char x;
        char buyMenuOptions[MAX_SELL_GOODS+1][10];
        unsigned int buyMenuCosts[MAX_SELL_GOODS+1];
        tenCharCopy(buyMenuOptions[0], s"return    ");
        buyMenuCosts[0] = 0;
        for (x = 1; x < MAX_SELL_GOODS+1; x++){
            if (cities[currMap][cityNum-1].respect >= cities[currMap][cityNum-1].sellGoods[x-1].reqRespectRate) {
                tenCharCopy(
                    buyMenuOptions[x], 
                    goods[cities[currMap][cityNum-1].sellGoods[x-1].goodsIndex].name);
                buyMenuCosts[x] = goods[cities[currMap][cityNum-1].sellGoods[x-1].goodsIndex].normalCost;
                buyMenuCosts[x] /= cities[currMap][cityNum-1].sellGoods[x-1].priceAdjustment;
            } else {
                // The city's goods list is sorted from lowest to highest respect
                break;
            }
        }
        unsigned char responseBuy = getMenuChoice(x, lastChoice, buyMenuOptions, true, buyMenuCosts);
        if (responseBuy == 0) {
            break;
        } else {
            if ((buyMenuCosts[responseBuy] < data->money) && 
                (addCargoIfPossible(data, cities[currMap][cityNum-1].sellGoods[responseBuy-1].goodsIndex))) 
            {
                data->money -= buyMenuCosts[responseBuy];
                cargoInAnimation();
                showScoreBoard(data);
            }
            lastChoice = responseBuy;
        }
    }       
}

void cityMenuSell(PlayerData *data) 
{
    unsigned char lastChoice = 0;
    for (;;) {
        unsigned char goodsIndexList[MAX_CARGO_SPACE];
        unsigned char goodsCountList[MAX_CARGO_SPACE];
        unsigned char listLength = makeShortCargoList(data, goodsIndexList, goodsCountList);
        char sellMenuOptions[MAX_CARGO_SPACE+1][10];
        unsigned int sellMenuCosts[MAX_CARGO_SPACE+1];
        unsigned char x;

        tenCharCopy(sellMenuOptions[0], s"return    ");
        sellMenuCosts[0] = 0;
        for (x = 1; x < listLength+1; x++) {
            tenCharCopy( sellMenuOptions[x], goods[goodsIndexList[x-1]].name);
            sellMenuCosts[x] = getGoodsPurchasePrice(&cities[currMap][cityNum-1], goodsIndexList[x-1], goods[goodsIndexList[x-1]].normalCost);
        }
        unsigned char responseSell = getMenuChoice(x, lastChoice, sellMenuOptions, true, sellMenuCosts);
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
            lastChoice = responseSell;                    
        }
    }
}

void cityMenuRepair(PlayerData *data)
{
    unsigned char lastChoice = 0;
    for (;;) {
        unsigned char repairList[4][10] = 
            {s"return    ", s"blln fabrc", s"psgr cabin", s"cargo bay "};
        unsigned int repairListCosts[4] = 
            {0, REPAIR_COST_BALLOON_FABRIC, REPAIR_COST_PSGR_CABIN, REPAIR_COST_CARGO};

        if (cities[currMap][cityNum-1].facility & CITY_FACILITY_BALLOON_FABRIC) {
            repairListCosts[1] -= REPAIR_COST_FACILITY_REDUCTION; 
        }
        if (cities[currMap][cityNum-1].facility & CITY_FACILITY_PSGR_CABIN) {
            repairListCosts[2] -= REPAIR_COST_FACILITY_REDUCTION;
        }
        if (cities[currMap][cityNum-1].facility & CITY_FACILITY_CARGO) {
            repairListCosts[3] -= REPAIR_COST_FACILITY_REDUCTION;
        }
            
        unsigned char responseRepair = getMenuChoice(4, 0, repairList, true, repairListCosts);
        if (responseRepair == 0) {
            break;
        } else if (responseRepair == 1) {
            if ((data->balloonHealth < MAX_BALLOON_HEALTH) && (data->money > repairListCosts[1])) {
                data->balloonHealth++;
                data->money -= repairListCosts[1];
                showScoreBoard(data);
            }
        } else if (responseRepair == 2) {
            if (data->money > repairListCosts[2]) {
                for (unsigned char x=0; x<MAX_PASSENGERS; x++) {
                    if (data->cargo.psgr[x].destination.code == DESTINATION_CODE_DAMAGED_CABIN) {
                        data->cargo.psgr[x].destination.code = DESTINATION_CODE_NO_PASSENGER;
                        data->money -= repairListCosts[2];
                        showScoreBoard(data);
                        break;
                    }
                }
            }
        } else if (responseRepair == 3) {
            if ((data->cargo.cargoSpace < MAX_CARGO_SPACE) && (data->money > repairListCosts[3])) {
                for (unsigned char x=0; x<MAX_CARGO_SPACE; x++) {
                    if (data->cargo.cargo[x] == DAMAGED_SLOT) {
                        data->cargo.cargo[x] == NO_GOODS;
                        data->cargo.cargoSpace++;
                        data->money -= repairListCosts[3];
                        showScoreBoard(data);
                    }
                }
            }
        }
        lastChoice = responseRepair;
    }   
}

void cityMenuRefuel(PlayerData *data) 
{
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
        unsigned char responseRefuel = getMenuChoice(3, 0, fuelList, true, fuelListCosts);
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

void cityMenuPassenger(PlayerData *data, Passenger *tmpPsgrData) 
{
    for (;;) {
        unsigned char psgrList[MAX_PASSENGERS_AVAILABLE+1][10] = {s"return    "};
        unsigned char listSize;
        for (listSize=1; listSize<MAX_PASSENGERS_AVAILABLE+1; listSize++) {
            if (tmpPsgrData[listSize-1].destination.code) {
                tenCharCopy(psgrList[listSize], tmpPsgrData[listSize-1].name);
            } else {
                break;
            }
        }
        unsigned char responsePassenger = getMenuChoice(listSize, 0, psgrList, false, nullptr);
            
        if (responsePassenger == 0) {
            break;
        } else if (addPassenger(data, &tmpPsgrData[responsePassenger-1])) {
            passengerInAnimation();
            removePassengerFromList(tmpPsgrData, responsePassenger-1);
            showScoreBoard(data);
        }
    }
}

void displayQuest(unsigned char questIndex) {
    for (unsigned char y=0;y<QUEST_TEXT_LENGTH/20;y++) {
        putText(
            &allQuests[questIndex].questExplanation[y*20],
            2,
            y+4,
            20,
            VCOL_WHITE);
    }
}
    
void cityMenuQuest(PlayerData *data)
{
    unsigned char lastChoice = 0;
    for (;;) {
        unsigned char invList[MAX_QUESTS_IN_PROGRESS+1][10] = {s"return    "};
        unsigned char invQuestIndexList[MAX_QUESTS_IN_PROGRESS+1] = {0};
        unsigned char invListLength = 1;
        unsigned char x;
        for (x=0; x<MAX_QUESTS_IN_PROGRESS; x++) {
            if (questLog[x].questIndex != INVALID_QUEST_INDEX) {
                tenCharCopy(invList[invListLength], allQuests[questLog[x].questIndex].questTitle);
                invQuestIndexList[invListLength] = questLog[x].questIndex;
                invListLength++;
            }
        }
        unsigned char responseQuest = getMenuChoice(invListLength, lastChoice, invList, false, nullptr);
        
        if (responseQuest == 0) {
            break;
        } else {
            clearWorkScreen();
            putText(getCityNameFromCityCode(allQuests[questLog[invQuestIndexList[responseQuest]].questIndex].cityNumber), 2, 2, 10, VCOL_WHITE);
            displayQuest(invQuestIndexList[responseQuest]);
            lastChoice = responseQuest;
        }
    }
}

void cityMenuInventory(PlayerData *data, Passenger *tmpPsgrData) 
{
    unsigned char lastChoice = 0;
    for (;;) {
        unsigned char invList[4][10] = {s"return    ",s"passengers",s"cargo     ",s"quest     "};
        unsigned char responseInv = getMenuChoice(4, lastChoice, invList, false, nullptr);
        if (responseInv == 0) {
            break;
        } else if (responseInv == 1) {
            showWorkPassengers(data->cargo.psgr);
        } else if (responseInv == 2) {
            showWorkCargo(data);
        } else {
            cityMenuQuest(data);
        }
        lastChoice = responseInv;
    }
    drawBalloonDockScreen();
}

void cityMenuMayor(PlayerData *data)
{
    showMayor(data);
    for (;;) {
        unsigned char mayorList[4][10] = {s"return    ",s"town      ",s"chat      ",s"gift      "};
        unsigned char responseMayor = getMenuChoice(4, 0, mayorList, false, nullptr);

        if (responseMayor == 0) { break; }
        else {
            clearWorkScreen(3);
            if (responseMayor == 1) {
                // list of town needs
                putText (s"my town needs",2,4,13,VCOL_WHITE);
                for (unsigned char x=0; x<MAX_BUY_GOODS; x++) {
                    unsigned char index = cities[currMap][cityNum-1].buyGoods[x].goodsIndex;
                    if (index == NO_GOODS) break;
                    putText (goods[index].name , 3, 5+x, 10, VCOL_WHITE);
                }
            } else if (responseMayor == 2) {
                // the mayor names his next quest
                CityCode cityCode = {CityCode_generateCityCode(currMap,cityNum)};
                unsigned char questIndex = Quest_getCityQuest(
                    cityCode,
                    &cities[currMap][cityNum-1]);
                if (questIndex != INVALID_QUEST_INDEX) {
                    displayQuest(questIndex);
                }
            } else {
                // gift
            }
        }
    }
    drawBalloonDockScreen();
}
    
void cityMenu(PlayerData *data, Passenger *tmpPsgrData) 
{
    for (;;) {
        unsigned char response = getMenuChoice(MAIN_MENU_SIZE, 0, main_menu_options, false, nullptr);
        if (response == MENU_OPTION_BUY) {
            cityMenuBuy(data);            
        } else if (response == MENU_OPTION_SELL) {
            cityMenuSell(data);
        } else if (response == MENU_OPTION_MAYOR) {
            cityMenuMayor(data);
        } else if (response == MENU_OPTION_REPAIR) {
            cityMenuRepair(data);
        } else if (response == MENU_OPTION_REFUEL) {
            cityMenuRefuel(data);
        } else if (response == MENU_OPTION_PASSENGER) {
            cityMenuPassenger(data, tmpPsgrData);
        } else if (response == MENU_OPTION_INVENTORY) {
            cityMenuInventory(data, tmpPsgrData);
        } else if (response == MENU_OPTION_EXIT) {
            break;
        }
    }
}

void checkForLandingPassengers(PlayerData *data)
{
    bool oneLanded = false;
    for (unsigned char p=0; p<MAX_PASSENGERS; p++) {
        CityCode dest = data->cargo.psgr[p].destination;
        if((CityCode_getMapNum(dest) == currMap) && (CityCode_getCityNum(dest) == cityNum)) {
            passengerOutAnimation();
            // take fare
            data->money += data->cargo.psgr[p].fare;
            // add passenger name back to list
            returnName(data->cargo.psgr[p].name);
            // remove passenger
            removePassenger(data, p);
            oneLanded = true;
        }
    }
    if (oneLanded) {
        showScoreBoard(data);
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
    
    Passenger tmpPsgrData[10];
    CityCode cityCode;
    cityCode.code = CityCode_generateCityCode(currMap, cityNum);
    generateCurrentCityTmpData(tmpPsgrData, cityCode);

    const char respect[7] = s"respect";
    // City Name
    putText(cities[currMap][cityNum-1].name, 27, 1, 10, VCOL_WHITE);
    putText(respect, 26, 3, 7, VCOL_DARK_GREY);
    if (cities[currMap][cityNum-1].respect == CITY_RESPECT_NONE) {
        putText(s"n/a ", 34, 3, 4, VCOL_BLACK);
    } else if (cities[currMap][cityNum-1].respect == CITY_RESPECT_LOW){
        putText(s"low ", 34, 3, 4, VCOL_DARK_GREY);
    } else if (cities[currMap][cityNum-1].respect == CITY_RESPECT_MED){
        putText(s"med ", 34, 3, 4, VCOL_BROWN);
    } else {
        putText(s"high", 34, 3, 4, VCOL_YELLOW);
    }

    setupRasterIrqsWorkScreen();
    clearKeyboardCache();
    checkForLandingPassengers(data);
    cityMenu(data, tmpPsgrData);
    
    // return to travelling state
    if (currScreen == 0) {
        vic.memptr = 0x10 | (vic.memptr & 0x0f);
    } else {
        vic.memptr = 0xb0 | (vic.memptr & 0x0f);
    }
    clearRasterIrqs();
    setAveragePosition();
    vic.spr_enable = SPRITE_BALLOON_OUTLINE_ENABLE | SPRITE_BALLOON_BG_ENABLE;
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
        // cabins
        if (data->cargo.psgr[x].destination.code == DESTINATION_CODE_DAMAGED_CABIN) {
            Screen0[863+5+x] = 78;
            ScreenColor[863+5+x] = VCOL_RED;
        } else if (data->cargo.psgr[x].destination.code == DESTINATION_CODE_NO_PASSENGER) {
            Screen0[863+5+x] = 78;
            ScreenColor[863+5+x] = VCOL_WHITE;
        } else {
            Screen0[863+5+x] = 94;
            ScreenColor[863+5+x] = VCOL_GREEN;
        }
        // balloon health
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
            Screen0[943+5+x] = cities[currMap][cityNum-1].name[x];
        }
    } else {
        for (x=0;x<10;x++) {
            Screen0[943+5+x] = 32;
        }
    }
}

void initialiseGameVariables(void)
{
    xScroll = 4;    // middle of scroll
    currScreen = 0; // start with Screen 0 (0x0400)
    flip = 1;       // act like flip has happened, this triggers copy to Screen 1
    yPos = 20480;   // internal Y position of balloon, middle of field
    yVel = 0;       // No starting velocity
    holdCount = 0;
    flameDelay = 0;

    // set up map
    currMap = 0;
    mapXCoord = 0;

    status = 0;
    cityNum = 0;
    vic.spr_enable = 0;
}

void startGame(char *name, unsigned char title)
{
    initialiseGameVariables();
    Quest_init();
    initScreenWithDefaultColors(true);

    struct PlayerData playerData;
    playerDataInit(&playerData, name, title);

    // set up scoreboard
    showScoreBoard(&playerData);
    
    // Initialise clouds
    vic.spr_enable |= SPRITE_CLOUD_OUTLINE_ENABLE | SPRITE_CLOUD_BG_ENABLE; 
    cloudXPos[0] = 0;
    cloudYPos[0] = TOP_OF_SCREEN_RASTER + (rand()&15);
    cloudXPos[1] = 200;
    cloudYPos[1] = TOP_OF_SCREEN_RASTER + 35 + (rand()&15);

    setupTravellingSprites();
    for (unsigned char cloudNum = 0; cloudNum < NUM_CLOUDS; cloudNum++) {
        cloudXPos[cloudNum] = 512;
    }
    int dummy = vic.spr_backcol;  // clear sprite-bg collisions
    dummy = vic.spr_sprcol;       // clear sprite^2 collisions

    // Two Raster IRQs, one at line 20, one at bottom of screen
    setupRasterIrqs();
    
    // Begin scrolling
    status |= STATUS_SCROLLING;

    for (;;) {
        if (vic.spr_backcol & (SPRITE_BALLOON_OUTLINE_ENABLE | SPRITE_BALLOON_BG_ENABLE)) {
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
        //if (sprColl) {debugChar(6,sprColl);}
        if ((sprColl & (SPRITE_RAMP_ENABLE | SPRITE_BALLOON_BG_ENABLE)) == (SPRITE_RAMP_ENABLE | SPRITE_BALLOON_BG_ENABLE)) {
            // Collision with Ramp - GOOD
            landingOccurred(&playerData);
        } else if ((sprColl & (SPRITE_CITY_OUTLINE_ENABLE | SPRITE_BALLOON_BG_ENABLE)) == (SPRITE_CITY_OUTLINE_ENABLE | SPRITE_BALLOON_BG_ENABLE)) {
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
                cloudYPos[cloudNum] = TOP_OF_SCREEN_RASTER + 35*cloudNum + (rand()&15);
                vic.spr_enable |= SPRITE_CLOUD_OUTLINE_ENABLE | SPRITE_CLOUD_BG_ENABLE; 
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
                        vic.spr_enable |= SPRITE_RAMP_ENABLE;
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
                cityXPos = (unsigned int)(256+92);
                cityYPos = 202 - (8*mountainHeight[(terrain[currCoord] & 0x7)]) - 16;
            }
            flip = 0;
        }
    }

    clearRasterIrqs();

}

unsigned char introScreen(char *name, unsigned char* title)
{
    // set up sprites
    setupUpIntroSprites();
    setupRasterIrqsIntro1();
    // go to Screen Work
    vic.memptr = 0xc0 | (vic.memptr & 0x0f);
    vic.ctrl2 = 0xc8; // 40 columns, no scroll
    vic.color_border = VCOL_BLACK;
    vic.color_back = VCOL_BLACK;
    
    clearFullWorkScreen();
    cloudXPos[0] = 0;
    cloudYPos[0] = 80;
    cloudXPos[1] = 150;
    cloudYPos[1] = 160;
    putText(s"odyssey of the aeronaut",2,4,23,VCOL_YELLOW);
    putText(s"an airborne adventure",3,13,21,VCOL_YELLOW);
    putText(s"8-bit gyetko games",11,23,18,VCOL_LT_GREY);
    putText(s"copyright @mmxxv",12,24,16,VCOL_LT_GREY);
    vic_sprxy(SPRITE_BALLOON_OUTLINE,100,100);
    vic_sprxy(SPRITE_BALLOON_BG,100,100);    
    vic_sprxy(SPRITE_BACK_THRUST,103,100);
    vic_sprxy(SPRITE_CLOUD_BG,cloudXPos[0],cloudYPos[0]);
    vic_sprxy(SPRITE_CLOUD_OUTLINE,cloudXPos[0],cloudYPos[0]);
    vic_sprxy(SPRITE_RAMP,cloudXPos[1],cloudYPos[1]);
    vic_sprxy(SPRITE_UP_THRUST,cloudXPos[1],cloudYPos[1]);
    vic.spr_enable = SPRITE_BALLOON_OUTLINE_ENABLE | SPRITE_BALLOON_BG_ENABLE | SPRITE_BACK_THRUST_ENABLE | 
                     SPRITE_CLOUD_BG_ENABLE| SPRITE_CLOUD_OUTLINE_ENABLE | SPRITE_RAMP_ENABLE | SPRITE_UP_THRUST_ENABLE ;
    
    char menuOptions[4][10] = {
    s"new game  ",
    s"load game ",
    s"instructns",
    s"exit game "};
    
    for (;;) {
        unsigned char result = getMenuChoice(4, 0, menuOptions, false, nullptr);
        
        if (result == 0) {
            getInputText(28, 12, 10, s"your name ", name);
            char titleOptions[4][10] = {
                s"mr        ",
                s"mrs       ",
                s"ms        ",
                s"mx        "};
            *title = getMenuChoice(4,0,titleOptions,false,nullptr);
            return 1;
        } else if (result == 1) {
            
        } else if (result == 2) {
            
        } else {
            return 0;
        }
    }
    vic.spr_enable = 0x00;
    clearRasterIrqs();
}

int main(void)
{
    mmap_set(MMAP_NO_BASIC);
 	// Keep kernal alive 
	rirq_init(true);
    vic.memptr = (vic.memptr & 0xf1) | 0x08; // xxxx100x means $2000 for character map

    char tempName[10];
    unsigned char tempTitle;
    for (;;) {
        if (introScreen(tempName, &tempTitle) == 0) break;
        startGame(tempName, tempTitle);
    }

	return 0;
}
