#ifndef CITY_H
#define CITY_H

#include "goods.h"
#include "playerData.h"
#include "utils.h"

enum CityRespect {
    CITY_RESPECT_NONE=0,
    CITY_RESPECT_LOW,
    CITY_RESPECT_MED,  
    CITY_RESPECT_HIGH  
};

const unsigned char MAX_BUY_GOODS = 4;
const unsigned char MAX_SELL_GOODS = 4;

struct CityData {
    unsigned char name[10];
    unsigned char respect;
    DemandedGoods buyGoods[MAX_BUY_GOODS];
    AvailableGoods sellGoods[MAX_SELL_GOODS];
};

#define MAX_PASSENGERS_AVAILABLE     10
#define PASSENGER_COST_PER_PASSAGE   50
#define PASSENGER_COST_PER_MAP       100
#define PASSENGER_COST_GALLEY_BONUS  50   
#define PASSENGER_COST_1ST_CLS_BONUS 100  
//struct CityTempPassengerData {
//    char name[10];
//    unsigned char fare;
//    CityCode destination; // #..FMMM## , FIRST CLASS, MAP NUMBER, CITY NUMBER
//};
// call this upon landing to make up a newly generated passenger list
void generateCurrentCityTmpData(Passenger *tempPsgData, CityCode currentCity) // Assume size 10 array
{
    unsigned char psgDataIndex = 0;
    unsigned char currMapNum = CityCode_getMapNum(currentCity);
    unsigned char currCityNum = CityCode_getCityNum(currentCity);
    // 2 low-class customers for each nearby city
    for (unsigned char city=1; city<4; city++) {
        if (city != currCityNum) {
            tenCharCopy(tempPsgData[psgDataIndex].name,s"alice     ");
            tempPsgData[psgDataIndex].fare = PASSENGER_COST_PER_PASSAGE;
            tempPsgData[psgDataIndex].destination.code = CityCode_generateCityCode(currMapNum, currCityNum);
            psgDataIndex++;
            tenCharCopy(tempPsgData[psgDataIndex].name,s"bob       ");
            tempPsgData[psgDataIndex].fare = PASSENGER_COST_PER_PASSAGE;
            tempPsgData[psgDataIndex].destination.code = CityCode_generateCityCode(currMapNum, currCityNum);
            psgDataIndex++;
        }
    }
    
    // if reputation is medium, 1 or 2 first class customers for nearby cities
    // or if reputation is high, 2 first class customers for each nearby city
    
    // 2 low-class customers for farther away cities
    
    // if reputation is high, 2 first class customers for far away cities
    
    // remainder of passenger slots forced to empty
    for (unsigned char p = psgDataIndex; p<MAX_PASSENGERS_AVAILABLE; p++) {
        tempPsgData[psgDataIndex].destination.code = 0; // This is an invalid code
    }
}

void removePassengerFromList(Passenger *tempPsgData, unsigned char index)
{
    for (unsigned char p = index; p<MAX_PASSENGERS_AVAILABLE-1; p++) {
        tenCharCopy(tempPsgData[p].name,tempPsgData[p+1].name);
        tempPsgData[p].fare = tempPsgData[p+1].fare;
        tempPsgData[p].destination.code = tempPsgData[p+1].destination.code;
    }
    tempPsgData[MAX_PASSENGERS_AVAILABLE-1].destination.code = 0; // invalidate last slot
}

unsigned int getGoodsPurchasePrice(CityData const *cityData, unsigned char goodsIndex, unsigned int normalPrice) {
    unsigned char x;
    bool found = false;
    for (x=0;x<MAX_BUY_GOODS;x++) {
        if (cityData->buyGoods[x].goodsIndex == goodsIndex) {
            found = true;
            break;
        }
    }
    if (!found) {
        for (x=0;x<MAX_SELL_GOODS;x++) {
            if (cityData->sellGoods[x].goodsIndex == goodsIndex) {
                // This city sells this, they won't buy it
                return 1;
            }
        }
        return normalPrice;
    }
    return normalPrice + (normalPrice / 4 * cityData->buyGoods[x].priceAdjustment);
}

#endif