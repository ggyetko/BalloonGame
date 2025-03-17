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

enum CityFacilities {
    CITY_FACILITY_BALLOON_FABRIC = 0x01,
    CITY_FACILITY_PSGR_CABIN     = 0x02,
    CITY_FACILITY_CARGO          = 0x04
};

extern const unsigned int REPAIR_COST_BALLOON_FABRIC = 400;
extern const unsigned int REPAIR_COST_PSGR_CABIN     = 600;
extern const unsigned int REPAIR_COST_CARGO          = 850;
extern const unsigned int REPAIR_COST_FACILITY_REDUCTION = 150;

#define MAX_BUY_GOODS 4
#define MAX_SELL_GOODS 4

struct CityData {
    unsigned char name[10];
    unsigned char facility;
    DemandedGoods buyGoods[MAX_BUY_GOODS];
    AvailableGoods sellGoods[MAX_SELL_GOODS];
};

struct CityCode {
    unsigned char code; // ...MMM##, MMM map number, ## city within map (1,2,3) 0 is illegal
};
unsigned char CityCode_getCityNum(CityCode cityCode);
unsigned char CityCode_getMapNum(CityCode cityCode);
CityCode CityCode_generateCityCode(unsigned char mapNum, unsigned char cityNum);
#define DESTINATION_CODE_NO_PASSENGER     0
#define DESTINATION_CODE_DAMAGED_CABIN    0xff

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

extern PassengerName psgrNames[20];
extern unsigned char psgrNameCount;
extern const CityData cities[2][3];
extern unsigned char cityRespectLevel[2][3];

// call this upon landing to make up a newly generated passenger list
void generateCurrentCityTmpData(Passenger *tempPsgData, CityCode currentCity); // Assume size 10 array

void addRecentQuestToCityTmpData(Passenger *tempPsgData, unsigned char namedPassengerIndex);

void removePassengerFromList(Passenger *tempPsgData, unsigned char index);

unsigned int getGoodsPurchasePrice(CityData const *cityData, unsigned char goodsIndex, unsigned int normalPrice);

unsigned char takeRandomName(void);
void returnName(char *name);

char const* getCityNameFromCityCode(CityCode cityCode);
#endif