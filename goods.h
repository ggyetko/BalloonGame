struct Goods {
    char name[10];
    unsigned char normalCost;  // everyone charges this amount
    unsigned char variance;    // make it look kinda real with a bit of random wobble
    unsigned char dummy1;
    unsigned char dummy2;
};

struct DemandedGoods {
    unsigned char goodsIndex;
    unsigned char priceAdjustment; // multiple this x10 = %ge increase (e.g 1 means 10%, 10 means 100%)
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

struct AvailableGoods {
    unsigned char goodsIndex;
    unsigned char priceAdjustment; // 1 - half price, 2- quarter price, 3 - eight price
    unsigned char reqRespectRate;  // How well respected are you determines if this is available
    unsigned char productionRate;  // How many do they produce every round of the map?
};
enum AvailableGoodsDivisor {    
    AVAILABLE_HALF_PRICE = 1,
    AVAILABLE_QUARTER_PRICE,
    AVAILABLE_EIGHTH_PRICE
};
