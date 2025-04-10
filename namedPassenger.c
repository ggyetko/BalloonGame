#include "namedPassenger.h"
#include "utils.h"

NamedPassengerStatus namedPassengerStatus[NAMED_PASSENGER_TOTAL] = {
    Passenger_Status_Inactive,
    Passenger_Status_Inactive_First_Class,
    Passenger_Status_Inactive,
};

NamedPassenger const namedPassengers[NAMED_PASSENGER_TOTAL] = {
    {
     0b00000001, // Cloud City
     0b00000010, // Floria
     s"ms cloud  "
    },
    {
     0b00000011, // Serenia
     0b00001101, // Map #3, City #1
     s"princess  "
    },
    {
     0b00000010, // Floria
     0b00000111, // Map #1, City #3, Hoth
     s"sir floria"
    },
};

void NamedPassenger_deboardPassenger(char const *name)
{
    unsigned char namedPsgrIndex;
    for (namedPsgrIndex=0;namedPsgrIndex<NAMED_PASSENGER_TOTAL;namedPsgrIndex++) {
        if ((namedPassengerStatus[namedPsgrIndex].status == Passenger_Status_Aboard_First_Class) 
            || (namedPassengerStatus[namedPsgrIndex].status == Passenger_Status_Aboard)) {
            if (tenCharCmp(namedPassengers[namedPsgrIndex].name,name) == 0) {
                namedPassengerStatus[namedPsgrIndex].status = Passenger_Status_Complete;
                break;
            }
        }
    }
}

void NamedPassenger_boardPassenger(char const *name)
{
    unsigned char namedPsgrIndex;
    for (namedPsgrIndex=0;namedPsgrIndex<NAMED_PASSENGER_TOTAL;namedPsgrIndex++) {
        if ((namedPassengerStatus[namedPsgrIndex].status == Passenger_Status_Waiting_First_Class) 
            || (namedPassengerStatus[namedPsgrIndex].status == Passenger_Status_Waiting)) {
            if (tenCharCmp(namedPassengers[namedPsgrIndex].name, name) == 0) {
                namedPassengerStatus[namedPsgrIndex].status += 2; // waiting -> aboard for either status
                break;
            }
        }
    }
}

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

void NamedPassenger_debug()
{
    for (unsigned char p=0; p<NAMED_PASSENGER_TOTAL; p++){
        debugChar(p*2+2, namedPassengerStatus[p].status);
        debugChar(p*2+3, namedPassengers[p].sourceCity.code);
    }
}

unsigned char NamedPassenger_getQuestPassenger(CityCode sourceCityCode)
{
    //debugWipe();
    //debugChar(0, sourceCityCode.code);
    for (unsigned char p=0; p<NAMED_PASSENGER_TOTAL; p++){
        //debugChar(p*2+2, namedPassengerStatus[p].status);
        //debugChar(p*2+3, namedPassengers[p].sourceCity.code);

        if (
            ((namedPassengerStatus[p].status == Passenger_Status_Waiting) 
             || (namedPassengerStatus[p].status == Passenger_Status_Waiting))
            && (sourceCityCode.code == namedPassengers[p].sourceCity.code)
           )
        {
            return p;
        }
    }
    return INVALID_NAMED_PASSENGER_INDEX;
}