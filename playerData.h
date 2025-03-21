#ifndef PLAYER_DATA_H
#define PLAYER_DATA_H

#include "goods.h"
#include "utils.h"
#include "city.h"
#include "stdlib.h"


#define FUEL_COST_QUARTER_TANK  2048  // probably don't use this
#define FUEL_COST_QUARTER_TANK  512
#define FUEL_COST_DIVISOR       32

#define MAX_PASSENGERS          8
#define MAX_BALLOON_HEALTH      8

const unsigned char MAX_CARGO_SPACE = 16;

const char PlayerDataTitles[4][3] = {s"mr ",s"mrs",s"ms ",s"mx "};

struct Cargo {
    Passenger psgr[MAX_PASSENGERS];
    unsigned char cargoSpace; // reduced by damage, max 16
    unsigned char currCargoCount; 
    int cargo[MAX_CARGO_SPACE];
};
enum BALLOON_UPGRADES {
    BALLOON_FIREPROOF  = 0x01,
    BALLOON_ICEPROOF   = 0x02,
    BALLOON_FIRSTCLASS = 0x04,
    BALLOON_GALLEY     = 0x08,
    BALLOON_AIRDROP    = 0x10,
    BALLOON_PORTAL     = 0x20,
};
struct PlayerData {
    char          name[10];
    unsigned char title;
    unsigned char balloonUpgrades;
    unsigned int  fuel;  // fuel max out of 65535
    unsigned int  money;
    unsigned char balloonHealth; // value out of 8
    unsigned char knownMaps; // bitmap of maps
    Cargo cargo;
};
void playerDataInit(PlayerData *data, char* tempName, unsigned char tempTitle){
    tenCharCopy(data->name, tempName);
    data->title = tempTitle;
    data->balloonUpgrades = 0;
    data->fuel = 20000;
    data->money = 1000;
    data->balloonHealth = MAX_BALLOON_HEALTH;
    // Cargo
    data->cargo.cargoSpace = MAX_CARGO_SPACE;
    data->cargo.currCargoCount = 0;
    for (unsigned char x=0; x<MAX_CARGO_SPACE; x++) {
        data->cargo.cargo[x] = NO_GOODS;
    }
    // Passengers
    for (unsigned char x=0; x<MAX_PASSENGERS; x++) {
        data->cargo.psgr[x].destination.code = DESTINATION_CODE_NO_PASSENGER;
    }
    // known maps
    data->knownMaps = 0x01; // only the primary map is accessible
}

bool isMapAccessible(PlayerData const *data, unsigned char mapIndex)
{
    return ((1<<mapIndex) & data->knownMaps) ? true : false;
}

void addMapAccessible(PlayerData *data, unsigned char mapIndex)
{
    data->knownMaps |= 1 << mapIndex;
}

void balloonDamage(PlayerData *data){
    if (data->balloonHealth) {
        data->balloonHealth--;
    }
}

unsigned char getFunctionalCabinCount(PlayerData *data) {
    unsigned char total = 0;
    for (unsigned char x = 0; x<MAX_PASSENGERS; x++) {
        if (data->cargo.psgr[x].destination.code != DESTINATION_CODE_DAMAGED_CABIN) {
            total ++;
        }
    }
    return total;
}

void carriageDamage(PlayerData *data) {
    unsigned int rnd = 0; //rand() & 0x03;
    if (rnd) {  
        // 3/4 of the time, it's cargo damage
        if (data->cargo.cargoSpace) {
            unsigned char rnd = rand() % data->cargo.cargoSpace;
            for (unsigned char x=0; x<MAX_CARGO_SPACE; x++) {
                if (data->cargo.cargo[x] != DAMAGED_SLOT) {
                    if (rnd == 0) {
                        // this is the damaged one, cargo might be lost here
                        data->cargo.cargo[x] = DAMAGED_SLOT;
                        break;
                    } else {
                        rnd --;
                    }
                }
            }
            data->cargo.cargoSpace--;
        } 
    }
    else {
        unsigned char funcCabins = getFunctionalCabinCount(data);
        if (funcCabins) {
            unsigned char rnd = rand() % funcCabins;
            for (unsigned char x=0; x<MAX_PASSENGERS; x++) {
                if (data->cargo.psgr[x].destination.code != DESTINATION_CODE_DAMAGED_CABIN) {
                    if (rnd == 0) {
                        if (data->cargo.psgr[x].destination.code != DESTINATION_CODE_NO_PASSENGER) {
                            returnName(data->cargo.psgr[x].name);
                        }
                        data->cargo.psgr[x].destination.code = DESTINATION_CODE_DAMAGED_CABIN;
                        break;
                    } else {
                        rnd --;
                    }
                }
            }
        }
    }
}
bool addCargoIfPossible(PlayerData *data, unsigned char cargoIndex) {
    if (data->cargo.currCargoCount < data->cargo.cargoSpace) {
        for (unsigned char x=0; x<MAX_CARGO_SPACE; x++) {
            if (data->cargo.cargo[x] == NO_GOODS) {
                data->cargo.cargo[x] = cargoIndex;
                data->cargo.currCargoCount++;
                return true;
            } 
        }
    }
    return false;
}
unsigned char makeShortCargoList(PlayerData const *data, unsigned char* list16, unsigned char* count16)
{
    unsigned char x, index;
    index = 0;
    for (x=0;x<MAX_CARGO_SPACE;x++) {
        unsigned char thisCargo = data->cargo.cargo[x];
        // Ignore empty cargo spots
        if (thisCargo < DAMAGED_SLOT) {
            bool found = false;
            for (unsigned char y=0;y<index;y++) {
                if (list16[y] == thisCargo) {
                    count16[y] += 1;
                    found = true;
                    break;
                }
            }
            if (!found) {
                for (char y=index; y>-1 ;y--) {
                    if ((y==0) || (thisCargo > list16[y-1])) {
                        list16[y] = thisCargo;
                        count16[y] = 1;
                        break;
                    } else {
                        list16[y] = list16[y-1];
                        count16[y] =count16[y-1];
                    }
                }
                index ++;
            }
        }
    }
    return index;
}
void removeCargo(PlayerData *data, unsigned char cargoIndex) 
{
    unsigned char x;
    for (x=0;x<MAX_CARGO_SPACE;x++) {
        if (data->cargo.cargo[x] == cargoIndex) {
            data->cargo.cargo[x] = NO_GOODS;
            data->cargo.currCargoCount -= 1;
            break;
        }
    }
}
bool addPassenger(PlayerData *data, Passenger *passenger)
{
    for (unsigned char x=0;x<MAX_PASSENGERS;x++) {
        if (data->cargo.psgr[x].destination.code == DESTINATION_CODE_NO_PASSENGER) {
            tenCharCopy(data->cargo.psgr[x].name, passenger->name);
            data->cargo.psgr[x].fare = passenger->fare;
            data->cargo.psgr[x].destination.code = passenger->destination.code;
            return true;
        }
    }
    return false;
}

void removePassenger(PlayerData *data, unsigned char index)
{
    data->cargo.psgr[index].destination.code = DESTINATION_CODE_NO_PASSENGER;
}

#endif