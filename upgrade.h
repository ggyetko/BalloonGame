#ifndef UPGRADE_H
#define UPGRADE_H

#include "city.h"

struct Upgrade {
    char name[10];
    unsigned int cost;
    char facilityMask;
    char upgradeMask;
};

#define UPGRADE_NUM_UPGRADES 6
Upgrade const upgrades[UPGRADE_NUM_UPGRADES] = {
    {"fire proof", 8000, CITY_FACILITY_FIREPROOF_UPGRADE, BALLOON_FIREPROOF},
    {"ice proof ", 8000, CITY_FACILITY_ICEPROOF_UPGRADE, BALLOON_ICEPROOF},
    {"1st class ",12000, CITY_FACILITY_FIRSTCLASS_UPGRADE, BALLOON_FIRSTCLASS},
    {"galley    ", 6000, CITY_FACILITY_GALLEY_UPGRADE, BALLOON_GALLEY},
    {"air drop  ", 5000, CITY_FACILITY_AIRDROP_UPGRADE, BALLOON_AIRDROP},
    {"portal dvc", 8000, CITY_FACILITY_PORTAL_UPGRADE, BALLOON_PORTAL},
};

#endif