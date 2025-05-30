#ifndef PLAYER_DATA_H
#define PLAYER_DATA_H

#include <c64/types.h>

#include "city.h"

#define MAX_PASSENGERS          8
#define MAX_BALLOON_HEALTH      8
#define MAX_CARGO_SPACE         16
#define CARGO_SLOT_NOT_SELECTED 0xff

#define FUEL_COST_QUARTER_TANK  512
#define FUEL_COST_DIVISOR       32

extern const char PlayerDataTitles[4][3];

struct Cargo {
    Passenger psgr[MAX_PASSENGERS];
    unsigned char cargoSpace; // reduced by damage, max 16
    unsigned char currCargoCount; 
    unsigned char airDropCargoIndex;
    int cargo[MAX_CARGO_SPACE];
};

struct PlayerData {
    char          name[10];
    byte title;
    byte balloonUpgrades;
    unsigned int  fuel;  // fuel max out of 65535
    unsigned int  money;
    byte balloonHealth; // value out of 8
    byte knownMaps; // bitmap of maps
    byte coldDamage;
    byte heatDamage;
    Cargo cargo;
};

void playerDataInit(PlayerData *data, char* tempName, unsigned char tempTitle);

bool isMapAccessible(PlayerData const *data, unsigned char mapIndex);

void addMapAccessible(PlayerData *data, unsigned char mapIndex);

void balloonDamage(PlayerData *data);

unsigned char getFunctionalCabinCount(PlayerData *data);

void carriageDamage(PlayerData *data);

bool addCargoIfPossible(PlayerData *data, unsigned char cargoIndex) ;

unsigned char makeShortCargoList(PlayerData const *data, unsigned char* list16, unsigned char* count16);

void removeCargo(PlayerData *data, unsigned char cargoIndex);

bool addPassenger(PlayerData *data, Passenger *passenger);

void removePassenger(PlayerData *data, unsigned char index);

#endif