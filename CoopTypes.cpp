#ifndef Coop_Types_Cpp
#define Coop_Types_Cpp

#include <Arduino.h>
#include <DS3231_Simple.h>

enum COOP_ACTIONS { AM_LIGHT_ON, DOOR_OPEN, AM_LIGHT_OFF,
                    PM_LIGHT_ON, DOOR_CLOSE, PM_LIGHT_OFF, RESET };

struct Location {
    double Latitude;
    double Longitude;
};

struct LocalTimeParams {
    short GMTOffsetHours;
    byte DSTOffsetHours;
    byte DSTStartMonth;
    byte DSTStartWeek;
    byte DSTEndMonth;
    byte DSTEndWeek;
    DateTime CurrentTime;

    short CurrentDSTOffsetHours() {//https://github.com/probonopd/TimeLord/blob/master/TimeLord.cpp
        if (DSTOffsetHours == 0 || CurrentTime.Month < DSTStartMonth || CurrentTime.Month > DSTEndMonth)
            return 0;
            
        if (CurrentTime.Month > DSTStartMonth && CurrentTime.Month < DSTEndMonth) {
            return DSTOffsetHours;
        }

        byte dow = CurrentTime.Dow;
        byte nSundays = 0;
        byte prevSunday = CurrentTime.Day - dow;
        if (prevSunday > 0) {
            nSundays = prevSunday / 7;
            nSundays++;
        }
        
        if (CurrentTime.Month == DSTStartMonth) {
            if (nSundays < DSTStartWeek) return 0;
            if (nSundays > DSTStartWeek) return DSTOffsetHours;
            if (dow > 0) return DSTOffsetHours;
            if (CurrentTime.Hour > 1) return DSTOffsetHours;
            return 0;
        }
        
        if (nSundays < DSTEndWeek) return DSTOffsetHours;
        if (nSundays > DSTEndWeek) return 0;
        if (dow > 0) return 0;
        if (CurrentTime.Hour > 1) return 0;
        return DSTOffsetHours;
    }

    short CurrentDSTOffset() {
      return GMTOffsetHours + CurrentDSTOffsetHours();
    }
};
#endif
