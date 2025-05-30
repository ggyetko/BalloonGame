#include "goods.h"
#include "namedGoods.h"

const Goods goods[NUM_GOODS] =
{
    // VEGETABLES
    {s"rice      ", 75,  3}, // 0
    {s"wheat     ", 75,  3}, // 1
    {s"corn      ", 75,  3}, // 2
    {s"spinach   ", 75,  3}, // 3
    {s"peppers   ", 75,  3}, // 4
    {s"seaweed   ", 75,  3}, // 5
    {s"mushrooms ", 75,  3}, // 6
    {s"soy beans ", 200, 3}, // 7
   //METALS
    {s"copper    ", 300, 10}, // 8
    {s"bronze    ", 300, 10}, // 9
    {s"silver    ", 300, 10}, // 10
    {s"iron      ", 500,  3}, // 11
    {s"aluminum  ", 500,  3}, // 12
    {s"gold      ", 500,  3}, // 13
    {s"smithore  ", 500,  3}, // 14
    {s"rfnd smthr",1500,  3}, // 15
    // Animal products
    {s"milk      ", 75,  3}, // 16
    {s"beef      ", 75,  3}, // 17
    {s"pork      ", 75,  3}, // 18
    {s"eggs      ", 100,  3}, // 19
    {s"quail eggs", 300,  3}, // 20
    {s"          ", 75,  3}, // 21
    {s"          ", 75,  3}, // 22
    {s"          ", 75,  3}, // 23
    // More veggies
    {s"bok choy  ", 300,  3}, // 24
    {s"blackbeans", 400,  3}, // 25 
    {s"wintermeln", 75,  3}, // 26
    {s"          ", 75,  3}, // 27
    {s"          ", 75,  3}, // 28
    {s"          ", 75,  3}, // 29
    {s"          ", 75,  3}, // 30
    {s"          ", 75,  3}, // 31
    // MANUFACTURED GOODS MINERALS
    {s"gruel     ", 75,  3}, // 32
    {s"coal      ", 75,  3}, // 33
    {s"          ", 75,  3}, // 
    {s"          ", 75,  3}, // 
    {s"          ", 75,  3}, // 
    {s"          ", 75,  3}, // 
    {s"          ", 75,  3}, // 
    {s"crystite  ", 75,  3}, // 39
    // MILITARY
    {s"          ", 75,  3}, // 40
    {s"          ", 75,  3}, //
    {s"          ", 75,  3}, // 
    {s"          ", 75,  3}, // 
    {s"          ", 75,  3}, // 
    {s"          ", 75,  3}, // 
    {s"          ", 75,  3}, // 
    {s"          ", 75,  3}  // 47
    // 254 is a damaged cargo slot
    // 255 is an empty slot or invalid code
};
