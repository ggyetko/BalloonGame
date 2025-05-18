#ifndef UPGRADE_H
#define UPGRADE_H

#include <c64/vic.h>

#include "city.h"

enum BALLOON_UPGRADES {
    BALLOON_FIREPROOF  = 0x01,
    BALLOON_ICEPROOF   = 0x02,
    BALLOON_FIRSTCLASS = 0x04,
    BALLOON_GALLEY     = 0x08,
    BALLOON_AIRDROP    = 0x10,
    BALLOON_PORTAL     = 0x20,
};

struct Upgrade {
    char name[10];
    unsigned int cost;
    char facilityMask;
    char upgradeMask;
    char screenChar;
    unsigned char screenColor;
};

#define UPGRADE_NUM_UPGRADES 6
const Upgrade upgrades[UPGRADE_NUM_UPGRADES] = {
    {s"fire proof", 8000, CITY_FACILITY_FIREPROOF_UPGRADE, BALLOON_FIREPROOF, 80, VCOL_RED},
    {s"ice proof ", 8000, CITY_FACILITY_ICEPROOF_UPGRADE, BALLOON_ICEPROOF, 81, VCOL_LT_BLUE},
    {s"1st class ",12000, CITY_FACILITY_FIRSTCLASS_UPGRADE, BALLOON_FIRSTCLASS, 83, VCOL_GREEN},
    {s"galley    ", 6000, CITY_FACILITY_GALLEY_UPGRADE, BALLOON_GALLEY, 124, VCOL_ORANGE},
    {s"air drop  ", 5000, CITY_FACILITY_AIRDROP_UPGRADE, BALLOON_AIRDROP, 82, VCOL_WHITE},
    {s"portal dvc", 10, CITY_FACILITY_PORTAL_UPGRADE, BALLOON_PORTAL, 122, VCOL_PURPLE},
};

#endif