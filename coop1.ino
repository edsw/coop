#include <DS3231_Simple.h>
#include <DateTime.h>
#include <TimeSpan.h>
#include "CoopTypes.cpp"
#include "Sol.h"
// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
//#include "RTClib.h"
#include "EEPROM.h"

const bool DEBUG = true;

const Location location = {
  Latitude: 41.931745,
  Longitude: -72.480238
};

LocalTimeParams timeParams = {
  GMTOffsetHours: -5,
  DSTOffsetHours: 1,
  DSTStartMonth: 3,
  DSTStartWeek: 2,
  DSTEndMonth: 11,
  DSTEndWeek: 1
};

Sol sun;
DS3231_Simple Clock;

void setup() {  
  Serial.begin(9600);
  delay(1000);

  Clock.begin();
  if (Clock.lostPower() || DEBUG) {
    timeParams.CurrentTime = DateTime(F(__DATE__), F(__TIME__));
    Clock.write(timeParams.CurrentTime);

    if (timeParams.DSTOffsetHours != 0) {
      EEPROM.write(0, (int)(timeParams.CurrentDSTOffset() != 0));
    }
  }
  else
    adjustTime();

  sun = Sol(location, timeParams);
}

void adjustTime() {
    DateTime now = Clock.read();

    if (timeParams.DSTOffsetHours != 0) {
      bool timeSetDst = (bool)EEPROM.read(0);
      byte offset = timeParams.CurrentDSTOffset();

      if (timeSetDst && offset == 0)
        now = now - TimeSpan(0, offset, 0, 0);
      else if (!timeSetDst && offset != 0)
        now = now + TimeSpan(0, offset, 0, 0);
    }
    
    timeParams.CurrentTime = now;
}

void loop () {
  adjustTime();
  printTimeInfo();

  //Clock.printTo(Serial);
  //Serial.println();
  
  sun.Update(timeParams.CurrentTime);
  //printTimeInfo();
  delay(5000);
}

void printTimeInfo() {
  int dstOffset = timeParams.CurrentDSTOffset();
  Serial.print("Current Time: ");
  Serial.print(timeParams.CurrentTime.Year);
  Serial.print('-');
  Serial.print(timeParams.CurrentTime.Month);
  Serial.print('-');
  Serial.print(timeParams.CurrentTime.Day);
  Serial.print(' ');
  Serial.print(timeParams.CurrentTime.Hour);
  Serial.print(':');
  Serial.print(timeParams.CurrentTime.Minute);
  Serial.print(':');
  Serial.print(timeParams.CurrentTime.Second);
  Serial.print(" (GMT");
  Serial.print(timeParams.GMTOffsetHours);
  if (dstOffset != 0) {
    Serial.print(", DST");
    Serial.print(dstOffset > 0 ? '+' : '-');
    Serial.print(dstOffset);
  }
  Serial.print(')');
  Serial.println();

  Serial.print("Sunrise: ");
  Serial.print(sun.Sunrise.Hour, DEC);
  Serial.print(':');
  Serial.print(sun.Sunrise.Minute, DEC);
  Serial.print(" Sunset: ");
  Serial.print(sun.Sunset.Hour, DEC);
  Serial.print(':');
  Serial.print(sun.Sunset.Minute, DEC);
  Serial.println();
}
