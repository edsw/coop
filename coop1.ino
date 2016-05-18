#include "CoopTypes.cpp"
#include "Sol.h"
// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include "RTClib.h"
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

RTC_DS3231 rtc;
Sol sun;

void setup() {  
  Serial.begin(9600);

  if (!rtc.begin()) {
    Serial.println(F("Couldn't find RTC!"));
    while (1);
  }

  if (rtc.lostPower() || DEBUG) {
    timeParams.CurrentTime = DateTime(F(__DATE__), F(__TIME__));
    rtc.adjust(timeParams.CurrentTime);

    if (timeParams.DSTOffsetHours != 0) {
      EEPROM.write(0, (int)(timeParams.CurrentDSTOffset() != 0));
    }
  }
  else {
    adjustTime();
  }

  sun = Sol(location, timeParams);
}

void adjustTime() {
    DateTime now = rtc.now();

    if (timeParams.DSTOffsetHours != 0) {
      bool timeSetDst = (bool)EEPROM.read(0);
      byte offset = timeParams.CurrentDSTOffset();

      if (timeSetDst && offset == 0) {
        now = now - TimeSpan(0, offset, 0, 0);
      }
      else if (!timeSetDst && offset != 0) {
        now = now + TimeSpan(0, offset, 0, 0);
      }
    }
    
    timeParams.CurrentTime = now;
}

void loop () {
  adjustTime();
  sun.Update(timeParams.CurrentTime);
  printTimeInfo();
  delay(5000);
}

void printTimeInfo() {
  int dstOffset = timeParams.CurrentDSTOffset();
  Serial.print("Current Time: ");
  Serial.print(timeParams.CurrentTime.year());
  Serial.print('-');
  Serial.print(timeParams.CurrentTime.month());
  Serial.print('-');
  Serial.print(timeParams.CurrentTime.day());
  Serial.print(' ');
  Serial.print(timeParams.CurrentTime.hour());
  Serial.print(':');
  Serial.print(timeParams.CurrentTime.minute());
  Serial.print(':');
  Serial.print(timeParams.CurrentTime.second());
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
  Serial.print(sun.Sunrise.hour(), DEC);
  Serial.print(':');
  Serial.print(sun.Sunrise.minute(), DEC);
  Serial.print(" Sunset: ");
  Serial.print(sun.Sunset.hour(), DEC);
  Serial.print(':');
  Serial.print(sun.Sunset.minute(), DEC);
  Serial.println();
}
