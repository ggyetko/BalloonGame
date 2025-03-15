#include "quest.h"

// if it's 0, this is a new quest, unknown to the user
// if it's 1, this quest is 
//     a) in progress in the Quest Log
//     b) complete and reward claimed if not in Quest Log
unsigned char questBitmap[1] = {0};

// constant quest data
const Quest allQuests[QUEST_COUNT] = {
    {s"bronze    ",
     0b00000001,        // Cloud City
     CITY_RESPECT_NONE,
     {0b00000011},        // Sirenia
     9,                 // Bronze
     2, // should be 20
     {REWARD_RESPECT_MED,0,0},
     //0---------0---------2---------0---------4---------0---------6---------0---------8---------0---------
     s"please deliver 20   crates of bronze to our neighbours in   sirenia                                 ",
     s"you have earned my  trust. thanks for   delivering our goods"
    },
    {s"cloud lady",
     0b00000001,        // Cloud City
     CITY_RESPECT_MED,
     {0b00000010},      // Floria
     0xff,                 // Bronze
     1, // 1 person
     {REWARD_RESPECT_HIGH,0,0},
     //0---------0---------2---------0---------4---------0---------6---------0---------8---------0---------
     s"my daugter needs a  ride in your balloonto our friendly     neigbhours in floria                    ",
     s"thank you for safelytranporting my      daugther            "
    },
};

QuestLog questLog[MAX_QUESTS_IN_PROGRESS];

void Quest_init(void)
{
    for (unsigned char x = 0; x < MAX_QUESTS_IN_PROGRESS; x++) {
        questLog[x].questIndex = INVALID_QUEST_INDEX;
        questLog[x].completeness = INVALID_QUEST_INDEX;
    }
}

unsigned char Quest_checkComplete(CityCode const cityCode)
{
    for (unsigned x = 0; x<MAX_QUESTS_IN_PROGRESS; x++) {
        unsigned char questIndex = questLog[x].questIndex;
        if (questIndex != INVALID_QUEST_INDEX) {
            if (
                ((allQuests[questIndex].cityNumber.code & 0x1f) == cityCode.code)
                && (questLog[x].completeness == allQuests[questIndex].numItems)) {
                // remove it from the log
                questLog[x].questIndex = INVALID_QUEST_INDEX;
                return questIndex;
            }
        }
    }    
    return INVALID_QUEST_INDEX;
}

// call this whenever a good is sold to anyone
void Quest_processDeliverTrigger(unsigned char const itemIndex, CityCode const destCity)
{
    for (unsigned x = 0; x<MAX_QUESTS_IN_PROGRESS; x++) {
        unsigned char questIndex = questLog[x].questIndex;
        if (questIndex != INVALID_QUEST_INDEX) {
            // check for "sell my stuff" quests
            /*debugChar(1,x);
            debugChar(2,allQuests[questIndex].cityNumber.code);
            debugChar(3,allQuests[questIndex].destinationCity.code);
            debugChar(4,destCity.code);
            debugChar(5,allQuests[questIndex].itemIndex);
            debugChar(6,itemIndex);
            debugChar(7,questLog[x].completeness);
            debugChar(8,allQuests[questIndex].numItems);*/
            
            if (((allQuests[questIndex].cityNumber.code & QUEST_TYPE_MASK) == QUEST_TYPE_SELL)
                && (allQuests[questIndex].destinationCity.code == destCity.code)
                && (allQuests[questIndex].itemIndex == itemIndex)
                && (questLog[x].completeness < allQuests[questIndex].numItems)) {
                questLog[x].completeness ++;
            }
            // check for "buy me this stuff" quests
        }
    }
}

// call this whenever a passenger arrives anywhere
void Quest_processArrivalTrigger(char const *name, CityCode const destCity)
{
    return INVALID_QUEST_INDEX;    
}

// this will tell the caller if the mayor wants an item, 0xff if nothing
unsigned char Quest_getMayorDesire(char const CityCode)
{
    
}

bool isQuestLogged(unsigned char questIndex)
{
    unsigned char index = questIndex >> 3;
    unsigned char column = questIndex & 3;
    return questBitmap[index] >> column;
}

bool logQuest(unsigned char questIndex)
{
    bool logged = false;
    unsigned char q;
    for (q=0;q<MAX_QUESTS_IN_PROGRESS;q++) {
        if (questLog[q].questIndex == INVALID_QUEST_INDEX) {
            questLog[q].questIndex = questIndex;
            questLog[q].completeness = 0;
            logged = true;
            break;
        }
    }
    if (logged) {
        debugChar(1,99);
        unsigned char index = questIndex >> 3;
        unsigned char column = questIndex & 3;
        questBitmap[index] |= 1 << column;
    }
    return logged;
}

// the quest is complete AND claimed. Remove it.
void unLogQuest(unsigned char questIndex)
{
    for (unsigned char q=0;q<MAX_QUESTS_IN_PROGRESS;q++) {
        if (questLog[q].questIndex == questIndex) {
            questLog[q].questIndex = INVALID_QUEST_INDEX;
            questLog[q].completeness = 0;
            break;
        }
    }
}

// returns the index of an available Quest
// INVALID_QUEST_INDEX if no quest available
// there should only be one Quest per respect level per city.
unsigned char Quest_getCityQuest(CityCode const city, unsigned char currCityRespect)
{
    unsigned char q;
    for (unsigned char q=0; q<QUEST_COUNT; q++) {
        if (isQuestLogged(q)) continue;
        if (((allQuests[q].cityNumber.code & 0x1f) == city.code) && (allQuests[q].respectLevel <= currCityRespect)) {
            if (logQuest(q)) {
                return q;
            } else {
                return INVALID_QUEST_INDEX;
            }
        }
    }
    return INVALID_QUEST_INDEX;    
}