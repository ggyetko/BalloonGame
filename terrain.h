#ifndef TERRAIN_H
#define TERRAIN_H

#define TERRAIN_NO_PORTAL 0xff

#define NUM_TERRAINS 2

struct Palette {
    unsigned char skyColor;
    unsigned char mountainColor;
    unsigned char cityColor;
    unsigned char rampColor;
};

extern const Palette palette[NUM_TERRAINS];

extern const char terrain[NUM_TERRAINS][256];

extern bool isPortalSignallable(unsigned char mapNum, unsigned char scrollPos);
extern bool isPortalNear(unsigned char mapNum, unsigned char scrollPos);
// returns a map number
// returns TERRAIN_NO_PORTAL for nothing here
extern unsigned char isPortalHere(unsigned char mapNum, unsigned char scrollPos);

#endif