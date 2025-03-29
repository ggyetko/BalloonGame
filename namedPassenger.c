#include "namedPassenger.h"

NamedPassengerStatus namedPassengerStatus[NAMED_PASSENGER_TOTAL] = {
    Passenger_Status_Inactive,
    Passenger_Status_Inactive_First_Class,
    Passenger_Status_Inactive,
};

NamedPassenger const namedPassengers[NAMED_PASSENGER_TOTAL] = {
    {Passenger_Status_Inactive,
     0b00000001, // Cloud City
     0b00000010, // Floria
     s"ms cloud  "
    },
    {Passenger_Status_Inactive_First_Class,
     0b00000011, // Serenia
     0b00001101, // Map #3, City #1
     s"princess  "
    },
    {Passenger_Status_Inactive,
     0b00000010, // Floria
     0b00000111, // Map #1, City #3, Hoth
     s"sir floria"
    },
};

void NamedPassenger_activatePassenger(unsigned char passengerIndex)
{
    if (passengerIndex < NAMED_PASSENGER_TOTAL) {
        if (namedPassengerStatus[passengerIndex].status == Passenger_Status_Inactive_First_Class) {
            namedPassengerStatus[passengerIndex].status = Passenger_Status_Waiting_First_Class;
        } else {
            namedPassengerStatus[passengerIndex].status = Passenger_Status_Waiting;
        }
    }
}

unsigned char NamedPassenger_getQuestPassenger(CityCode sourceCityCode)
{
    for (unsigned char p=0; p<NAMED_PASSENGER_TOTAL; p++){
        if ((namedPassengers[p].status == Passenger_Status_Waiting || namedPassengers[p].status == Passenger_Status_Waiting)
            && (sourceCityCode.code == namedPassengers[p].sourceCity.code))
        {
            return p;
        }
    }
    return INVALID_NAMED_PASSENGER_INDEX;
}