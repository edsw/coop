#ifndef Coop_Types_Cpp
#define Coop_Types_Cpp

#include <Arduino.h>
#include "RTClib.h"

struct Location
{
    double Latitude;
    double Longitude;
};

struct LocalTimeParams
{
    short GMTOffsetHours;
    byte DSTOffsetHours;
    byte DSTStartMonth;
    byte DSTStartWeek;
    byte DSTEndMonth;
    byte DSTEndWeek;
    DateTime CurrentTime;

    byte CurrentDSTOffset() //https://github.com/probonopd/TimeLord/blob/master/TimeLord.cpp
    {
        if (DSTOffsetHours == 0 || CurrentTime.month() < DSTStartMonth || CurrentTime.month() > DSTEndMonth)
        {
            return 0;
        }
        
        if (CurrentTime.month() > DSTStartMonth && CurrentTime.month() < DSTEndMonth)
        {
            return DSTOffsetHours;
        }

        byte weekday = CurrentTime.dayOfTheWeek();
        byte nSundays = 0;
        byte prevSunday = CurrentTime.day() - weekday;
        if (prevSunday > 0)
        {
            nSundays = prevSunday / 7;
            nSundays++;
        }
        
        if (CurrentTime.month() == DSTStartMonth)
        {
            if (nSundays < DSTStartWeek) return 0;
            if (nSundays > DSTStartWeek) return DSTOffsetHours;
            if (weekday > 0) return DSTOffsetHours;
            if (CurrentTime.hour() > 1) return DSTOffsetHours;
            return 0;
        }
        
        if (nSundays < DSTEndWeek) return DSTOffsetHours;
        if (nSundays > DSTEndWeek) return 0;
        if (weekday > 0) return 0;
        if (CurrentTime.hour() > 1) return 0;
        return DSTOffsetHours;
    }
};
#endif
