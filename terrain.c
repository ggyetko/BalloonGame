#include <c64/vic.h>

#include "terrain.h"
#include "quest.h"
#include "playerData.h"

const Palette palette[NUM_TERRAINS] = {
    {VCOL_LT_BLUE, VCOL_DARK_GREY, VCOL_WHITE, VCOL_BROWN, VCOL_DARK_GREY},
    {VCOL_BLUE, VCOL_WHITE, VCOL_LT_GREY, VCOL_ORANGE, VCOL_LT_GREY},
    {VCOL_ORANGE, VCOL_BROWN, VCOL_WHITE, VCOL_DARK_GREY, VCOL_LT_GREY},
    {VCOL_GREEN, VCOL_LT_GREEN, VCOL_LT_GREY, VCOL_ORANGE, VCOL_DARK_GREY},
    {VCOL_BLUE, VCOL_WHITE, VCOL_ORANGE, VCOL_LT_GREY, VCOL_LT_GREY},
    {VCOL_LT_RED, VCOL_RED, VCOL_BLACK, VCOL_BLACK, VCOL_DARK_GREY},
    {VCOL_BLACK, VCOL_DARK_GREY, VCOL_LT_GREY, VCOL_WHITE, VCOL_LT_GREY},
    {VCOL_LT_BLUE, VCOL_GREEN, VCOL_WHITE, VCOL_WHITE, VCOL_DARK_GREY},
};

const WorldType[5] = {
    {0,0},  // Normal World
    {16,0}, // cool world
    {64,0}, // ice world
    {0,16}, // desert world
    {0,64}, // volcanic world
};

const char terrainMapNames[NUM_TERRAINS][10] = {   
    s"cave world",
    s"icelands  ",
    s"arid plain",
    s"jungle    ",
    s"frigidia  ",
    s"vulcania  ",
    s"darks end ",
    s"lands end ",
};

const char terrain[NUM_TERRAINS][256] ={
    #include "terrainFile.c"
};

#define SCROLL_PORTAL_WARNING_DISTANCE 20
struct PortalCoord {
    unsigned char scrollPos;
    unsigned char mapDest;
};

// Not allowed to have a portal at 255
PortalCoord portalCoord[NUM_TERRAINS][3] = {
    {{50,1},{TERRAIN_NO_PORTAL,0},{180,7}},
    {{90,0},{180,2},{TERRAIN_NO_PORTAL,0}},
    {{85,1},{160,3},{TERRAIN_NO_PORTAL,0}},
    {{80,2},{160,4},{TERRAIN_NO_PORTAL,0}},
    {{100,3},{210,5},{TERRAIN_NO_PORTAL,0}},
    {{70,4},{140,6},{TERRAIN_NO_PORTAL,0}},
    {{80,5},{250,7},{TERRAIN_NO_PORTAL,0}},
    {{100,6},{190,2},{TERRAIN_NO_PORTAL,0}},
};

bool isPortalSignallable(unsigned char mapNum, unsigned char scrollPos, PlayerData *data)
{
    for (unsigned char co = 0; co < 3; co++) {
        if (portalCoord[mapNum][co].scrollPos == TERRAIN_NO_PORTAL) { 
            return false;
        }
        //if ((scrollPos == 50) && (mapNum == 0)) {
        //    debugChar(0, portalCoord[mapNum][co].mapDest);
        //    debugChar(1, data->knownMaps);
        //}
        if (isMapAccessible(data, portalCoord[mapNum][co].mapDest) == false) {
            continue;
        } 
        if (portalCoord[mapNum][co].scrollPos - scrollPos == SCROLL_PORTAL_WARNING_DISTANCE) {
            return true;
        }
    }
    return false;
}

bool isPortalNear(unsigned char mapNum, unsigned char scrollPos, PlayerData *data)
{
    for (unsigned char co = 0; co < 3; co++) {
        if (portalCoord[mapNum][co].scrollPos == TERRAIN_NO_PORTAL) { 
            return false;
        }
        if (isMapAccessible(data, portalCoord[mapNum][co].mapDest) == false) {
            continue;
        } 
        if (portalCoord[mapNum][co].scrollPos - scrollPos <= SCROLL_PORTAL_WARNING_DISTANCE) {
            return true;
        }
    }
    return false;
}

unsigned char isPortalHere(unsigned char mapNum, unsigned char scrollPos)
{
    for (unsigned char co = 0; co < 3; co++) {
        if (portalCoord[mapNum][co].scrollPos == TERRAIN_NO_PORTAL) { 
            return TERRAIN_NO_PORTAL;
        }
        if (scrollPos == portalCoord[mapNum][co].scrollPos) {
            return portalCoord[mapNum][co].mapDest;
        }
    }
    return TERRAIN_NO_PORTAL;
}