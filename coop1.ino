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

enum COOP_ACTIONS { AM_LIGHTS_ON, DOOR_OPEN, AM_LIGHTS_OFF, PM_LIGHTS_ON, DOOR_CLOSE, PM_LIGHTS_OFF };
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
}

void loop () {
  /*adjustTime();
  Sun.Update(timeParams.CurrentTime);
  
  Clock.printTo(Serial); Serial.print(' ');
  Sun.PrintTo(Serial); Serial.println();

  delay(10000);*/

  //uint8_t alarmFired = Clock.checkAlarms();

  if (alarmIsrWasCalled) {
    uint8_t AlarmsFired = Clock.checkAlarms();

    if(AlarmsFired & 1)
    {
      Clock.printTo(Serial); Serial.println(": First alarm has fired!");
    }
    
    if(AlarmsFired & 2)
    {
      Clock.printTo(Serial); Serial.println(": Second alarm has fired!");
    }

    alarmIsrWasCalled = false;
  }
}

void takeCurrentAction() {
  byte step = EEPROM.read(1);

  switch (step) {
    case AM_LIGHTS_ON:
      break;

     case DOOR_OPEN:
      break;

     case AM_LIGHTS_OFF:
      break;

     case PM_LIGHTS_ON:
      break;

     case DOOR_CLOSE:
      break;

     case PM_LIGHTS_OFF:
      break;
  }

  EEPROM.write(1, step == PM_LIGHTS_OFF ? 0 : step + 1);
}

void setNextAlarm() {
  
}

void alarmIsr() {
    alarmIsrWasCalled = true;
}
