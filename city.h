#ifndef CITY_H
#define CITY_H

#include "goods.h"
#include "utils.h"

enum CityRespect {
    CITY_RESPECT_NONE=0,
    CITY_RESPECT_LOW,
    CITY_RESPECT_MED,  
    CITY_RESPECT_HIGH  
};

#define MAX_BUY_GOODS 4
#define MAX_SELL_GOODS 4

struct CityData {
    unsigned char name[10];
    unsigned char respect;
    DemandedGoods buyGoods[MAX_BUY_GOODS];
    AvailableGoods sellGoods[MAX_SELL_GOODS];
};

struct CityCode {
    unsigned char code; // ...MMM##, MMM map number, ## city within map (1,2,3) 0 is illegal
};
unsigned char CityCode_getCityNum(CityCode cityCode);
unsigned char CityCode_getMapNum(CityCode cityCode);
unsigned char CityCode_generateCityCode(unsigned char mapNum, unsigned char cityNum);

struct PassengerName {
    char name[10];
    bool inUse;
};

struct Passenger {
    char name[10];
    unsigned char fare;
    CityCode destination;
};

#define MAX_PASSENGERS_AVAILABLE     10
#define PASSENGER_COST_PER_PASSAGE   50
#define PASSENGER_COST_PER_MAP       100
#define PASSENGER_COST_GALLEY_BONUS  50   
#define PASSENGER_COST_1ST_CLS_BONUS 100

PassengerName psgrNames[20];
unsigned char psgrNameCount;
const CityData cities[1][3];

// call this upon landing to make up a newly generated passenger list
void generateCurrentCityTmpData(Passenger *tempPsgData, CityCode currentCity); // Assume size 10 array

void removePassengerFromList(Passenger *tempPsgData, unsigned char index);

unsigned int getGoodsPurchasePrice(CityData const *cityData, unsigned char goodsIndex, unsigned int normalPrice);

unsigned char takeRandomName(void);
void returnName(char *name);
#endif