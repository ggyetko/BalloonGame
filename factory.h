#include <c64/types.h>
#include "goods.h"

#ifndef FACTORY_H
#define FACTORY_H

#define NUM_FACTORIES 4
#define FACTORY_INDEX_NONE 255

struct Factory {
    byte inputGoodsIndex;
    byte inputGoods2Index;    // make this NO_GOODS if not needed
    byte outputGoodsIndex;
    byte inputGoodsRequired;  // how many of first good needed for one output
    byte inputGoods2Required; // how many of seconds good needed for one output (0 if not needed)
    // The output of a factory is always one unit
};

struct FactoryCurrent {
    byte currentInputCount1;
    byte currentInputCount2;
    byte currentOutputCount;
};

extern const Factory factories[NUM_FACTORIES];
extern FactoryCurrent factoriesStatus[NUM_FACTORIES]; // TBD must be saved

void initFactoryStatuses(void);
void addGoodsToFactory(byte factoryIndex, byte goodsIndex, byte count);
byte getOutputType(byte factoryIndex);
byte getOutputCount(byte factoryIndex);
byte takeOutput(byte factoryIndex); // returns outputGoodsIndex

#endif