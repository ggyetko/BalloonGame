#include "city.h"

unsigned char CityCode_getCityNum(CityCode cityCode){
    return cityCode.code & 0x03;
}
unsigned char CityCode_getMapNum(CityCode cityCode){
    return (cityCode.code & 0x1C) >> 2;
}
unsigned char CityCode_generateCityCode(unsigned char mapNum, unsigned char cityNum) {
    return (mapNum << 2) | cityNum;
}

#define NUM_PASSENGER_NAMES 20
PassengerName psgrNames[NUM_PASSENGER_NAMES] = {
    {s"wong      ",false},
    {s"wang      ",false},
    {s"gyetko    ",false},
    {s"tofinetti ",false},
    {s"campbell  ",false},
    {s"brown     ",false},
    {s"verne     ",false},
    {s"diggs     ",false},
    {s"wright    ",false},
    {s"earhart   ",false},
    {s"bly       ",false},
    {s"polo      ",false},
    {s"scoresby  ",false},
    {s"blanchard ",false},
    {s"corominas ",false},
    {s"yeager    ",false},
    {s"maverick  ",false},
    {s"stark     ",false},
    {s"hadfield  ",false},
    {s"garneau   ",false}
};
unsigned char psgrNameCount = NUM_PASSENGER_NAMES;

const CityData cities[1][3] = {
    // MAP #0
    {
        // NAME         RESPECT            BUY     SELL
        {s"cloud city", CITY_RESPECT_LOW, 
            // Demands
            {{0,1},{3,1}},
            // For sale
            {{1,  2, CITY_RESPECT_LOW, 2},  // wheat
             {9,  2, CITY_RESPECT_LOW, 1},  // Bronze
             {11, 2, CITY_RESPECT_MED, 1}, // Iron
             {14, 2, CITY_RESPECT_HIGH,1} // Smithore
            }
        },
        {s"floria    ", CITY_RESPECT_LOW, 
            {{1,1}, {7,1}},
            {{2, 2, CITY_RESPECT_LOW, 2},  // corn
             {3, 2, CITY_RESPECT_LOW, 2},  // spinach
             {19,2, CITY_RESPECT_MED, 2},  // eggs
             {20,2, CITY_RESPECT_HIGH,2}   // quail eggs
            } 
        },
        {s"sirenia   ", CITY_RESPECT_LOW, 
            {{2,1}, {9,1}},
            {{0,2,CITY_RESPECT_LOW,2}, // rice
             {7,2,CITY_RESPECT_LOW,2}, // soy beans
             {21,2,CITY_RESPECT_MED,2}, // bok choy
             {22,2,CITY_RESPECT_HIGH,2} // black beans
            } 
        }
    }
    // MAP #1
};

void generateCurrentCityTmpData(Passenger *tempPsgData, CityCode currentCity) // Assume size 10 array
{
    unsigned char psgDataIndex = 0;
    unsigned char currMapNum = CityCode_getMapNum(currentCity);
    unsigned char currCityNum = CityCode_getCityNum(currentCity);
    // 2 low-class customers for each nearby city
    for (unsigned char city=1; city<4; city++) {
        if (city != currCityNum) {
            unsigned char nameIndex = takeRandomName();
            tenCharCopy(tempPsgData[psgDataIndex].name,psgrNames[nameIndex].name);
            tempPsgData[psgDataIndex].fare = PASSENGER_COST_PER_PASSAGE;
            tempPsgData[psgDataIndex].destination.code = CityCode_generateCityCode(currMapNum, city);
            psgDataIndex++;
            
            nameIndex = takeRandomName();
            tenCharCopy(tempPsgData[psgDataIndex].name,psgrNames[nameIndex].name);
            tempPsgData[psgDataIndex].fare = PASSENGER_COST_PER_PASSAGE;
            tempPsgData[psgDataIndex].destination.code = CityCode_generateCityCode(currMapNum, city);
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

unsigned char takeRandomName(void)
{
    unsigned char preIndex = rand() % psgrNameCount;
    for (unsigned char index = 0; index<NUM_PASSENGER_NAMES; index++) {
        if (!psgrNames[index].inUse) {
            if (preIndex == 0) {
                psgrNames[index].inUse = true;
                psgrNameCount--;
                return index;
            }
            preIndex--;
        }
    }
    return 0;
}

void returnName(char *name)
{
    for (unsigned char p=0; p<NUM_PASSENGER_NAMES; p++) {
        bool diff = false;
        for (unsigned char ch=0; ch<10; ch++) {
            if (psgrNames[p].name[ch] !=  name[ch]) {
                diff = true;
                break;
            }
        }
        if (!diff) {
            psgrNames[p].inUse = false;
            psgrNameCount ++;
            break;
        }
    }
}
