#ifndef TERRAIN_H
#define TERRAIN_H

#include "playerData.h"

#define TERRAIN_NO_PORTAL 0xff

#define NUM_TERRAINS 8

struct Palette {
    unsigned char skyColor;
    unsigned char mountainColor;
    unsigned char cityColor;
    unsigned char rampColor;
    unsigned char inactiveTextColor;
};
struct WorldType {
    unsigned char coldDamage;
    unsigned char heatDamage;
};

extern const WorldType[5];

extern const char terrainMapNames[NUM_TERRAINS][10];

extern const Palette palette[NUM_TERRAINS];

extern const char terrain[NUM_TERRAINS][256];

extern bool isPortalSignallable(unsigned char mapNum, unsigned char scrollPos, PlayerData *data);
extern bool isPortalNear(unsigned char mapNum, unsigned char scrollPos, PlayerData *data);
// returns a map number
// returns TERRAIN_NO_PORTAL for nothing here
extern unsigned char isPortalHere(unsigned char mapNum, unsigned char scrollPos);

#endif