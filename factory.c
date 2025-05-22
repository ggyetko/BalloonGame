#include "factory.h"
#include "goods.h"
#include "namedGoods.h"

const Factory factories[NUM_FACTORIES] = {
    // REFINED SMITHORE FACTORY
    {
    GOODS_SMITHORE,
    NO_GOODS,
    GOODS_REFINED_SMITHORE,
    2,
    0
    },
    // UKNOWN FACTORY
    {
    NO_GOODS,
    NO_GOODS,
    NO_GOODS,
    1,
    0
    },
    // UKNOWN FACTORY
    {
    NO_GOODS,
    NO_GOODS,
    NO_GOODS,
    1,
    0
    },
    // UKNOWN FACTORY
    {
    NO_GOODS,
    NO_GOODS,
    NO_GOODS,
    1,
    0
    },
    
};
FactoryCurrent factoriesStatus[NUM_FACTORIES];

void initFactoryStatuses(void);


void addGoodsToFactory(byte factoryIndex, byte goodsIndex, byte count)
{
    // add the input
    if (goodsIndex == factories[factoryIndex].inputGoodsIndex) {
        factoriesStatus[factoryIndex].currentInputCount1 ++;
    } else if (goodsIndex == factories[factoryIndex].inputGoods2Index) {
        factoriesStatus[factoryIndex].currentInputCount2 ++;
    } else {
        return;
    }
    // check if we can make the output
    if (factoriesStatus[factoryIndex].currentInputCount1 >= factories[factoryIndex].inputGoodsRequired
        && factoriesStatus[factoryIndex].currentInputCount2 >= factories[factoryIndex].inputGoods2Required) {
        factoriesStatus[factoryIndex].currentInputCount1 -= factories[factoryIndex].inputGoodsRequired;
        factoriesStatus[factoryIndex].currentInputCount2 -= factories[factoryIndex].inputGoods2Required;
        factoriesStatus[factoryIndex].currentOutputCount ++;
    }
}

byte getOutputType(byte factoryIndex)
{
    return factories[factoryIndex].outputGoodsIndex;
}

byte getOutputCount(byte factoryIndex)
{
    return factoriesStatus[factoryIndex].currentOutputCount;
}

byte takeOutput(byte factoryIndex)
{
    if (factoriesStatus[factoryIndex].currentOutputCount) {
        factoriesStatus[factoryIndex].currentOutputCount --;
        return factories[factoryIndex].outputGoodsIndex;
    }
    return NO_GOODS;
}
