#ifndef QUESTS_H
#define QUESTS_H

#include "city.h"

enum {
    REWARD_NONE         = 0, // the reward is whatever happened
    REWARD_RESPECT_MED  = 1,
    REWARD_RESPECT_HIGH = 2,
    REWARD_SPECIAL_ITEM = 3,
    REWARD_MAP_ACCESS   = 4
};

struct Reward{  
    unsigned char rewardType;
    unsigned char index; 
    // if the reward is: an item  - item index
    //                   a map    - map index
    unsigned char count; 
    // if the reward is: an item  - number of items
    //                      map   - irrelevant
};

#define INVALID_QUEST_INDEX    0xff
struct QuestLog{
    unsigned char questIndex;
    unsigned char completeness; 
    // 0xff - not started (default value)
    // 0    - quest started
    // numItems   - quest complete
    // numItems+1 - quest complete and reward claimed
};

#define QUEST_TEXT_LENGTH            100
#define QUEST_CONCLUSION_TEXT_LENGTH 60

struct Quest{
    unsigned char cityNumber;  // the home city of the reward (the mayor you talk to)
    // XX.MMM##  
    // XX - 00 (sell home city's item to other city) 
    //      01 (buy quest, bring home city something and sell it) 
    //      10 (bring item to mayor of home city)
    //      11 (escort a person to destination city)
    // .  - ?
    // MMM- map number of home city of quest
    // ## - city number of home city
    unsigned char respectLevel;    // the respect level needed to start the quest
    unsigned char destinationCity; // 0xff - none, otherwise ...MMM##
    unsigned char itemIndex; 
        // 00-01-10: item type involved in quest
        // 11: Person's ID from person list
    unsigned char numItems;  // number of items to be deliver (usually 1)
    Reward reward;
    char questExplanation[QUEST_TEXT_LENGTH];
    char questConclusion[QUEST_CONCLUSION_TEXT_LENGTH];
};

#define QUEST_COUNT 1
extern const Quest allQuests[QUEST_COUNT];

void Quest_init(void);

// call this whenever a good is sold to anyone
// returns if this completes a quest
// INVALID_QUEST_INDEX if it completes no quest
unsigned char Quest_processDeliverTrigger(unsigned char const itemIndex, CityCode const destCity);

// call this whenever a passenger arrives anywhere
// returns if this completes a quest
// INVALID_QUEST_INDEX if it completes no quest
unsigned char Quest_processArrivalTrigger(char const *name, CityCode const destCity);

// this will tell the caller if the mayor wants an item, 0xff if nothing
unsigned char Quest_getMayorDesire(char const CityCode);

// returns the index of an available Quest
// INVALID_QUEST_INDEX if no quest available
unsigned char Quest_getCityQuest(CityCode const city, CityData const *currCity);


#endif