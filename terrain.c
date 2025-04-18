#include <c64/vic.h>

#include "terrain.h"
#include "quest.h"
#include "playerData.h"

const Palette palette[NUM_TERRAINS] = {
    {VCOL_LT_BLUE, VCOL_DARK_GREY, VCOL_WHITE, VCOL_BROWN, VCOL_DARK_GREY},
    {VCOL_BLUE, VCOL_WHITE, VCOL_LT_GREY, VCOL_ORANGE, VCOL_LT_GREY},
};

extern const WorldType[5] = {
    {0,0},  // Normal World
    {16,0}, // cool world
    {64,0}, // ice world
    {0,16}, // desert world
    {0,64}, // volcanic world
};

// TBD - 8 should be num terrains later
const char terrainMapNames[8][10] = {   
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
    {
        013,013,022,022,031,021,021,022, 012,013,023,032,041,031,021,022,
        012,022,023,032,032,031,021,022, 012,022,023,012,012,021,031,022,
        013,013,022,022,031,021,021,022, 012,013,023,032,041,031,021,022,
        012,022,023,032,032,031,021,022, 012,022,0123,013,013,022,032,022, // City #1
        013,013,022,022,031,021,021,022, 012,013,023,032,041,031,021,022,
        012,022,023,032,032,031,021,022, 012,022,023,012,012,021,031,022,
        013,013,022,022,031,021,021,022, 012,013,023,032,041,031,021,022,
        012,022,023,032,032,031,021,022, 012,022,023,012,012,021,031,022,
        013,013,022,022,031,021,021,022, 012,013,023,032,041,031,021,022, 
        012,022,021,031,032,031,022,0223, 013,023,023,014,013,022,032,022, // City #2
        013,013,022,022,031,021,021,022, 012,013,023,032,041,031,021,022,
        012,022,023,032,032,031,021,022, 012,022,023,012,012,021,031,022,
        013,013,022,022,031,021,021,022, 012,013,023,032,041,031,021,022,
        011,022,022,031,032,031,021,022, 012,022,021,011,011,021,031,021,
        011,011,021,0324,034,024,024,023, 012,012,023,032,041,031,021,022, // City #3
        012,022,023,032,032,031,021,022, 012,022,023,012,012,021,031,022,
    },
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
    }
};

#define SCROLL_PORTAL_WARNING_DISTANCE 20
struct PortalCoord {
    unsigned char scrollPos;
    unsigned char mapDest;
};

// Not allowed to have a portal at 255
PortalCoord portalCoord[NUM_TERRAINS][3] = {
    {{50,1},{TERRAIN_NO_PORTAL,0},{TERRAIN_NO_PORTAL,0}},
    {{200,0},{TERRAIN_NO_PORTAL,0},{TERRAIN_NO_PORTAL,0}},
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