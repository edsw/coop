#include <DS3231_Simple.h>
#include <EEPROM.h>
#include "Sol.h"

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

enum COOP_ACTIONS { AM_LIGHTS_ON, DOOR_OPEN, AM_LIGHTS_OFF,
                    PM_LIGHTS_ON, DOOR_CLOSE, PM_LIGHTS_OFF };
byte currentStep;
Sol Sun;
DS3231_Simple Clock;
volatile boolean alarmIsrWasCalled = false;

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

    EEPROM.write(1, 0);
  }
  else
    adjustTime();

  Sun = Sol(location, timeParams);

  setAlarm();
}

void setAlarm() {
  Clock.disableSquareWave();

  pinMode(7, INPUT_PULLUP);
  attachInterrupt(4, alarmIsr, FALLING);

  DateTime alarm1 = Clock.read();
  alarm1.Second = 20;
  Clock.setAlarm(alarm1, DS3231_Simple::ALARM_MATCH_SECOND);

  Clock.setAlarm(DS3231_Simple::ALARM_EVERY_MINUTE);
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
    Sun.Update(timeParams.CurrentTime);
}

void loop() {
  if (alarmIsrWasCalled) {
    uint8_t AlarmsFired = Clock.checkAlarms();

    if(AlarmsFired & 1)
    {
      Clock.printTo(Serial); Serial.println(F(": First alarm has fired!"));
    }
    
    if(AlarmsFired & 2)
    {
      Clock.printTo(Serial); Serial.println(F(": Second alarm has fired!"));
    }

    alarmIsrWasCalled = false;
    takeCurrentAction();
  }
}

void takeCurrentAction() {
  adjustTime();
  currentStep = EEPROM.read(1);

  switch (currentStep) {
    case AM_LIGHTS_ON:
      printAction(F("Turning on morning lights"));
      break;

     case DOOR_OPEN:
      printAction(F("Opening door"));
      break;

     case AM_LIGHTS_OFF:
      printAction(F("Turning off morning lights"));
      break;

     case PM_LIGHTS_ON:
      printAction(F("Turning on evening lights"));
      break;

     case DOOR_CLOSE:
      printAction(F("Closing door"));
      break;

     case PM_LIGHTS_OFF:
      printAction(F("Turning off evening lights"));
      break;
  }

  setNextAlarm();
  EEPROM.write(1, currentStep == PM_LIGHTS_OFF ? 0 : currentStep + 1);
}

void setNextAlarm() {
  adjustTime();
  DateTime toSet;

  switch (currentStep) {
    case AM_LIGHTS_ON:
      toSet = Sun.Sunrise;
      break;

     case DOOR_OPEN:
      toSet = timeParams.CurrentTime + TimeSpan(0, 0, 30, 0);
      break;

     case AM_LIGHTS_OFF:
      toSet = Sun.Sunset - TimeSpan(0, 0, 30, 0);
      break;

     case PM_LIGHTS_ON:
      toSet = Sun.Sunset;
      break;

     case DOOR_CLOSE:
      toSet = DateTime(timeParams.CurrentTime.Year, timeParams.CurrentTime.Month,
                timeParams.CurrentTime.Day, 21, 0, 0);
      break;

     case PM_LIGHTS_OFF:
      toSet = timeParams.CurrentTime + TimeSpan(0, 8, 0, 0);
      break;
  }

  //TODO: Convert to function like DateTime.printTo(Serial)
  byte offset = timeParams.GMTOffsetHours + timeParams.CurrentDSTOffset();
  String offsetText = (offset < 0 ? '-' : '+') + offset + ":00";
  
  printAction("Setting alarm to " + toSet.Year + '-' + toSet.Month + '-' + toSet.Day +
     'T' + toSet.Hour + ':' + toSet.Minute + ':' + toSet.Second + offsetText);
    
  Clock.setAlarm(toSet, DS3231_Simple::ALARM_MATCH_SECOND_MINUTE_HOUR);
}

void printAction(String action) {
  Clock.printTo(Serial);
  Serial.print(' ');
  Serial.print("Action: ");
  Serial.println(action);
}

void alarmIsr() {
    alarmIsrWasCalled = true;
}
