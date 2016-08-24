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

const int LIGHT_PIN = 10,
          MOTOR_PWM_PIN = 5,
          MOTOR_DIR_PIN = 6,
          UP_SWITCH_PIN = 16,
          DOWN_SWITCH_PIN = 14,
          RTC_SQW_INTERRUPT = 4;

const int MOTOR_UP_SECONDS = 2,//60
          MOTOR_DOWN_SECONDS = 2;//60;

const int EEPROM_DST = 0,
          EEPROM_ACTION = 1,
          EEPROM_DOOR_STATE = 2;

Sol Sun;
DS3231_Simple Clock;

bool switchPressed = false;
volatile boolean alarmIsrWasCalled = false;

void setup() {
  Serial.begin(9600);
  delay(3000);
  Clock.begin();

  pinMode(LIGHT_PIN, OUTPUT);
  pinMode(MOTOR_PWM_PIN, OUTPUT);
  pinMode(MOTOR_DIR_PIN, OUTPUT);
  pinMode(UP_SWITCH_PIN, INPUT_PULLUP);
  pinMode(DOWN_SWITCH_PIN, INPUT_PULLUP);

  digitalWrite(MOTOR_PWM_PIN, LOW);
  digitalWrite(MOTOR_DIR_PIN, LOW);

  if (DEBUG) {
    printAction(F("Entering DEBUG mode"));
    timeParams.CurrentTime = DateTime(F(__DATE__), F(__TIME__));
    Clock.write(timeParams.CurrentTime);

    if (timeParams.DSTOffsetHours != 0) {
      byte val = (int)(timeParams.CurrentDSTOffsetHours() != 0);
      printAction("Currently in DST? " + (String)val);
      EEPROM.write(EEPROM_DST, val);
    }
  }

  adjustTime();
  setupAlarm();
  
  if (!DEBUG) {
    COOP_ACTIONS next = getNextAlarm();
    printAction("Setting next alarm " + (String)next);
    EEPROM.write(EEPROM_ACTION, next);
    setNextAlarm();
  }
  else {
    EEPROM.write(EEPROM_ACTION, 0);
  }

  lightToggle(true);
}

void adjustTime() {
  printAction(F("Adjusting time"));
  DateTime now = Clock.read();

  if (timeParams.DSTOffsetHours != 0) {
    bool timeSetDst = (bool)EEPROM.read(EEPROM_DST);
    printAction("Currently in DST? " + (String)timeSetDst);
    byte offset = timeParams.CurrentDSTOffsetHours();
    printAction("Current DST offset: " + (String)offset);

    if (timeSetDst && offset == 0) {
      now = now - TimeSpan(0, offset, 0, 0);
    }
    else if (!timeSetDst && offset != 0) {
      now = now + TimeSpan(0, offset, 0, 0);
    }
  }

  if (DEBUG) {
    printAction("Adjusted time: " + now.ToISO8601(timeParams.CurrentDSTOffset()));
    Sun.PrintTo(Serial);
  }
  
  timeParams.CurrentTime = now;
  if (DEBUG) {
    printAction("Adjusted set time: " + timeParams.CurrentTime.ToISO8601(timeParams.CurrentDSTOffset()));
  }

  Sun = Sol(location, timeParams);

  if (DEBUG) {
    Sun.PrintTo(Serial);
  }
}

void setupAlarm() {
  printAction(F("Configuring alarm interrupt"));
  Clock.disableSquareWave();

  pinMode(RTC_SQW_INTERRUPT, INPUT_PULLUP);
  attachInterrupt(RTC_SQW_INTERRUPT, alarmIsr, FALLING);

  if (DEBUG) {
    printAction(F("Configuring DEBUG alarms"));
    DateTime alarm1 = Clock.read();
    alarm1.Second = 20;
    Clock.setAlarm(alarm1, DS3231_Simple::ALARM_MATCH_SECOND);
    Clock.setAlarm(DS3231_Simple::ALARM_EVERY_MINUTE);
  }
}

void loop() {
  if (digitalRead(UP_SWITCH_PIN) == LOW) {
    motorOpen();
    switchPressed = true;
  }

  if (digitalRead(DOWN_SWITCH_PIN) == LOW) {
    motorClose();
    switchPressed = true;
  }

  if (digitalRead(UP_SWITCH_PIN) == HIGH && digitalRead(DOWN_SWITCH_PIN) == HIGH && switchPressed) {
    motorStop();
    switchPressed = false;
  }

  uint8_t AlarmsFired = Clock.checkAlarms();

  if (AlarmsFired & 1)
  {
    printAction(F("First alarm fired"));
    alarmIsrWasCalled = false;
    takeCurrentAction();
  }
  
  if (AlarmsFired & 2)
  {
    printAction(F("Second alarm fired"));
    alarmIsrWasCalled = false;
    takeCurrentAction();
  }
    
  if (alarmIsrWasCalled) {
    alarmIsrWasCalled = false;
    takeCurrentAction();
  }
}

void motorOpen() {
  printAction(F("Powering motor OPEN"));
  digitalWrite(MOTOR_DIR_PIN, LOW);
  digitalWrite(MOTOR_PWM_PIN, HIGH);
}

void motorClose() {
  printAction(F("Powering motor CLOSE"));
  digitalWrite(MOTOR_DIR_PIN, HIGH);
  digitalWrite(MOTOR_PWM_PIN, HIGH);
}

void motorStop() {
  printAction(F("Stopping motor"));
  digitalWrite(MOTOR_PWM_PIN, LOW);
}

void doorToggle(bool open) {
  if (open) {
    if (!isDoorOpen()) {
      printAction(F("Opening door"));
      motorOpen();
      delay(MOTOR_UP_SECONDS * 1000);
      motorStop();
      EEPROM.write(EEPROM_DOOR_STATE, 1);
    }
  }
  else {
    if (isDoorOpen()) {
      printAction(F("Closing door"));
      motorClose();
      delay(MOTOR_DOWN_SECONDS * 1000);
      motorStop();
      EEPROM.write(EEPROM_DOOR_STATE, 0);
    }
  }
}

bool isDoorOpen() {
  return (bool)EEPROM.read(EEPROM_DOOR_STATE);
}

void lightToggle(bool turnOn) {
  if (turnOn) {
    printAction(F("Turning on lights"));
    digitalWrite(LIGHT_PIN, HIGH);
  }
  else {
    printAction(F("Turning off lights"));
    digitalWrite(LIGHT_PIN, LOW);
  }
}

void takeCurrentAction() {
  adjustTime();
  COOP_ACTIONS currentStep = EEPROM.read(EEPROM_ACTION);
  printAction("Taking action " + (String)currentStep);

  switch (currentStep) {
    case AM_LIGHT_ON:
      lightToggle(true);
      break;

    case DOOR_OPEN:
      doorToggle(true);
      break;

    case AM_LIGHT_OFF:
      lightToggle(false);
      break;

    case PM_LIGHT_ON:
      lightToggle(true);
      break;

    case DOOR_CLOSE:
      doorToggle(false);
      break;

    case PM_LIGHT_OFF:
      lightToggle(false);
      break;
  }

  setNextAlarm();
  EEPROM.write(EEPROM_ACTION, currentStep == PM_LIGHT_OFF ? 0 : currentStep + 1);
}

void setNextAlarm() {
  DateTime toSet;

  COOP_ACTIONS action = EEPROM.read(EEPROM_ACTION);
  printAction("Setting alarm for action " + (String)action);

  switch (action) {
    case AM_LIGHT_ON:
      toSet = Sun.Sunrise;
      break;

    case DOOR_OPEN:
      toSet = Sun.Sunrise + TimeSpan(0, 0, 30, 0);
      break;

    case AM_LIGHT_OFF:
      toSet = Sun.Sunset - TimeSpan(0, 0, 30, 0);
      break;

    case PM_LIGHT_ON:
      toSet = Sun.Sunset;
      break;

    case DOOR_CLOSE:
      toSet = DateTime(timeParams.CurrentTime.Year, timeParams.CurrentTime.Month,
                       timeParams.CurrentTime.Day, 21, 0, 0);
      break;

    case PM_LIGHT_OFF:
      toSet = DateTime(timeParams.CurrentTime.Year, timeParams.CurrentTime.Month,
                       timeParams.CurrentTime.Day, 21, 0, 0) + TimeSpan(0, 8, 0, 0);
      break;
    case RESET:
      toSet = DateTime(timeParams.CurrentTime.Year, timeParams.CurrentTime.Month,
                       timeParams.CurrentTime.Day, 5, 0, 0);
      break;
  }

  printAction("Setting alarm to: " + toSet.ToISO8601(timeParams.CurrentDSTOffset()));
  Clock.setAlarm(toSet, DS3231_Simple::ALARM_MATCH_SECOND_MINUTE_HOUR);
}

COOP_ACTIONS getNextAlarm() {
  printAction(F("Configuring initial alarm"));
  adjustTime();
  uint32_t currentUnix = timeParams.CurrentTime.unixtime();
  uint32_t fiveAMUnix = (
                          DateTime(timeParams.CurrentTime.Year, timeParams.CurrentTime.Month,
                                   timeParams.CurrentTime.Day, 5, 0, 0)).unixtime();
  uint32_t ninePMUnix = (
                          DateTime(timeParams.CurrentTime.Year, timeParams.CurrentTime.Month,
                                   timeParams.CurrentTime.Day, 21, 0, 0)).unixtime();
  uint32_t sunriseUnix = Sun.Sunrise.unixtime();
  uint32_t sunsetUnix = Sun.Sunset.unixtime();
  uint32_t thirtyMinutes = TimeSpan(0, 0, 30, 0).totalseconds();

  if (currentUnix < fiveAMUnix) {
    return RESET;
  }

  if (currentUnix < sunriseUnix) {
    lightToggle(true);
    return AM_LIGHT_ON;
  }

  if (currentUnix < sunriseUnix + thirtyMinutes) {
    lightToggle(true);
    doorToggle(true);
    return DOOR_OPEN;
  }

  if (currentUnix < sunsetUnix - thirtyMinutes) {
    lightToggle(false);
    doorToggle(true);
    return AM_LIGHT_OFF;
  }

  if (currentUnix < sunsetUnix) {
    lightToggle(true);
    return PM_LIGHT_ON;
  }

  if (currentUnix < ninePMUnix) {
    lightToggle(true);
    return DOOR_CLOSE;
  }

  lightToggle(false);
  doorToggle(false);
  return PM_LIGHT_OFF;
}

void printAction(String action) {
  Clock.printTo(Serial);
  Serial.print(F(": "));
  Serial.println(action);
}

void alarmIsr() {
  printAction("SOME ACTION");
  alarmIsrWasCalled = true;
}
