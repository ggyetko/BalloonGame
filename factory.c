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
    // GRUEL FACTORY (FAKE, for testing)
    {
    GOODS_CORN,
    GOODS_RICE,
    GOODS_GRUEL,
    1,
    2
    },
    
};
FactoryCurrent factoriesStatus[NUM_FACTORIES];

void Factory_initFactoryStatuses(void)
{
    factoriesStatus[0].currentOutputCount = 1;
}

// not completely convinced I need "count" parameter here. Will I ever deliver more than one?
bool Factory_addGoodsToFactory(byte factoryIndex, byte goodsIndex, byte count)
{
    // add the input
    if (goodsIndex == factories[factoryIndex].inputGoodsIndex) {
        factoriesStatus[factoryIndex].currentInputCount1 ++;
    } else if (goodsIndex == factories[factoryIndex].inputGoods2Index) {
        factoriesStatus[factoryIndex].currentInputCount2 ++;
    } else {
        return false;
    }
    // check if we can make the output
    if (factoriesStatus[factoryIndex].currentInputCount1 >= factories[factoryIndex].inputGoodsRequired
        && factoriesStatus[factoryIndex].currentInputCount2 >= factories[factoryIndex].inputGoods2Required) {
        factoriesStatus[factoryIndex].currentInputCount1 -= factories[factoryIndex].inputGoodsRequired;
        factoriesStatus[factoryIndex].currentInputCount2 -= factories[factoryIndex].inputGoods2Required;
        factoriesStatus[factoryIndex].currentOutputCount ++;
        return true;
    }
    return false;
}

byte Factory_getOutputType(byte factoryIndex)
{
    return factories[factoryIndex].outputGoodsIndex;
}

byte Factory_getOutputCount(byte factoryIndex)
{
    return factoriesStatus[factoryIndex].currentOutputCount;
}

byte Factory_takeOutput(byte factoryIndex)
{
    if (factoriesStatus[factoryIndex].currentOutputCount) {
        factoriesStatus[factoryIndex].currentOutputCount --;
        return factories[factoryIndex].outputGoodsIndex;
    }
    return NO_GOODS;
}
