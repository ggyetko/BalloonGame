#ifndef CITY_H
#define CITY_H

#include "goods.h"

enum CityRespect {
    CITY_RESPECT_NONE=0,
    CITY_RESPECT_LOW,
    CITY_RESPECT_MED,  
    CITY_RESPECT_HIGH  
};

const unsigned char MAX_BUY_GOODS = 4;
const unsigned char MAX_SELL_GOODS = 4;

struct CityCode {
    unsigned char code; // ...MMM##, MMM map number, ## city within map (1,2,3) 0 is illegal
};
unsigned char CityCode_getCityNum(CityCode *cityCode){
    return *cityCode & 0x03;
}
unsigned char CityCode_getMapNum(CityCode *cityCode){
    return (*cityCode & 0x1C) >> 2;
}
struct CityCode CityCode_generateCityCode(unsigned char mapNum, unsigned char cityNum) {
    return (mapNum << 2) | cityNum;
}

struct CityData {
    unsigned char name[10];
    unsigned char respect;
    DemandedGoods buyGoods[MAX_BUY_GOODS];
    AvailableGoods sellGoods[MAX_SELL_GOODS];
};
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