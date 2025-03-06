#ifndef PLAYER_DATA_H
#define PLAYER_DATA_H

#include "goods.h"
#include "utils.h"
#include "city.h"

#define FUEL_COST_QUARTER_TANK  2048  // probably don't use this
#define FUEL_COST_QUARTER_TANK  512
#define FUEL_COST_DIVISOR       32

#define MAX_PASSENGERS          8

const unsigned char MAX_CARGO_SPACE = 16;
struct Passenger {
    char name[10];
    unsigned char fare;
    CityCode destination;
};
struct Cargo {
    unsigned char psgrSpace; // reduced by damage
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
    BALLOON_AIRDROP    = 0x10
};
struct PlayerData {
    unsigned char balloonUpgrades;
    unsigned int fuel;  // fuel max out of 65535
    unsigned int money;
    unsigned char balloonHealth; // value out of 8
    Cargo cargo;
};
void playerDataInit(PlayerData *data){
    data->balloonUpgrades = 0;
    data->fuel = 20000;
    data->money = 1000;
    data->balloonHealth = 8;
    // Cargo
    data->cargo.cargoSpace = MAX_CARGO_SPACE;
    data->cargo.currCargoCount = 0;
    for (unsigned char x=0; x<MAX_CARGO_SPACE; x++) {
        data->cargo.cargo[x] = NO_GOODS;
    }
    // Passengers
    data->cargo.psgrSpace = 8;
    for (unsigned char x=0; x<MAX_PASSENGERS; x++) {
        data->cargo.psgr[x].destination.code = 0;
    }
    
}
void balloonDamage(PlayerData *data){
    if (data->balloonHealth) {
        data->balloonHealth--;
    }
}
void carriageDamage(PlayerData *data) {
    unsigned int rnd = rand() & 0x03;
    if (rnd) {  
        // 3/4 of the time, it's cargo damage
        if (data->cargo.cargoSpace) {
            unsigned char rnd = rand() % data->cargo.cargoSpace;
            for (unsigned char x=0; x<16; x++) {
                if (data->cargo.cargo[x] != 0xfe) {
                    if (rnd == 0) {
                        // this is the damaged one
                        data->cargo.cargo[x] = 0xfe;
                        break;
                    } else {
                        rnd --;
                    }
                }
            }
            data->cargo.cargoSpace--;
        } 
    } else if (data->cargo.psgrSpace) {
        data->cargo.psgrSpace--;
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
unsigned char makeShortCargoList(PlayerData const *data, unsigned char* list16)
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
                    found = true;
                    break;
                }
            }
            if (!found) {
                ScreenWork[10] = index/10 +48;
                ScreenWork[11] = index%10 +48;
                ScreenWork[13] = thisCargo/10 +48;
                ScreenWork[14] = thisCargo%10 +48;
                for (char y=index; y>-1 ;y--) {
                    if ((y==0) || (thisCargo > list16[y-1])) {
                        list16[y] = thisCargo;
                        break;
                    } else {
                        list16[y] = list16[y-1];
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
void addPassenger(PlayerData *data, Passenger *passenger)
{
    for (unsigned char x=0;x<MAX_PASSENGERS;x++) {
        if (data->cargo.psgr[x].destination.code == 0) {
            tenCharCopy(data->cargo.psgr[x].name, passenger->name);
            data->cargo.psgr[x].fare = passenger->fare;
            data->cargo.psgr[x].destination.code = passenger->destination.code;
            break;
        }
    }
}

#endif