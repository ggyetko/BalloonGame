#include "city.h"
#include "namedPassenger.h"

unsigned char CityCode_getCityNum(CityCode cityCode){
    return cityCode.code & 0x03;
}
unsigned char CityCode_getMapNum(CityCode cityCode){
    return (cityCode.code & 0x1C) >> 2;
}
CityCode CityCode_generateCityCode(unsigned char mapNum, unsigned char cityNum) {
    CityCode retval;
    retval.code = (mapNum << 2) | cityNum;
    return retval;
}

#define NUM_PASSENGER_NAMES 20
PassengerName psgrNames[NUM_PASSENGER_NAMES] = {
    {s"wong      ",false},
    {s"wang      ",false},
    {s"gyetko    ",false},
    {s"tofinetti ",false},
    {s"campbell  ",false},
    {s"brown     ",false},  // Great Scott!
    {s"verne     ",false},  // Jules Verne
    {s"diggs     ",false},  // Wizard of OZ
    {s"wright    ",false},  // orville or wilbur
    {s"earhart   ",false},  // Amelia
    {s"bly       ",false},  // around the world in 72 days
    {s"polo      ",false},  // marco
    {s"scoresby  ",false},  // Lee, from Golden Compass
    {s"blanchard ",false},  // Sophie
    {s"corominas ",false},  // Mercedes
    {s"yeager    ",false},  // Chuck
    {s"maverick  ",false},  // Air brakes!
    {s"stark     ",false},  // Not tony, the other one
    {s"hadfield  ",false},  // Oh, I thought you said "Astronaut"
    {s"garneau   ",false}   // I thought you said "Astronaut" again
};
unsigned char psgrNameCount = NUM_PASSENGER_NAMES;

const unsigned int REPAIR_COST_BALLOON_FABRIC = 400;
const unsigned int REPAIR_COST_PSGR_CABIN     = 600;
const unsigned int REPAIR_COST_CARGO          = 850;
const unsigned int REPAIR_COST_FACILITY_REDUCTION = 150;

// constant data related to cities - NO SAVING REQUIRED
const CityData cities[CITY_NUM_MAPS][CITY_NUM_CITIES_PER_MAP] = {
    // MAP #0
    {
        // NAME         FACILITY
        {s"cloud city", CITY_FACILITY_BALLOON_FABRIC, 
            // Demands
            {{0,1},{3,1},{0xff,0},{0xff,0}},
            // For sale
            {{1,  2, CITY_RESPECT_LOW, 2},  // wheat
             {9,  2, CITY_RESPECT_LOW, 1},  // Bronze
             {11, 2, CITY_RESPECT_MED, 1}, // Iron
             {14, 2, CITY_RESPECT_HIGH,1} // Smithore
            },
          //01234567890123456789
            s"you are lost and are"
            s"looking for a way to"
            s"your home? my friend"
            s"in sirenia might be "
            s"able to help. talk  "
            s"to the mayor there. "
        },
        {s"floria    ", 0,
            {{1,1}, {7,1},{0xff,0},{0xff,0}},
            {{2, 2, CITY_RESPECT_LOW, 2},  // corn
             {3, 2, CITY_RESPECT_LOW, 2},  // spinach
             {19,2, CITY_RESPECT_MED, 2},  // eggs
             {20,2, CITY_RESPECT_HIGH,2}   // quail eggs
            },
          //01234567890123456789
            s"there are ways to   "
            s"reach other cities. "
            s"but you will need a "
            s"special device to go"
            s"beyond the mountains"
            s"that you see here.  "
        },
        {s"sirenia   ", CITY_FACILITY_PORTAL_UPGRADE,
            {{2,1},{9,1},{0xff,0},{0xff,0}},
            {{0,2,CITY_RESPECT_LOW,2}, // rice
             {7,2,CITY_RESPECT_LOW,2}, // soy beans
             {21,2,CITY_RESPECT_MED,2}, // bok choy
             {22,2,CITY_RESPECT_HIGH,2} // black beans
            },
          //01234567890123456789
            s"we make a device to "
            s"allow special travel"
            s"it may not take you "
            s"home right away and "
            s"we do not sell it to"
            s"just anyone.        "

        }
    },
    // MAP #1
    {
        // NAME         FACILITY
        {s"frieren   ", CITY_FACILITY_BALLOON_FABRIC, 
            // Demands
            {{0xff,0},{0xff,0},{0xff,0},{0xff,0}},
            // For sale
            {{0xff, 2, CITY_RESPECT_LOW, 2},
             {0xff, 2, CITY_RESPECT_LOW, 1},
             {0xff, 2, CITY_RESPECT_MED,  1},
             {0xff, 2, CITY_RESPECT_HIGH, 1}
            }
        },
        {s"granzam   ", 0,
            {{0xff,0},{0xff,0},{0xff,0},{0xff,0}},
            {{0xff, 2, CITY_RESPECT_LOW, 2},
             {0xff, 2, CITY_RESPECT_LOW, 1},
             {0xff, 2, CITY_RESPECT_MED,  1},
             {0xff, 2, CITY_RESPECT_HIGH, 1}
            } 
        },
        {s"hoth      ", 0,
            {{0xff,0},{0xff,0},{0xff,0},{0xff,0}},
            {{0xff, 2, CITY_RESPECT_LOW, 2},
             {0xff, 2, CITY_RESPECT_LOW, 1},
             {0xff, 2, CITY_RESPECT_MED,  1},
             {0xff, 2, CITY_RESPECT_HIGH, 1}
            } 
        }
    }
};
// variable city data - MUST BE SAVED with SAVE GAME
unsigned char cityRespectLevel[CITY_NUM_MAPS][CITY_NUM_CITIES_PER_MAP];

void City_initCityVariables(void)
{
    cityRespectLevel[0][0] = CITY_RESPECT_LOW;
    cityRespectLevel[0][1] = CITY_RESPECT_LOW;
    cityRespectLevel[0][2] = CITY_RESPECT_LOW;
    cityRespectLevel[1][0] = CITY_RESPECT_LOW;
    cityRespectLevel[1][1] = CITY_RESPECT_LOW;
    cityRespectLevel[1][2] = CITY_RESPECT_LOW;
}

void generateCurrentCityTmpData(Passenger *tempPsgData, CityCode currentCity) // Assume size 10 array
{
    unsigned char psgDataIndex = 0;
    unsigned char currMapNum = CityCode_getMapNum(currentCity);
    unsigned char currCityNum = CityCode_getCityNum(currentCity);
    // Check for Quest Passengers
    unsigned char specialGuest = NamedPassenger_getQuestPassenger(currentCity);
    if (specialGuest != INVALID_NAMED_PASSENGER_INDEX) {
        tenCharCopy(tempPsgData[psgDataIndex].name, namedPassengers[specialGuest].name);
        tempPsgData[psgDataIndex].fare = 0;
        tempPsgData[psgDataIndex].destination = namedPassengers[specialGuest].destinationCity;
        psgDataIndex++;  
    }
    
    // 2 low-class customers for each nearby city
    for (unsigned char city=1; city<4; city++) {
        if (city != currCityNum) {
            unsigned char nameIndex = takeRandomName();
            tenCharCopy(tempPsgData[psgDataIndex].name,psgrNames[nameIndex].name);
            tempPsgData[psgDataIndex].fare = PASSENGER_COST_PER_PASSAGE;
            tempPsgData[psgDataIndex].destination = CityCode_generateCityCode(currMapNum, city);
            psgDataIndex++;
            
            nameIndex = takeRandomName();
            tenCharCopy(tempPsgData[psgDataIndex].name,psgrNames[nameIndex].name);
            tempPsgData[psgDataIndex].fare = PASSENGER_COST_PER_PASSAGE;
            tempPsgData[psgDataIndex].destination = CityCode_generateCityCode(currMapNum, city);
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

void addRecentQuestToCityTmpData(Passenger *tempPsgData, unsigned char namedPassengerIndex)
{
    unsigned char psgDataIndex = 0;
    for (psgDataIndex = 0; psgDataIndex<MAX_PASSENGERS_AVAILABLE; psgDataIndex++) {
        if (tempPsgData[psgDataIndex].destination.code == 0) {
            break; 
        }
    }
    if (psgDataIndex < MAX_PASSENGERS_AVAILABLE) {
        tenCharCopy(tempPsgData[psgDataIndex].name, namedPassengers[namedPassengerIndex].name);
        tempPsgData[psgDataIndex].fare = 0;
        tempPsgData[psgDataIndex].destination = namedPassengers[namedPassengerIndex].destinationCity;
        if (psgDataIndex + 1 < MAX_PASSENGERS_AVAILABLE) {
            tempPsgData[psgDataIndex+1].destination.code = 0;
        }
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
        if (tenCharCmp(psgrNames[p].name, name) == 0) {
            psgrNames[p].inUse = false;
            psgrNameCount ++;
            break;
        }
    }
}

char const* getCityNameFromCityCode(CityCode cityCode)
{
    unsigned char mapNum = CityCode_getMapNum(cityCode);
    unsigned char cityNum = CityCode_getCityNum(cityCode);
    return cities[mapNum][cityNum-1].name;
}
