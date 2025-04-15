#ifndef CITY_H
#define CITY_H

#include "goods.h"
#include "utils.h"

#define CITY_NUM_MAPS           2
#define CITY_NUM_CITIES_PER_MAP 3

enum CityRespect {
    CITY_RESPECT_NONE=0,
    CITY_RESPECT_LOW,
    CITY_RESPECT_MED,  
    CITY_RESPECT_HIGH  
};

enum CityFacilities {
    CITY_FACILITY_BALLOON_FABRIC    = 0x0001,
    CITY_FACILITY_PSGR_CABIN        = 0x0002,
    CITY_FACILITY_CARGO             = 0x0004,
    CITY_FACILITY_PORTAL_UPGRADE    = 0x0008,
    CITY_FACILITY_FIREPROOF_UPGRADE = 0x0010,
    CITY_FACILITY_ICEPROOF_UPGRADE  = 0x0020,
    CITY_FACILITY_FIRSTCLASS_UPGRADE= 0x0040,
    CITY_FACILITY_GALLEY_UPGRADE    = 0x0080,
    CITY_FACILITY_AIRDROP_UPGRADE   = 0x0100,
};

extern const unsigned int REPAIR_COST_BALLOON_FABRIC = 400;
extern const unsigned int REPAIR_COST_PSGR_CABIN     = 600;
extern const unsigned int REPAIR_COST_CARGO          = 850;
extern const unsigned int REPAIR_COST_FACILITY_REDUCTION = 150;

#define MAX_BUY_GOODS 4
#define MAX_SELL_GOODS 4

#define CITY_GAMEINFO_SIZE 120

struct CityData {
    char name[10];
    unsigned int facility;
    DemandedGoods buyGoods[MAX_BUY_GOODS];
    AvailableGoods sellGoods[MAX_SELL_GOODS];
    char gameInfo[CITY_GAMEINFO_SIZE];
};

struct CityDataVar {
    unsigned char respectLevel;
    unsigned char status;
};
enum CityDataVarStatus {
    CITY_STATUS_CITY = 0,
    CITY_STATUS_MULE = 1,
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
extern const CityData cities[CITY_NUM_MAPS][CITY_NUM_CITIES_PER_MAP];
extern CityDataVar citiesVar[CITY_NUM_MAPS][CITY_NUM_CITIES_PER_MAP];

void City_initCityVariables(void);

// call this when leaving a city to return the unused psgr names to the list
void City_returnUnusedPassengers(Passenger *tempPsgData);

// call this upon landing to make up a newly generated passenger list
void City_generateCurrentCityTmpData(Passenger *tempPsgData, CityCode currentCity); // Assume size 10 array

void addRecentQuestToCityTmpData(Passenger *tempPsgData, unsigned char namedPassengerIndex);

void removePassengerFromList(Passenger *tempPsgData, unsigned char index);

unsigned int getGoodsPurchasePrice(CityData const *cityData, unsigned char goodsIndex, unsigned int normalPrice);

unsigned char takeRandomName(void);
void City_returnName(char *name);

char const* getCityNameFromCityCode(CityCode cityCode);
#endif