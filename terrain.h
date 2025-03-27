#ifndef TERRAIN_H
#define TERRAIN_H

#define TERRAIN_NO_PORTAL 0xff

extern const char terrain[2][256];

extern bool isPortalSignallable(unsigned char mapNum, unsigned char scrollPos);
extern bool isPortalNear(unsigned char mapNum, unsigned char scrollPos);
// returns a map number
// returns TERRAIN_NO_PORTAL for nothing here
extern unsigned char isPortalHere(unsigned char mapNum, unsigned char scrollPos);

#endif