#ifndef PLAYER_DATA_H
#define PLAYER_DATA_H

#include "goods.h"

#define FUEL_COST_QUARTER_TANK  2048  // probably don't use this
#define FUEL_COST_QUARTER_TANK  512
#define FUEL_COST_DIVISOR       32

const unsigned char MAX_CARGO_SPACE = 16;
struct Passenger {
    char name[10];
    unsigned char fare;
};
struct Cargo {
    unsigned char psgrSpace; // reduced by damage
    Passenger psgr[8];
    unsigned char cargoSpace; // reduced by damage, max 16
    unsigned char currCargoCount; 
    int cargo[MAX_CARGO_SPACE];
};
struct PlayerData {
    unsigned int fuel;  // fuel max out of 65535
    unsigned int money;
    unsigned char balloonHealth; // value out of 8
    Cargo cargo;
};
void playerDataInit(PlayerData *data){
    data->fuel = 20000;
    data->money = 1000;
    data->balloonHealth = 8;
    data->cargo.cargoSpace = MAX_CARGO_SPACE;
    data->cargo.currCargoCount = 0;
    data->cargo.psgrSpace = 8;
    for (unsigned char x=0; x<MAX_CARGO_SPACE; x++) {
        data->cargo.cargo[x] = NO_GOODS;
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
            for (unsigned y=0;y<index;y++) {
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
                list16[index] = thisCargo;
                index++;
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

#endif