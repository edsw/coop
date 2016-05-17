// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include "RTClib.h"
#include "Sol.h"

int LATITUDE = 41.931745;
int LONGITUDE = -72.480238;
int GMTOFFSET = -5;

RTC_DS3231 rtc;
DateTime compiled;
boolean timeSet, compiledDST;
int DSTStartMonth = 3;
int DSTStartWeek = 2;
int DSTEndMonth = 11;
int DSTEndWeek = 1;

Sol sun;

void setup () {
  Serial.begin(9600);

  timeSet = false;

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    // following line sets the RTC to the date & time this sketch was compiled
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    timeSet = true;
    compiled = DateTime(F(__DATE__), F(__TIME__));
    compiledDST = IsInDST(compiled);
    rtc.adjust(compiled);
  }

  sun = Sol(LATITUDE, LONGITUDE, GMTOFFSET, ((timeSet && compiledDST) || IsInDST(rtc.now())));
}

void loop () {
  Serial.println("loopy");

  sun.Update(rtc.now());
  DateTime rise = sun.Sunrise();
  DateTime set = sun.Sunset();
  
  Serial.print("Sunrise: ");
  Serial.print(rise.hour(), DEC);
  Serial.print(":");
  Serial.print(rise.minute(), DEC);
  Serial.print(" Sunset: ");
  Serial.print(set.hour(), DEC);
  Serial.print(":");
  Serial.print(set.minute(), DEC);
  Serial.println();

  delay(5000);
}

boolean IsInDST(DateTime dt) //https://github.com/probonopd/TimeLord/blob/master/TimeLord.cpp
{
  if (dt.month() < DSTStartMonth || dt.month() > DSTEndMonth)
  {
    return false;
  }

  if (dt.month() > DSTStartMonth && dt.month() < DSTEndMonth)
  {
    return true;
  }

  int weekday = dt.dayOfTheWeek();
  int nSundays = 0;
  int prevSunday = dt.day() - weekday;
  if (prevSunday > 0) { 
    nSundays = prevSunday / 7;
    nSundays++;
  }
  
  if (dt.month() == DSTStartMonth) {
    if (nSundays < DSTStartWeek) return false;
    if (nSundays > DSTStartWeek) return true;
    if (weekday > 0) return true;
    if (dt.hour() > 1) return true;
    return false;
  }
  
  if(nSundays < DSTEndWeek) return true;
  if(nSundays > DSTEndWeek) return false;
  if(weekday > 0) return false;
  if(dt.hour() > 1) return false;
  return true;
}

