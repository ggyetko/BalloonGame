#ifndef NAMED_PASSENGERS_H
#define NAMED_PASSENGERS_H

#include "city.h"

#define NAMED_PASSENGER_TOTAL 3
#define INVALID_NAMED_PASSENGER_INDEX 0xff

enum {
    Passenger_Id_Ms_Cloud = 0,
    Passenger_Id_Princess = 1,
    Passenger_Id_Sir_Floria = 2,
};

enum {
    Passenger_Status_Inactive = 0,
    Passenger_Status_Inactive_First_Class,
    Passenger_Status_Waiting,
    Passenger_Status_Waiting_First_Class,
    Passenger_Status_Aboard,
    Passenger_Status_Aboard_First_Class,
    Passenger_Status_Complete
};    

struct NamedPassengerStatus {
    unsigned char status;
};
struct NamedPassenger {
    unsigned char status;
    CityCode sourceCity;
    CityCode destinationCity;
    char name[10];
};

extern NamedPassengerStatus namedPassengerStatus[NAMED_PASSENGER_TOTAL];
extern NamedPassenger const namedPassengers[NAMED_PASSENGER_TOTAL];

void NamedPassenger_activatePassenger(unsigned char passengerIndex);

unsigned char NamedPassenger_getQuestPassenger(CityCode sourceCityCode);

#endif