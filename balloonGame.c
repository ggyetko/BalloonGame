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
#include "terrain.h"
#include "playerData.h"
#include "quest.h"
#include "upgrade.h"
#include "sound.h"
#include "factory.h"

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

// My second batch of sprites go here
#pragma section( spriteset2, 0)
#pragma region( spriteset2, 0x3400, 0x4000, , , {spriteset2} )

// everything beyond will be code, data, bss and heap to the end
#pragma region( main, 0x4000, 0xd000, , , {code, data, bss, heap, stack} )

// My font at fixed location
#pragma data(charset)
__export const char charset[2048] = {
	#embed "myFont.bin"
};

// spriteset at fixed location
#pragma data(spriteset)
__export const char spriteset[1024] = { // starts at 0x2800
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

#pragma data(spriteset2)
__export const char spriteset2[1984] = { // starts at 0x3400
    #embed "swirlSpritesFour.bin" // 0xd0 0xd1 0xd2 0xd3
    #embed "faceMayor.bin"        // 0xd4 ... 0xdb
    #embed "faceChef.bin"         // 0xdc ... 0xe3
    #embed "faceEngineer.bin"     // 0xe4 ... 0xeb
    #embed "airdrop.bin"          // 0xec 0xed 0xee        (air drop, mule lt gry, mule dk gry)
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
#define BalloonRampLocation        0xae
#define BalloonCartLocation        0xaf

#define PortalSwirl1Location       0xd0
#define PortalSwirl2Location       0xd1
#define PortalSwirl3Location       0xd2
#define PortalSwirl4Location       0xd3

#define MayorFace1Location         0xd4
#define MayorFace2Location         0xd5
#define MayorFace3Location         0xd6
#define MayorFace4Location         0xd7

#define AirDropLocation            0xec
#define MuleMainLocation           0xed
#define MuleBackgroundLocation     0xee

#pragma data(data)

// Control variables for the interrupt

// .... ...? - Are we scrolling(1) or paused(0)
// .... ..?. - is there a city in Sprite #3?
// .... .?.. - is the city ramp out?
volatile unsigned char status;
enum ScrollingStatuses{
    STATUS_SCROLLING = 0x01,
    STATUS_CITY_VIS  = 0x02,
    STATUS_CITY_RAMP = 0x04,
    STATUS_SWIRL_READY = 0x08,
    STATUS_SWIRL_ON  = 0x10,
    STATUS_AIRDROP_ON = 0x20,
};

#define STATUS_UTILITY_SPRITES (STATUS_AIRDROP_ON|STATUS_SWIRL_ON|STATUS_CITY_RAMP)

enum {
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
volatile unsigned char dummy;

volatile unsigned int yPos; // 20*8 = 160 pixel
                            // 256 positions per pixel,
                            // 256 * 160 = 40960 locations 
                            // 0 is the top of screen, 40960 is bottom of 20th char
volatile int yVel;          // signed int, yPos/50th of a second
                            // +ve velocity is downward, -ve is upward
volatile unsigned int  cityXPos;  // While in travelling mode, where is the city sprite?
volatile unsigned char cityYPos;  // While in travelling mode, where is the city sprite?
volatile unsigned char cityNum;   // While in travelling mode, what city are we near?
volatile unsigned int  swirlXPos;  // While in travelling mode, where is the swirl sprite?
volatile unsigned char swirlYPos;  // While in travelling mode, where is the swirl sprite?
volatile unsigned char portalNextMap; // if you hit the next Portal, you'll go to this map.
volatile unsigned char airDropXPos;
volatile unsigned char airDropYPos;

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

volatile unsigned char currentSound;
volatile unsigned char soundIndex;
volatile unsigned char currWaterChar;

/* ??...... : info - (0=nothing 1,2,3:city number)
 * ..???... : stalagtite length (ceiling)
 * .....??? : stalagmite length (floor)
 */
 
#include "namedGoods.h"

 
const char decelPattern[8] =  {2,3,4,5,6,7,8,16};
const char mountainHeight[8] = {0,1,2,3,4,6,8,10};

// 
// PIXEL and SPRITE mappings
//
#define TOP_OF_RASTER             1
#define SOUND_RASTER             25
#define TOP_OF_SCREEN_RASTER     50
#define HIGH_CLOUD_RASTER_LIMIT  85
#define MID_CLOUD_RASTER_LIMIT  120
#define LOW_RASTER_LIMIT        210
#define BOTTOM_RASTER_LIMIT     250
// SPRITE LIST
//         050-85        086-120               121-210
//      Travelling Hi    Travelling Medium     Travelling Low      Work Screen
// #0 - Balloon Outline  ----------------------------------->      Cargo In
// #1 - Balloon Backgrnd ----------------------------------->      Cargo Out
// #2 - Back thrust      ----------------------------------->      Psgr In
// #3 - Up Thrust        ----------------------------------->      Psgr Out
// #4 -                                        Ramp/Swirl          Face 1
// #5 -                                        City Outline/swirl2 Face 2
// #6 - Cloud Outline    ----------------->    City Shape          Face 3
// #7 - Cloud Background ----------------->    City BackShade      Face 4

#define SPRITE_BALLOON_OUTLINE 0
#define SPRITE_BALLOON_BG      1
#define SPRITE_BACK_THRUST     2
#define SPRITE_UP_THRUST       3
//#define SPRITE_UNUSED        4
#define SPRITE_RAMP            5
#define SPRITE_SWIRL           5
#define SPRITE_AIRDROP         5
#define SPRITE_SWIRL2          6  // not in use yet
#define SPRITE_CLOUD_OUTLINE   6
#define SPRITE_CLOUD_BG        7

#define SPRITE_CITY_OUTLINE    4
#define SPRITE_CITY_BG         6
#define SPRITE_CITY_SHADE      7


#define SPRITE_BALLOON_OUTLINE_ENABLE 0x01
#define SPRITE_BALLOON_BG_ENABLE      0x02
#define SPRITE_BACK_THRUST_ENABLE     0x04
#define SPRITE_UP_THRUST_ENABLE       0x08
#define SPRITE_RAMP_ENABLE            0x20
#define SPRITE_SWIRL_ENABLE           0x20
#define SPRITE_AIRDROP_ENABLE         0x20
#define SPRITE_CLOUD_OUTLINE_ENABLE   0x40
#define SPRITE_CLOUD_BG_ENABLE        0x80

#define SPRITE_CITY_OUTLINE_ENABLE    0x10
#define SPRITE_CITY_BG_ENABLE         0x40
#define SPRITE_CITY_SHADE_ENABLE      0x80


#define SPRITE_CARGO_IN        0
#define SPRITE_CARGO_OUT       1
#define SPRITE_PSGR_IN         2
#define SPRITE_PSGR_OUT        3
#define SPRITE_FACE1           4
#define SPRITE_FACE2           5
#define SPRITE_FACE3           6
#define SPRITE_FACE4           7

#define SPRITE_CARGO_IN_ENABLE        0x01
#define SPRITE_CARGO_OUT_ENABLE       0x02
#define SPRITE_PSGR_IN_ENABLE         0x04
#define SPRITE_PSGR_OUT_ENABLE        0x08

// 
// Character map location
// 
#define LOWEST_SCROLLING_CHAR_ROW   19

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

__interrupt void soundInterrupt(void)
{
    Sound_tick();
}

__interrupt void prepWorkScreen(void)
{
    // work screen is 0xc0
    vic.memptr = 0xc0 | (vic.memptr & 0x0f);
    vic.color_back = palette[currMap].skyColor;
    
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
        Screen0[0x03f8+SPRITE_CLOUD_OUTLINE   ] = BalloonCloudOutlLocation;
        Screen0[0x03f8+SPRITE_CLOUD_BG        ] = BalloonCloudLocation;
    } else {
        vic.memptr = 0xb0 | (vic.memptr & 0x0f); // point to screen1
        Screen1[0x03f8+SPRITE_CLOUD_OUTLINE   ] = BalloonCloudOutlLocation;
        Screen1[0x03f8+SPRITE_CLOUD_BG        ] = BalloonCloudLocation;
    }
    // set up the cloud sprite pointers
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
    
    if (status & STATUS_SWIRL_ON) {
        Screen0[0x03f8+SPRITE_SWIRL] = PortalSwirl1Location + (counter & 0x3); // choose from 4 swirl sprites
        Screen1[0x03f8+SPRITE_SWIRL] = PortalSwirl1Location + (counter & 0x3); // choose from 4 swirl sprites
        vic.spr_enable |= SPRITE_SWIRL_ENABLE;
        vic.spr_color[SPRITE_SWIRL] = PORTAL_COLOR;
        vic_sprxy(SPRITE_SWIRL, swirlXPos, swirlYPos);
    }
    
    if ((counter % 5) == 0) {
        byte newWaterChar = currWaterChar + 1;
        if (newWaterChar == 41) {
            newWaterChar = 37;
        }
        for (byte x=1; x<40; x++) {
            if (Screen1[40*LOWEST_SCROLLING_CHAR_ROW+x] == currWaterChar) {
                Screen1[40*LOWEST_SCROLLING_CHAR_ROW+x] = newWaterChar;
            }
            if (Screen0[40*LOWEST_SCROLLING_CHAR_ROW+x] == currWaterChar) {
                Screen0[40*LOWEST_SCROLLING_CHAR_ROW+x] = newWaterChar;
            }
        }
        currWaterChar = newWaterChar;
    }
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
        unsigned char cityStatus = citiesVar[currMap][cityNum-1].status;
        if (cityStatus == CITY_STATUS_CITY) {
            Screen0[0x03f8+SPRITE_CITY_OUTLINE ] = BalloonCityOutlLocation; 
            Screen1[0x03f8+SPRITE_CITY_OUTLINE ] = BalloonCityOutlLocation;
            vic.spr_color[SPRITE_CITY_OUTLINE] = CITY_OUTLINE_COLOR;
            
            Screen0[0x03f8+SPRITE_CITY_BG ] = BalloonCityBgLocation;
            Screen1[0x03f8+SPRITE_CITY_BG ] = BalloonCityBgLocation;
            vic.spr_color[SPRITE_CITY_BG] = palette[currMap].cityColor;
            
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
            Screen0[0x03f8+SPRITE_CITY_OUTLINE ] = MuleMainLocation;
            Screen1[0x03f8+SPRITE_CITY_OUTLINE ] = MuleMainLocation;
            vic.spr_color[SPRITE_CITY_OUTLINE] = VCOL_MED_GREY;

            Screen0[0x03f8+SPRITE_CITY_BG ] = MuleBackgroundLocation;
            Screen1[0x03f8+SPRITE_CITY_BG ] = MuleBackgroundLocation;
            vic.spr_color[SPRITE_CITY_BG] = VCOL_DARK_GREY;

            vic.spr_enable |= SPRITE_CITY_BG_ENABLE | SPRITE_CITY_OUTLINE_ENABLE;            

            vic_sprxy(SPRITE_CITY_OUTLINE, cityXPos, cityYPos);
            vic_sprxy(SPRITE_CITY_BG, cityXPos, cityYPos);
        }
        
    } else {
        vic.spr_enable &= ~(SPRITE_CITY_BG_ENABLE | SPRITE_CITY_OUTLINE_ENABLE | SPRITE_CITY_SHADE_ENABLE);
    }
    if (status & STATUS_AIRDROP_ON) {
        vic_sprxy(SPRITE_AIRDROP, airDropXPos, airDropYPos);
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
    // handle city ramp status    
    if (cityXPos) {
        if (status & STATUS_CITY_RAMP){
            if (cityXPos < 80) {
                status &= ~STATUS_CITY_RAMP;
                vic.spr_enable &= ~SPRITE_RAMP_ENABLE;
                cityNum = 0;
            }
        }
    } 
}

__interrupt void scrollLeft(void)
{
    vic.color_back = palette[currMap].skyColor;
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
            if (status & STATUS_SWIRL_ON) {
                swirlXPos --;
                if (swirlXPos == 0) {
                    status &= ~(STATUS_SWIRL_ON | STATUS_SWIRL_READY);
                }
            } else if (status & STATUS_AIRDROP_ON) {
                airDropYPos++;
            }
        } else {
            if (status & STATUS_AIRDROP_ON) {
                airDropXPos++;
                if (airDropXPos > 250) {
                    status &= ~STATUS_AIRDROP_ON;
                    vic.spr_enable &= ~SPRITE_AIRDROP_ENABLE;
                }
                airDropYPos++;
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

RIRQCode spmux[6];
void setupRasterIrqs(void)
{
    rirq_stop();
 
    rirq_build(spmux, 1);
    rirq_call(spmux, 0, prepScreen);
    rirq_set(0, TOP_OF_RASTER, spmux);

    rirq_build(spmux+1, 1);
    rirq_call(spmux+1, 0, soundInterrupt);
    rirq_set(1, SOUND_RASTER, spmux+1);

    rirq_build(spmux+2, 1);
    rirq_call(spmux+2, 0, midCloudAdjustment);
    rirq_set(2, HIGH_CLOUD_RASTER_LIMIT, spmux+2);

    rirq_build(spmux+3, 1);
    rirq_call(spmux+3, 0, lowCloudAdjustment);
    rirq_set(3, MID_CLOUD_RASTER_LIMIT, spmux+3);

    rirq_build(spmux+4, 1);
    rirq_call(spmux+4, 0, lowerStatBar);
    rirq_set(4, LOW_RASTER_LIMIT, spmux+4);

    rirq_build(spmux+5, 1);
    rirq_call(spmux+5, 0, scrollLeft);
    rirq_set(5, BOTTOM_RASTER_LIMIT, spmux+5);

	// Sort interrupts and start processing
	rirq_sort();
	rirq_start();
}

void setupRasterIrqsIntro1(void)
{
    rirq_stop();

    rirq_build(spmux, 1);
    rirq_call(spmux, 0, soundInterrupt);
    rirq_set(0, SOUND_RASTER, spmux);

    rirq_build(spmux+1, 1);
    rirq_call(spmux+1, 0, introScreenInterrupt);
    rirq_set(1, BOTTOM_RASTER_LIMIT, spmux+1);

	// Sort interrupts and start processing
	rirq_sort();
	rirq_start();    
}

void setupRasterIrqsWorkScreen(void)
{
    rirq_stop();
    rirq_build(spmux, 1);
    rirq_call(spmux, 0, prepWorkScreen);
    rirq_set(0, TOP_OF_RASTER, spmux);

    rirq_build(spmux+1, 1);
    rirq_call(spmux+1, 0, soundInterrupt);
    rirq_set(1, SOUND_RASTER, spmux+1);

    rirq_build(spmux+2, 1);
    rirq_call(spmux+2, 0, lowerStatBarWorkScreen);
    rirq_set(2, 210, spmux+2);

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
    rirq_clear(5);
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
    cargoInXPos = 0;
    cargoInYPos = 0;
    cargoOutXPos = 0;
    cargoOutYPos = 0;
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

	Screen0[0x03f8+SPRITE_RAMP            ] = BalloonRampLocation; // 0xa6 * 0x40 = 0x2980 (city bridge sprite)
	Screen1[0x03f8+SPRITE_RAMP            ] = BalloonRampLocation;

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
    vic.spr_color[SPRITE_RAMP] = palette[currMap].rampColor;
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
    byte vChange = 5 * data->balloonHealth + 10;
    if ((data->balloonUpgrades & BALLOON_FIREPROOF) == 0) {
        vChange = vChange >> worldTypeDamages[terrainEnvironment[currMap]].heatPowerFactor;
    }
    if (yVel - vChange < -255) {
        yVel = -255;
    } else {
        yVel -= vChange;
    }
}

void invokeAirDrop(void)
{
    // The 16 should make it look like it falls out of the bottom of the balloon
    airDropXPos = 80;
    airDropYPos = (yPos>>8) + 24 + 16;
    vic_sprxy(SPRITE_AIRDROP, airDropXPos, airDropYPos); 
    Screen0[0x03f8+SPRITE_AIRDROP] = AirDropLocation;
	Screen1[0x03f8+SPRITE_AIRDROP] = AirDropLocation;
    vic.spr_enable |= SPRITE_AIRDROP_ENABLE;
    status |= STATUS_AIRDROP_ON;
}

void invokeRamp(void)
{
    Screen0[0x03f8+SPRITE_RAMP] = BalloonRampLocation;
	Screen1[0x03f8+SPRITE_RAMP] = BalloonRampLocation;
    status |= STATUS_CITY_RAMP;
    vic.spr_enable |= SPRITE_RAMP_ENABLE;    
}

const char SIZEOFSCOREBOARDMAP = 40;
const unsigned char colorMapScoreBoard[SIZEOFSCOREBOARDMAP] = {
    40, VCOL_YELLOW,
    5,  VCOL_WHITE, 17, VCOL_GREEN, 1, VCOL_YELLOW, 4, VCOL_WHITE, 13, VCOL_GREEN,
    5,  VCOL_WHITE, 9, VCOL_YELLOW,
        1, upgrades[0].screenColor, 1, upgrades[1].screenColor, 1, upgrades[2].screenColor, 1, upgrades[3].screenColor,
        1, upgrades[4].screenColor, 1, upgrades[5].screenColor,
        7, VCOL_WHITE, 13, VCOL_GREEN,
    5,  VCOL_ORANGE, 17, VCOL_WHITE, 5, VCOL_WHITE, 13, VCOL_WHITE
};
void initScreenWithDefaultColors(bool clearScreen) {
    vic.color_border = VCOL_BLACK;
    vic.color_back = palette[currMap].skyColor;
    unsigned int x = 0;
    while (x<20*40) {
        ScreenColor[x] = palette[currMap].mountainColor;
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
    unsigned char stalactite = mountainHeight[(terrain[currMap][borders] >> 3) & 0x07] * 8;
    unsigned char stalacmite = (LOWEST_SCROLLING_CHAR_ROW - mountainHeight[(terrain[currMap][borders]) & 0x07]) * 8;
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
// returns 2 if GAME OVER
unsigned char terrainCollisionOccurred()
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
        char buyMenuOptions[MAX_SELL_GOODS+2][10];
        unsigned int buyMenuCosts[MAX_SELL_GOODS+2];
        char buyMenuCostsText[MAX_SELL_GOODS+2][10];
        byte buyMenuGoodsIndex[MAX_SELL_GOODS+2];

        memset(buyMenuCostsText, 32, 10*(MAX_SELL_GOODS+1));

        tenCharCopy(buyMenuOptions[0], s"return    ");
        buyMenuCosts[0] = 0;
        tenCharCopy(buyMenuCostsText[0], s"          ");
        for (x = 1; x < MAX_SELL_GOODS+1; x++){
            if (citiesVar[currMap][cityNum-1].respectLevel >= cities[currMap][cityNum-1].sellGoods[x-1].reqRespectRate) {
                tenCharCopy(
                    buyMenuOptions[x], 
                    goods[cities[currMap][cityNum-1].sellGoods[x-1].goodsIndex].name);
                buyMenuCosts[x] =
                    goods[cities[currMap][cityNum-1].sellGoods[x-1].goodsIndex].normalCost
                    / cities[currMap][cityNum-1].sellGoods[x-1].priceAdjustment;
                uint16ToString(buyMenuCosts[x], buyMenuCostsText[x]);
                buyMenuGoodsIndex[x] = cities[currMap][cityNum-1].sellGoods[x-1].goodsIndex;
            } else {
                // The city's goods list is sorted from lowest to highest respect
                break;
            }
        }
        byte factoryItemCount = 0;
        byte facIndex = cities[currMap][cityNum-1].factoryIndex;
        debugChar(0, facIndex);
        if (facIndex != FACTORY_INDEX_NONE) {
            factoryItemCount = Factory_getOutputCount(facIndex);
            if (factoryItemCount) {
                debugChar(1, 1);
                tenCharCopy(buyMenuOptions[x], goods[Factory_getOutputType(facIndex)].name);
                buyMenuCosts[x] = goods[Factory_getOutputType(facIndex)].normalCost >> 1; // always half price
                uint16ToString(buyMenuCosts[x], buyMenuCostsText[x]);
                buyMenuGoodsIndex[x] = Factory_getOutputType(facIndex);
                x++;
            } else {
                debugChar(1, 0);
            }
        }
        unsigned char responseBuy = getMenuChoice(
            x, lastChoice, palette[currMap].inactiveTextColor, buyMenuOptions, true, s"cost      ", buyMenuCostsText);
        if (responseBuy == 0) {
            break;
        } else {
            if ((buyMenuCosts[responseBuy] < data->money) && 
                (addCargoIfPossible(data, buyMenuGoodsIndex[responseBuy])))
            {
                if (factoryItemCount && (responseBuy == x-1)) {
                    Factory_takeOutput(facIndex);
                }
                data->money -= buyMenuCosts[responseBuy];
                cargoInAnimation();
                Sound_doSound(SOUND_EFFECT_ROLLCAR);
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
        char sellMenuCostsText[MAX_CARGO_SPACE+1][10];
        unsigned char x;

        memset(sellMenuCostsText, 32, 10*(MAX_CARGO_SPACE+1));
        tenCharCopy(sellMenuOptions[0], s"return    ");
        sellMenuCosts[0] = 0;
        for (x = 1; x < listLength+1; x++) {
            tenCharCopy( sellMenuOptions[x], goods[goodsIndexList[x-1]].name);
            sellMenuCosts[x] = getGoodsPurchasePrice(&cities[currMap][cityNum-1], goodsIndexList[x-1], goods[goodsIndexList[x-1]].normalCost);
            uint16ToString(sellMenuCosts[x], sellMenuCostsText[x]);
        }
        unsigned char responseSell = getMenuChoice(
            x, lastChoice, palette[currMap].inactiveTextColor, sellMenuOptions, true, s"cost      ", sellMenuCostsText);
        if (responseSell == 0) {
            break;
        } else {
            cargoOutAnimation();
            Quest_processDeliverTrigger(goodsIndexList[responseSell-1], CityCode_generateCityCode(currMap, cityNum));
            if (cities[currMap][cityNum-1].factoryIndex != FACTORY_INDEX_NONE) {
                if (Factory_addGoodsToFactory(cities[currMap][cityNum-1].factoryIndex, goodsIndexList[responseSell-1], 1)) {
                    Sound_doSound(SOUND_EFFECT_FACT_WHISTLE);
                } else {
                    Sound_doSound(SOUND_EFFECT_ROLLCAR);
                }
            } else {
                Sound_doSound(SOUND_EFFECT_ROLLCAR);
            }
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
        char repairList[4][10] = 
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
        char repairCostText[4][10];
        memset(repairCostText, 32, 40);
        for (unsigned char x = 1;x<4;x++) {
            uint16ToString(repairListCosts[x], repairCostText[x]);
        }
            
        unsigned char responseRepair = getMenuChoice(
            4, 0, palette[currMap].inactiveTextColor, repairList, true, s"cost      ", repairCostText);
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
        char fuelListCostText[3][10] =  {s"          ",s"          ",s"          "};
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
        for (unsigned char x = 1;x<3;x++) {
            if (fuelListCosts[x]) {
                uint16ToString(fuelListCosts[x], fuelListCostText[x]);
            }
        }
        unsigned char responseRefuel = getMenuChoice(
            3, 0, palette[currMap].inactiveTextColor, fuelList, true, s"cost      ", fuelListCostText);
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
        unsigned char destList[MAX_PASSENGERS_AVAILABLE+1][10] = {s"          "};
        unsigned char listSize;
        for (listSize=1; listSize<MAX_PASSENGERS_AVAILABLE+1; listSize++) {
            if (tmpPsgrData[listSize-1].destination.code) {
                tenCharCopy(psgrList[listSize], tmpPsgrData[listSize-1].name);
                tenCharCopy(destList[listSize], getCityNameFromCityCode(tmpPsgrData[listSize-1].destination));
            } else {
                break;
            }
        }
        unsigned char responsePassenger = getMenuChoice(
            listSize, 0, palette[currMap].inactiveTextColor, psgrList, true, s"dest      ", destList);
            
        if (responsePassenger == 0) {
            break;
        } else if (addPassenger(data, &tmpPsgrData[responsePassenger-1])) {
            Sound_doSound(SOUND_EFFECT_ROLLCAR);
            passengerInAnimation();
            Quest_processBoardingTrigger(tmpPsgrData[responsePassenger-1].name);
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
            y+9,
            20,
            VCOL_WHITE);
    }
}
    
void cityMenuQuest(PlayerData *data)
{
    unsigned char lastChoice = 0;
    for (;;) {
        char invList[MAX_QUESTS_IN_PROGRESS+1][10] = {s"return    "};
        unsigned char invQuestIndexList[MAX_QUESTS_IN_PROGRESS+1] = {0}; // this list will contain the QUEST index (not the QUEST LOG index)
        unsigned char invListLength = 1;
        unsigned char questLogIndex;
        for (questLogIndex=0; questLogIndex<MAX_QUESTS_IN_PROGRESS; questLogIndex++) {
            if (questLog[questLogIndex].questIndex != INVALID_QUEST_INDEX) {
                tenCharCopy(invList[invListLength], allQuests[questLog[questLogIndex].questIndex].questTitle);
                invQuestIndexList[invListLength] = questLogIndex; //questLog[questLogIndex].questIndex;
                invListLength++;
            }
        }
        unsigned char responseQuest = getMenuChoice(
            invListLength, lastChoice, palette[currMap].inactiveTextColor, invList, false, nullptr, nullptr);
        
        if (responseQuest == 0) {
            break;
        } else {
            clearWorkScreen();
            unsigned char qli = invQuestIndexList[responseQuest];
            unsigned char qi = questLog[qli].questIndex;
            // show home city of quest
            putText(getCityNameFromCityCode(allQuests[qi].cityNumber), 2, 2, 10, VCOL_WHITE);
            displayQuest(qi);
            lastChoice = responseQuest;
            // show fraction complete
            char out[3];
            unsigned char num = questLog[qli].completeness;
            ucharToString(num, out);
            putText(out, 2, 14, 3, VCOL_DARK_GREY);
            num = allQuests[qi].numItems;
            ucharToString(num, out);
            putText(out, 8, 14, 3, VCOL_DARK_GREY);  
            ScreenColor[6 + 14*40] = VCOL_DARK_GREY;
            ScreenWork[6 + 14*40] = 59;
        }
    }
}

void cityMenuInventory(PlayerData *data, Passenger *tmpPsgrData) 
{
    unsigned char lastChoice = 0;
    for (;;) {
        unsigned char invList[5][10] = {s"return    ",s"passengers",s"cargo     ",s"quest     ",s"maps      "};
        unsigned char responseInv = getMenuChoice(
            5, lastChoice, palette[currMap].inactiveTextColor, invList, false, nullptr, nullptr);
        if (responseInv == 0) {
            break;
        } else if (responseInv == 1) {
            showWorkPassengers(data->cargo.psgr, palette[currMap].inactiveTextColor);
        } else if (responseInv == 2) {
            showWorkCargo(data, palette[currMap].inactiveTextColor);
        } else if (responseInv == 3) {
            cityMenuQuest(data);
        } else {
            showWorkMaps(data);
        }
        lastChoice = responseInv;
    }
    drawBalloonDockScreen(palette[currMap].cityColor);
}

void finishQuest(PlayerData *data, unsigned char questIndex)
{
    bool success = false;
    for (unsigned char y=0;y<QUEST_CONCLUSION_TEXT_LENGTH/20;y++) {
        putText(
            &allQuests[questIndex].questConclusion[y*20],
            2,
            y+9,
            20,
            VCOL_WHITE);
    }
    unsigned char rw = allQuests[questIndex].reward.rewardType;
    if (rw == REWARD_RESPECT_MED) {
        citiesVar[currMap][cityNum-1].respectLevel = CITY_RESPECT_MED;
        success = true;
    } else if (rw == REWARD_RESPECT_HIGH) {
        citiesVar[currMap][cityNum-1].respectLevel = CITY_RESPECT_HIGH;
        success = true;
    } else if (rw == REWARD_SPECIAL_ITEM) {
        // in case there's no room, we will leave the quest "complete but unclaimed"
        success = addCargoIfPossible(data, allQuests[questIndex].reward.index);
    } else if (rw == REWARD_MAP_ACCESS) {
        addMapAccessible(data, allQuests[questIndex].reward.index);
    }
    if (success) {
        
    }
}

void updateCityWindow(void)
{
    const char respect[7] = s"respect";
    // City Name
    putText(cities[currMap][cityNum-1].name, 27, 1, 10, VCOL_WHITE);
    putText(respect, 26, 3, 7, VCOL_DARK_GREY);
    if (citiesVar[currMap][cityNum-1].respectLevel == CITY_RESPECT_NONE) {
        putText(s"n/a ", 34, 3, 4, VCOL_BLACK);
    } else if (citiesVar[currMap][cityNum-1].respectLevel == CITY_RESPECT_LOW){
        putText(s"low ", 34, 3, 4, VCOL_DARK_GREY);
    } else if (citiesVar[currMap][cityNum-1].respectLevel == CITY_RESPECT_MED){
        putText(s"med ", 34, 3, 4, VCOL_DARK_GREY);
    } else {
        putText(s"high", 34, 3, 4, VCOL_YELLOW);
    }   
}

void cityMenuUpgrade(PlayerData *data)
{
    for (;;) {
        unsigned char listLength = 1;
        char upgradeList[4][10] = {s"return    "};
        int costList[4];
        char costListText[4][10];
        memset (costListText,32,40);
        unsigned char upgradeIndexList[4] = {0};
        if (citiesVar[currMap][cityNum-1].respectLevel == CITY_RESPECT_HIGH) {
            for (unsigned char x=0; x<UPGRADE_NUM_UPGRADES ;x++) {
                if (cities[currMap][cityNum-1].facility & upgrades[x].facilityMask) {
                    tenCharCopy(upgradeList[listLength], upgrades[x].name);
                    costList[listLength] = upgrades[x].cost;
                    uint16ToString(costList[listLength], costListText[listLength]);
                    upgradeIndexList[listLength] = x;
                    listLength++;
                }
            }
        }
        unsigned char respUpgrade = getMenuChoice(
            listLength,0, palette[currMap].inactiveTextColor,upgradeList,true,s"cost      ",costListText);
        if (respUpgrade == 0) {
            return;
        } else {
            // If you can afford the upgrade and don't already have it
            if ((costList[respUpgrade] <= data->money) 
                && ((upgrades[upgradeIndexList[respUpgrade]].upgradeMask & data->balloonUpgrades) == 0) ) {
                data->balloonUpgrades |= upgrades[upgradeIndexList[respUpgrade]].upgradeMask;
                data->money -= costList[respUpgrade];
                showScoreBoard(data);
            }
        }
    }
}

void displayMayorFace(void)
{
    drawBox(0,0,6,7,VCOL_LT_GREEN);
    ScreenWork[0x03f8+SPRITE_FACE1] = MayorFace1Location;
    ScreenWork[0x03f8+SPRITE_FACE2] = MayorFace2Location;
    ScreenWork[0x03f8+SPRITE_FACE3] = MayorFace3Location;
    ScreenWork[0x03f8+SPRITE_FACE4] = MayorFace4Location;
    vic_sprxy(SPRITE_FACE1, 28, 60);
    vic_sprxy(SPRITE_FACE2, 52, 60);
    vic_sprxy(SPRITE_FACE3, 28, 81);
    vic_sprxy(SPRITE_FACE4, 52, 81);
    vic.spr_color[SPRITE_FACE1] = VCOL_BLACK;
    vic.spr_color[SPRITE_FACE2] = VCOL_BLACK;
    vic.spr_color[SPRITE_FACE3] = VCOL_BLACK;
    vic.spr_color[SPRITE_FACE4] = VCOL_BLACK;
    vic.spr_enable |= 0xF0;
}

void removeMayorFace(void)
{
    vic.spr_enable &= ~0xF0;
}

void cityMenuMayor(PlayerData *data, Passenger *tmpPsgrData)
{
    clearWorkScreen();
    displayMayorFace();
    showMayor(data);
    for (;;) {
        unsigned char mayorList[5][10] = {s"return    ",s"town      ",s"quest     ",s"chat      ", s"gift      "};
        unsigned char responseMayor = getMenuChoice(
            5, 0, palette[currMap].inactiveTextColor, mayorList, false, nullptr, nullptr);

        if (responseMayor == 0) { break; }
        else {
            clearWorkScreen(8);
            if (responseMayor == 1) {
                // list of town needs
                putText (s"my town needs",2,9,13,VCOL_WHITE);
                for (unsigned char x=0; x<MAX_BUY_GOODS; x++) {
                    unsigned char index = cities[currMap][cityNum-1].buyGoods[x].goodsIndex;
                    if (index == NO_GOODS) break;
                    putText (goods[index].name , 3, 10+x, 10, VCOL_WHITE);
                }
            } else if (responseMayor == 2) {
                CityCode cityCode = CityCode_generateCityCode(currMap,cityNum);
                // check for quest completion first
                unsigned int completedQuestIndex = Quest_checkComplete(cityCode);
                if (completedQuestIndex != INVALID_QUEST_INDEX) {
                    Sound_doSound(SOUND_EFFECT_QUEST_DONE);
                    finishQuest(data, completedQuestIndex);
                    showScoreBoard(data);
                    updateCityWindow();
                } else {
                    // check if the mayor has a new quest
                    unsigned char questIndex = Quest_getCityQuest(
                        cityCode,
                        citiesVar[currMap][cityNum-1].respectLevel,
                        tmpPsgrData);
                    if (questIndex != INVALID_QUEST_INDEX) {
                        Sound_doSound(SOUND_EFFECT_QUEST_RING);
                        displayQuest(questIndex);
                    }
                }
            } else if (responseMayor == 3) {
                // list of city's gameinfo text
                for (unsigned char y=0; y<CITY_GAMEINFO_SIZE/20; y++) {
                    putText (&(cities[currMap][cityNum-1].gameInfo[y*20]), 2, 9+y, 20, VCOL_WHITE);
                }
            } else {
                // gift
            }
        }
    }
    removeMayorFace();
    drawBalloonDockScreen(palette[currMap].cityColor);
}
    
void cityMenu(PlayerData *data, Passenger *tmpPsgrData) 
{
    for (;;) {
        unsigned char response = getMenuChoice(
            MAIN_MENU_SIZE, 0, palette[currMap].inactiveTextColor, main_menu_options, false, nullptr, nullptr);
        if (response == MENU_OPTION_BUY) {
            cityMenuBuy(data);            
        } else if (response == MENU_OPTION_SELL) {
            cityMenuSell(data);
        } else if (response == MENU_OPTION_MAYOR) {
            cityMenuMayor(data, tmpPsgrData);
        } else if (response == MENU_OPTION_REPAIR) {
            cityMenuRepair(data);
        } else if (response == MENU_OPTION_REFUEL) {
            cityMenuRefuel(data);
        } else if (response == MENU_OPTION_UPGRADE) {
            cityMenuUpgrade(data);
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
            Sound_doSound(SOUND_EFFECT_ROLLCAR);
            passengerOutAnimation();
            // take fare
            data->money += data->cargo.psgr[p].fare;
            // inform quest code
            Quest_processArrivalTrigger(data->cargo.psgr[p].name, CityCode_generateCityCode(currMap, cityNum));
            // add passenger name back to list
            City_returnName(data->cargo.psgr[p].name);
            // remove passenger
            removePassenger(data, p);
            oneLanded = true;
        }
    }
    if (oneLanded) {
        showScoreBoard(data);
    }
}

void clearCollisions(void)
{
    dummy = vic.spr_backcol;  // clear sprite-bg collisions
    dummy = vic.spr_sprcol;       // clear sprite^2 collisions
}

void portalEntered(void)
{
    // Three seconds of no-scrolling on new map
    setScrollActive(false, 150);
    clearBalloonScreens();
    
    clearRasterIrqs();
    clearMovement();
    // turn off ALL sprites
    vic.spr_enable = 0x00;
    
    currMap = portalNextMap;
    initScreenWithDefaultColors(false);
    yPos = 20480;   // internal Y position of balloon, middle of field
    mapXCoord = 0;

    status = 0;
    cityNum = 0;
    vic.spr_enable = 0;

    setupRasterIrqs();
    setupTravellingSprites();
    vic.spr_enable = SPRITE_BALLOON_OUTLINE_ENABLE | SPRITE_BALLOON_BG_ENABLE;
    clearCollisions();
}

void landingOccurred(PlayerData *data)
{
    clearRasterIrqs();
    clearMovement();
    // turn off ALL sprites
    vic.spr_enable = 0x00;
    setupUpCargoSprites();
    status = 0;
    // go to Screen Work
    vic.memptr = 0xc0 | (vic.memptr & 0x0f);
    vic.ctrl2 = 0xc8; // 40 columns, no scroll
    vic.color_back = palette[currMap].skyColor; // TBD, is this a good idea? May harm text visibility
    
    drawBox(0,0,24,19,VCOL_YELLOW);
    drawBox(0,20,39,24,VCOL_YELLOW);
    drawBox(25,0,39,4,VCOL_BLACK);
    drawBox(25,5,39,19,VCOL_BLACK);
    drawBalloonDockScreen(palette[currMap].cityColor);
    
    Passenger tmpPsgrData[10];
    CityCode cityCode = CityCode_generateCityCode(currMap, cityNum);
    City_generateCurrentCityTmpData(tmpPsgrData, cityCode);

    updateCityWindow();

    setupRasterIrqsWorkScreen();
    clearKeyboardCache();
    
    // Data handling
    if (data->coldDamage) {
        if (data->coldDamage < 50) { data->coldDamage = 0; }
        else { data->coldDamage -= 50; }
    }
    checkForLandingPassengers(data);
    cityMenu(data, tmpPsgrData);
    City_returnUnusedPassengers(tmpPsgrData);
    status = STATUS_CITY_VIS;
    
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
    clearCollisions();
}

// Pass in an array of 3 roof (left, current, right) and 3 ground (same)
// Returns the character code for the top of the stalacmite and the bottom of the stalactite
void getFinalChars(char const* roof, char const* ground, unsigned char *finalRoofChar, unsigned char *finalGroundChar ){
    *finalRoofChar = 65; // peak
    if ((roof[0] <= roof[1]) && (roof[1] < roof[2])) {
        *finalRoofChar = 67;
    } else if ((roof[0] > roof[1]) && (roof[1] >= roof[2])) {
        *finalRoofChar = 66;
    }
    
    *finalGroundChar = 69; // peak
    if (ground[1] == LOWEST_SCROLLING_CHAR_ROW) {
        *finalGroundChar = currWaterChar; // water character
    }
    else if ((ground[0] < ground[1]) && (ground[1] <= ground[2])) {
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
    // Upgrades
    for (unsigned char x=0; x<UPGRADE_NUM_UPGRADES; x++) {
        Screen0[894+x] = upgrades[x].screenChar;
        if (data->balloonUpgrades & upgrades[x].upgradeMask) {
            ScreenColor[894+x] = upgrades[x].screenColor;
        } else {
            ScreenColor[894+x] = VCOL_DARK_GREY;
        }
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

void initialiseGameVariables()
{
    Quest_init();
    City_initCityVariables();
    Factory_initFactoryStatuses();

    xScroll = 4;    // middle of scroll
    currScreen = 0; // start with Screen 0 (0x0400)
    flip = 1;       // act like flip has happened, this triggers copy to Screen 1
    yPos = 20480;   // internal Y position of balloon, middle of field
    yVel = 0;       // No starting velocity
    holdCount = 0;
    flameDelay = 0;
    currWaterChar = 37; // Pick any current water char 37-40

    // set up map
    currMap = 0;
    mapXCoord = 0;

    status = 0;
    cityNum = 0;
    vic.spr_enable = 0;
}

void checkBalloonDamage(PlayerData *data)
{
    if((data->balloonUpgrades & BALLOON_ICEPROOF) == 0) {
        if (worldTypeDamages[terrainEnvironment[currMap]].coldDamage) {
            data->coldDamage += worldTypeDamages[terrainEnvironment[currMap]].coldDamage;
            if (data->coldDamage >= 200) {
                data->coldDamage = 0;
                balloonDamage(data);
            }
        }
    }
}

void startGame(char *name, unsigned char title)
{
    initialiseGameVariables();
    initScreenWithDefaultColors(true);

    struct PlayerData playerData;
    playerDataInit(&playerData, name, title);
    
    // DEBUG TBD
    playerData.knownMaps = 3;
    playerData.balloonUpgrades = BALLOON_PORTAL;
          
    // set up scoreboard
    showScoreBoard(&playerData);
    
    // Initialise clouds
    vic.spr_enable |= SPRITE_CLOUD_OUTLINE_ENABLE | SPRITE_CLOUD_BG_ENABLE; 
    cloudXPos[0] = 0;
    cloudYPos[0] = TOP_OF_SCREEN_RASTER + (rand()&15);
    cloudXPos[1] = 200;
    cloudYPos[1] = TOP_OF_SCREEN_RASTER + 35 + (rand()&15);

    setupTravellingSprites();
    Sound_startSong(SOUND_SONG_AIRBORNE);
    for (unsigned char cloudNum = 0; cloudNum < NUM_CLOUDS; cloudNum++) {
        cloudXPos[cloudNum] = 512;
    }
    clearCollisions();

    // Two Raster IRQs, one at line 20, one at bottom of screen
    setupRasterIrqs();
    
    // Begin scrolling
    status |= STATUS_SCROLLING;

    for (;;) {
        unsigned char backCol = vic.spr_backcol;
        if (backCol & (SPRITE_BALLOON_OUTLINE_ENABLE | SPRITE_BALLOON_BG_ENABLE)) {
            // Check the status. This loop can go around twice and count a collision each time.
            if (status & STATUS_SCROLLING) {
                if (playerData.fuel == 0) {
                    // GAME OVER SCREEN?
                    break;
                }
                if (terrainCollisionOccurred() == 1) {
                    balloonDamage(&playerData);
                } else {
                    carriageDamage(&playerData);
                }
                showScoreBoard(&playerData);
            }
        } 
        if ((status & STATUS_AIRDROP_ON) && (backCol & SPRITE_AIRDROP_ENABLE)) {
            vic.spr_enable &= ~SPRITE_AIRDROP_ENABLE;
            status &= ~STATUS_AIRDROP_ON;
        }
        unsigned char sprColl = vic.spr_sprcol;
        if (status & STATUS_CITY_RAMP) {
            if ((sprColl & (SPRITE_RAMP_ENABLE | SPRITE_BALLOON_BG_ENABLE)) == (SPRITE_RAMP_ENABLE | SPRITE_BALLOON_BG_ENABLE)) {
                // Collision with Ramp - GOOD
                Sound_endSong();
                landingOccurred(&playerData);
                Sound_doSound(SOUND_EFFECT_PREPARE);
                Sound_startSong(SOUND_SONG_AIRBORNE);
            }
        } else if (status & STATUS_SWIRL_ON) {
            if ((sprColl & (SPRITE_SWIRL_ENABLE | SPRITE_BALLOON_BG_ENABLE)) == (SPRITE_SWIRL_ENABLE | SPRITE_BALLOON_BG_ENABLE)) {
                // Collision with Ramp - GOOD
                Sound_endSong();
                // maybe a special sound here, special song?
                Sound_doSound(SOUND_EFFECT_PORTAL_ENTRY);
                portalEntered();
                Sound_startSong(SOUND_SONG_AIRBORNE);
            }
        } else if (status & STATUS_AIRDROP_ON) {
            
        }
        if ((sprColl & (SPRITE_CITY_OUTLINE_ENABLE | SPRITE_BALLOON_BG_ENABLE)) == 
            (SPRITE_CITY_OUTLINE_ENABLE | SPRITE_BALLOON_BG_ENABLE)) {
            // Collision with City - BAD
            if (playerData.fuel == 0) {
                // GAME OVER screen
                break;
            }
            if (terrainCollisionOccurred() == 1) {
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
                    if (playerData.fuel) {
                        if (playerData.fuel >= 600) {
                            playerData.fuel -= 600;
                        } else  {
                            playerData.fuel = 0;
                        }
                        Sound_doSound(SOUND_EFFECT_THRUST_BACK);
                        invokeDecel(64);
                    }
                } else if (ch == 'W') {
                    if (playerData.fuel) {
                        if (playerData.fuel > 200) {
                            playerData.fuel -= 200;
                        } else {
                            playerData.fuel = 0;
                        }
                        Sound_doSound(SOUND_EFFECT_THRUST);
                        invokeInternalFlame(25, &playerData);
                    } 
                } else if (ch == 'X') {
                    break;
                } else if ((status & STATUS_UTILITY_SPRITES) == 0) {
                    if  (ch == 'I') {
                        if ((status & STATUS_CITY_VIS) && (cityXPos > 80)){
                            Sound_doSound(SOUND_EFFECT_EXTEND);
                            invokeRamp();
                            showScoreBoard(&playerData);
                        }
                    } else if (ch == 'P') {
                        if (isPortalNear(currMap, mapXCoord, &playerData)) {
                            // This will trigger the swirl when it's swirl time
                            status |= STATUS_SWIRL_READY;
                            Sound_doSound(SOUND_EFFECT_PORTAL_SIGNAL);
                        }
                    } else if (ch == 'M') {
                        if ((status & STATUS_UTILITY_SPRITES) == 0) {
                            invokeAirDrop();
                        }
                    }
                }
            } // throw away key presses while game is frozen
        }
        if (flip) {
            showScoreBoard(&playerData);
            checkBalloonDamage(&playerData);
            char oldCoord = mapXCoord;
            mapXCoord += 1;
            if ((playerData.balloonUpgrades & BALLOON_PORTAL) && (isPortalSignallable (currMap, mapXCoord, &playerData))) {
                Sound_doSound(SOUND_EFFECT_PORTAL_ANNOUNCE);
            }
            char currCoord = mapXCoord;
            char nextCoord = mapXCoord + 1;
            char roof[3] = { mountainHeight[(terrain[currMap][oldCoord]>>3) & 0x7],
                             mountainHeight[(terrain[currMap][currCoord]>>3) & 0x7], 
                             mountainHeight[(terrain[currMap][nextCoord]>>3) & 0x7]};
            char ground[3] = {LOWEST_SCROLLING_CHAR_ROW - mountainHeight[(terrain[currMap][oldCoord] & 0x7)],
                              LOWEST_SCROLLING_CHAR_ROW - mountainHeight[(terrain[currMap][currCoord] & 0x7)], 
                              LOWEST_SCROLLING_CHAR_ROW - mountainHeight[(terrain[currMap][nextCoord] & 0x7)]};
            if (currScreen == 0) {
                copyS0toS1(roof,ground);
            } else {
                copyS1toS0(roof,ground);
            }
            unsigned char city = terrain[currMap][currCoord]>>6;
            if (city) {
                status |= STATUS_CITY_VIS;
                cityNum = city;
                cityXPos = (unsigned int)(256+92);
                cityYPos = 202 - (8*mountainHeight[(terrain[currMap][currCoord] & 0x7)]) - 16;
            }
            if ((status & STATUS_SWIRL_READY) && ((status & STATUS_SWIRL_ON) == 0))  {
                portalNextMap = isPortalHere(currMap, mapXCoord);
                if (portalNextMap != TERRAIN_NO_PORTAL) {
                    status |= STATUS_SWIRL_ON;
                    swirlXPos = (unsigned int)(256+92);
                    swirlYPos = 120;
                }
            }
            flip = 0;
        }
    }

    clearRasterIrqs();

}

unsigned char introScreen(char *name, unsigned char* title)
{
    // sound
    Sound_initSid();
    Sound_startSong(SOUND_SONG_THEME);
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
    
    unsigned char returnValue = 0;
    for (;;) {
        unsigned char result = getMenuChoice(4, 0, VCOL_DARK_GREY, menuOptions, false, nullptr, nullptr);
        
        if (result == 0) {
            getInputText(28, 12, 10, s"your name ", name);
            char titleOptions[4][10] = {
                s"mr        ",
                s"mrs       ",
                s"ms        ",
                s"mx        "};
            *title = getMenuChoice(4,0, VCOL_DARK_GREY,titleOptions,false,nullptr, nullptr);
            returnValue = 1;
            break;
        } else if (result == 1) {
            
        } else if (result == 2) {
            
        } else {
            returnValue = 0;
            break;
        }
    }
    vic.spr_enable = 0x00;
    clearRasterIrqs();
    Sound_endSong();
    return returnValue;
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
