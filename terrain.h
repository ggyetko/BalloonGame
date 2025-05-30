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
struct WorldTypeDamages {
    unsigned char coldDamage;       // damage done to balloon. Ramps up to 200 before 1 hp is lost
    unsigned char heatPowerFactor;  // how many bits to the right you shift upward thrust power
};
extern const WorldTypeDamages worldTypeDamages[5];
enum WorldTypes {
    WORLD_TYPE_NORMAL = 0,
    WORLD_TYPE_COOL,
    WORLD_TYPE_COLD,
    WORLD_TYPE_WARM,
    WORLD_TYPE_HOT,
};

extern const char terrainMapNames[NUM_TERRAINS][10];

extern const Palette palette[NUM_TERRAINS];

extern const byte terrain[NUM_TERRAINS][256];

extern const byte terrainEnvironment[NUM_TERRAINS];

extern bool isPortalSignallable(unsigned char mapNum, unsigned char scrollPos, PlayerData *data);
extern bool isPortalNear(unsigned char mapNum, unsigned char scrollPos, PlayerData *data);
// returns a map number
// returns TERRAIN_NO_PORTAL for nothing here
extern unsigned char isPortalHere(unsigned char mapNum, unsigned char scrollPos);

#endif