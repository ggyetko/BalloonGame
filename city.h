#include "goods.h"

enum CityRespect {
    CITY_RESPECT_NONE=0,
    CITY_RESPECT_LOW,
    CITY_RESPECT_MED,  
    CITY_RESPECT_HIGH  
};

const unsigned char MAX_BUY_GOODS = 1;
const unsigned char MAX_SELL_GOODS = 4;

struct CityData {
    unsigned char name[10];
    unsigned char respect;
    DemandedGoods buyGoods;
    AvailableGoods sellGoods[4];
};
