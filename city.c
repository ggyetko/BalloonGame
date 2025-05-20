#include "city.h"
#include "namedPassenger.h"
#include "namedGoods.h"

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
const CityData cities[CITY_MAPS_COUNT][CITY_NUM_CITIES_PER_MAP] = {
    // MAP #0
    {
        // NAME         FACILITY
        {s"cloud city", CITY_FACILITY_BALLOON_FABRIC, 
            // Demands
            {{GOODS_RICE,1},{GOODS_SPINACH,1},{0xff,0},{0xff,0}},
            // For sale
            {{GOODS_WHEAT,  2, CITY_RESPECT_LOW, 2},  // wheat
             {GOODS_BRONZE,  2, CITY_RESPECT_LOW, 1},  // Bronze
             {GOODS_IRON, 2, CITY_RESPECT_MED, 1}, // Iron
             {GOODS_SMITHORE, 2, CITY_RESPECT_HIGH,1} // Smithore
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
            {{GOODS_WHEAT,1}, {GOODS_SOYBEANS,1},{0xff,0},{0xff,0}},
            {{GOODS_CORN, 2, CITY_RESPECT_LOW, 2},  // corn
             {GOODS_SPINACH, 2, CITY_RESPECT_LOW, 2},  // spinach
             {GOODS_EGGS,2, CITY_RESPECT_MED, 2},  // eggs
             {GOODS_QUAIL_EGGS,2, CITY_RESPECT_HIGH,2}   // quail eggs
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
            {{GOODS_CORN,1},{GOODS_BRONZE,1},{0xff,0},{0xff,0}},
            {{GOODS_RICE,2,CITY_RESPECT_LOW,2}, // rice
             {GOODS_SOYBEANS,2,CITY_RESPECT_LOW,2}, // soy beans
             {GOODS_BOK_CHOY,2,CITY_RESPECT_MED,2}, // bok choy
             {GOODS_BLACK_BEANS,2,CITY_RESPECT_HIGH,2} // black beans
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
    // MAP #1 (Icelands)
    {
        // NAME         FACILITY
        {s"frieren   ", 0, 
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
    },
    // MAP #2 (Arid Plain)
    {
        {s"arakis    ", 0, 
            // Demands
            {{0xff,0},{0xff,0},{0xff,0},{0xff,0}},
            // For sale
            {{0xff, 2, CITY_RESPECT_LOW, 2},
             {0xff, 2, CITY_RESPECT_LOW, 1},
             {0xff, 2, CITY_RESPECT_MED,  1},
             {0xff, 2, CITY_RESPECT_HIGH, 1}
            }
        },
        {s"alensia   ", 0,
            {{0xff,0},{0xff,0},{0xff,0},{0xff,0}},
            {{0xff, 2, CITY_RESPECT_LOW, 2},
             {0xff, 2, CITY_RESPECT_LOW, 1},
             {0xff, 2, CITY_RESPECT_MED,  1},
             {0xff, 2, CITY_RESPECT_HIGH, 1}
            } 
        },
        {s"tatooine  ", 0,
            {{0xff,0},{0xff,0},{0xff,0},{0xff,0}},
            {{0xff, 2, CITY_RESPECT_LOW, 2},
             {0xff, 2, CITY_RESPECT_LOW, 1},
             {0xff, 2, CITY_RESPECT_MED,  1},
             {0xff, 2, CITY_RESPECT_HIGH, 1}
            } 
        }
    },
    
    // MAP #3 (Jungle)
    {
        {s"swampland ", 0, 
            // Demands
            {{0xff,0},{0xff,0},{0xff,0},{0xff,0}},
            // For sale
            {{0xff, 2, CITY_RESPECT_LOW, 2},
             {0xff, 2, CITY_RESPECT_LOW, 1},
             {0xff, 2, CITY_RESPECT_MED,  1},
             {0xff, 2, CITY_RESPECT_HIGH, 1}
            }
        },
        {s"acacia    ", 0,
            {{0xff,0},{0xff,0},{0xff,0},{0xff,0}},
            {{0xff, 2, CITY_RESPECT_LOW, 2},
             {0xff, 2, CITY_RESPECT_LOW, 1},
             {0xff, 2, CITY_RESPECT_MED,  1},
             {0xff, 2, CITY_RESPECT_HIGH, 1}
            } 
        },
        {s"tree top  ", 0,
            {{0xff,0},{0xff,0},{0xff,0},{0xff,0}},
            {{0xff, 2, CITY_RESPECT_LOW, 2},
             {0xff, 2, CITY_RESPECT_LOW, 1},
             {0xff, 2, CITY_RESPECT_MED,  1},
             {0xff, 2, CITY_RESPECT_HIGH, 1}
            } 
        }
    },

    // MAP #4 (Frigidia)
    {
        {s"vurfel    ", 0, 
            // Demands
            {{0xff,0},{0xff,0},{0xff,0},{0xff,0}},
            // For sale
            {{0xff, 2, CITY_RESPECT_LOW, 2},
             {0xff, 2, CITY_RESPECT_LOW, 1},
             {0xff, 2, CITY_RESPECT_MED,  1},
             {0xff, 2, CITY_RESPECT_HIGH, 1}
            }
        },
        {s"blitzen   ", 0,
            {{0xff,0},{0xff,0},{0xff,0},{0xff,0}},
            {{0xff, 2, CITY_RESPECT_LOW, 2},
             {0xff, 2, CITY_RESPECT_LOW, 1},
             {0xff, 2, CITY_RESPECT_MED,  1},
             {0xff, 2, CITY_RESPECT_HIGH, 1}
            } 
        },
        {s"gliss     ", 0,
            {{0xff,0},{0xff,0},{0xff,0},{0xff,0}},
            {{0xff, 2, CITY_RESPECT_LOW, 2},
             {0xff, 2, CITY_RESPECT_LOW, 1},
             {0xff, 2, CITY_RESPECT_MED,  1},
             {0xff, 2, CITY_RESPECT_HIGH, 1}
            } 
        }
    },

    // MAP #5 (Vulcania)
    {
        {s"feuer     ", 0, 
            // Demands
            {{0xff,0},{0xff,0},{0xff,0},{0xff,0}},
            // For sale
            {{0xff, 2, CITY_RESPECT_LOW, 2},
             {0xff, 2, CITY_RESPECT_LOW, 1},
             {0xff, 2, CITY_RESPECT_MED,  1},
             {0xff, 2, CITY_RESPECT_HIGH, 1}
            }
        },
        {s"forge     ", 0,
            {{0xff,0},{0xff,0},{0xff,0},{0xff,0}},
            {{0xff, 2, CITY_RESPECT_LOW, 2},
             {0xff, 2, CITY_RESPECT_LOW, 1},
             {0xff, 2, CITY_RESPECT_MED,  1},
             {0xff, 2, CITY_RESPECT_HIGH, 1}
            } 
        },
        {s"ash peak  ", 0,
            {{0xff,0},{0xff,0},{0xff,0},{0xff,0}},
            {{0xff, 2, CITY_RESPECT_LOW, 2},
             {0xff, 2, CITY_RESPECT_LOW, 1},
             {0xff, 2, CITY_RESPECT_MED,  1},
             {0xff, 2, CITY_RESPECT_HIGH, 1}
            } 
        }
    },

    // MAP #6 (Darks end)
    {
        {s"abyss     ", 0, 
            // Demands
            {{0xff,0},{0xff,0},{0xff,0},{0xff,0}},
            // For sale
            {{0xff, 2, CITY_RESPECT_LOW, 2},
             {0xff, 2, CITY_RESPECT_LOW, 1},
             {0xff, 2, CITY_RESPECT_MED,  1},
             {0xff, 2, CITY_RESPECT_HIGH, 1}
            }
        },
        {s"schwartz  ", 0,
            {{0xff,0},{0xff,0},{0xff,0},{0xff,0}},
            {{0xff, 2, CITY_RESPECT_LOW, 2},
             {0xff, 2, CITY_RESPECT_LOW, 1},
             {0xff, 2, CITY_RESPECT_MED,  1},
             {0xff, 2, CITY_RESPECT_HIGH, 1}
            } 
        },
        {s"void      ", 0,
            {{0xff,0},{0xff,0},{0xff,0},{0xff,0}},
            {{0xff, 2, CITY_RESPECT_LOW, 2},
             {0xff, 2, CITY_RESPECT_LOW, 1},
             {0xff, 2, CITY_RESPECT_MED,  1},
             {0xff, 2, CITY_RESPECT_HIGH, 1}
            } 
        }
    },

    // MAP #7 (Lands end)
    {
        {s"utopia    ", 0, 
            // Demands
            {{0xff,0},{0xff,0},{0xff,0},{0xff,0}},
            // For sale
            {{0xff, 2, CITY_RESPECT_LOW, 2},
             {0xff, 2, CITY_RESPECT_LOW, 1},
             {0xff, 2, CITY_RESPECT_MED,  1},
             {0xff, 2, CITY_RESPECT_HIGH, 1}
            }
        },
        {s"drift     ", 0,
            {{0xff,0},{0xff,0},{0xff,0},{0xff,0}},
            {{0xff, 2, CITY_RESPECT_LOW, 2},
             {0xff, 2, CITY_RESPECT_LOW, 1},
             {0xff, 2, CITY_RESPECT_MED,  1},
             {0xff, 2, CITY_RESPECT_HIGH, 1}
            } 
        },
        {s"zuruck    ", 0,
            {{0xff,0},{0xff,0},{0xff,0},{0xff,0}},
            {{0xff, 2, CITY_RESPECT_LOW, 2},
             {0xff, 2, CITY_RESPECT_LOW, 1},
             {0xff, 2, CITY_RESPECT_MED,  1},
             {0xff, 2, CITY_RESPECT_HIGH, 1}
            } 
        }
    },


};
// variable city data - MUST BE SAVED with SAVE GAME
extern CityDataVar citiesVar[CITY_MAPS_COUNT][CITY_NUM_CITIES_PER_MAP];

void City_initCityVariables(void)
{
    for (unsigned char map=0; map<CITY_MAPS_COUNT; map++) {
        for (unsigned char city=0; city<CITY_NUM_CITIES_PER_MAP; city++) {
            citiesVar[map][city].respectLevel = CITY_RESPECT_LOW;
            citiesVar[map][city].status = CITY_STATUS_CITY;
        }
    }
    // set up MULE city and other special statuses here
    //citiesVar[0][1].status = CITY_STATUS_MULE;
}

void City_returnUnusedPassengers(Passenger *tempPsgData)
{
    for (unsigned char index=0;index<10;index++) {
        if (tempPsgData[index].destination.code != 0) {
            City_returnName(tempPsgData[index].name);
        }
    }
}

void City_generateCurrentCityTmpData(Passenger *tempPsgData, CityCode currentCity) // Assume size 10 array
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

void City_returnName(char *name)
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
