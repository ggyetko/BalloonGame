#ifndef QUESTS_H
#define QUESTS_H

struct Quest{
    unsigned char cityNumber;  
    // XX.MMM##  
    // XX - 00 (sell home city's item to other city) 
    //      01 (buy quest, bring home city something and sell it) 
    //      10 (bring item to mayor of home city)
    //      11 ???
    // ?
    // MMM- map number of home city of quest
    // ## - city number of home city
    unsigned char destinationCity; // 0xff - none, otherwise ...MMM##
    unsigned char itemIndex; // item type involved in quest
    unsigned char numItems;  // number of items to be deliver (usually 1)
    unsigned char completeness; 
    // 0xff - not started (default value)
    // 0    - quest started
    // numItems   - quest complete
    // numItems+1 - quest complete and reward claimed
    char questExplanation[128];
};

#endif