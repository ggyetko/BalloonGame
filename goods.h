#ifndef GOODS_H
#define GOODS_H

struct Goods {
    char name[10];
    unsigned int normalCost;  // everyone charges this amount
    unsigned char variance;    // make it look kinda real with a bit of random wobble
    unsigned char dummy1;
    unsigned char dummy2;
    unsigned char dummy3;
};

struct DemandedGoods {
    unsigned char goodsIndex;
    unsigned char priceAdjustment; // number of 25% to add (e.g. 1=1.25, 2=1.50, 4=2.00, 8=3.00)
};
enum DemandGoodsMultiplier {
    DEMAND_PLUS_10PCT = 1,
    DEMAND_PLUS_20PCT,
    DEMAND_PLUS_30PCT,
    DEMAND_PLUS_40PCT,
    DEMAND_PLUS_50PCT,
    DEMAND_PLUS_60PCT,
    DEMAND_PLUS_70PCT,
    DEMAND_PLUS_80PCT,
    DEMAND_PLUS_90PCT,
    DEMAND_PLUS_100PCT,
    DEMAND_PLUS_200PCT = 20
};
#define NO_GOODS 255
#define DAMAGED_SLOT 254

struct AvailableGoods {
    unsigned char goodsIndex;
    unsigned char priceAdjustment; // divisor
    unsigned char reqRespectRate;  // How well respected are you determines if this is available
    unsigned char productionRate;  // How many do they produce every round of the map?
};

#endif