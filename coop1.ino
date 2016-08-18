#include <DS3231_Simple.h>
#include <EEPROM.h>
#include "Sol.h"

const bool DEBUG = false;

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
bool switchPressed = false;
Sol Sun;
DS3231_Simple Clock;
volatile boolean alarmIsrWasCalled = false;

const int LIGHTS_PIN = 10;
const int MOTOR_PWM_PIN = 5;
const int MOTOR_DIR_PIN = 6;
const int UP_SWITCH_PIN = 14;
const int DOWN_SWITCH_PIN = 16;
const int RTC_SQW_INTERRUPT = 4;
const int MOTOR_UP_SECONDS = 60;
const int MOTOR_DOWN_SECONDS = 60;

void setup() {
  Serial.begin(9600);

  pinMode(LIGHTS_PIN, OUTPUT);
  pinMode(MOTOR_PWM_PIN, OUTPUT);
  pinMode(MOTOR_DIR_PIN, OUTPUT);
  pinMode(UP_SWITCH_PIN, INPUT_PULLUP);
  pinMode(DOWN_SWITCH_PIN, INPUT_PULLUP);

  digitalWrite(MOTOR_PWM_PIN, LOW);
  digitalWrite(MOTOR_DIR_PIN, LOW);

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

  setupAlarm();
  currentStep = EEPROM.read(1);
  setNextAlarm();
}

void setupAlarm() {
  Clock.disableSquareWave();

  pinMode(RTC_SQW_INTERRUPT, INPUT_PULLUP);
  attachInterrupt(RTC_SQW_INTERRUPT, alarmIsr, FALLING);

  if (DEBUG) {
    DateTime alarm1 = Clock.read();
    alarm1.Second = 20;
    Clock.setAlarm(alarm1, DS3231_Simple::ALARM_MATCH_SECOND);
    Clock.setAlarm(DS3231_Simple::ALARM_EVERY_MINUTE);
  }
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
  if (digitalRead(UP_SWITCH_PIN) == LOW) {
      digitalWrite(MOTOR_DIR_PIN, LOW);
      digitalWrite(MOTOR_PWM_PIN, HIGH);
      switchPressed = true;
  }
  
  if (digitalRead(DOWN_SWITCH_PIN) == LOW) {
      digitalWrite(MOTOR_DIR_PIN, HIGH);
      digitalWrite(MOTOR_PWM_PIN, HIGH);
      switchPressed = true;
  }
  
  if (digitalRead(UP_SWITCH_PIN) == HIGH && digitalRead(DOWN_SWITCH_PIN) == HIGH && switchPressed) {
       digitalWrite(MOTOR_PWM_PIN, LOW);
       switchPressed = false;
  }
  
  if (alarmIsrWasCalled) {
    uint8_t AlarmsFired = Clock.checkAlarms();

    if (AlarmsFired & 1)
    {
      printAction(F("First alarm fired"));
    }
    
    if (AlarmsFired & 2)
    {
      printAction(F("Second alarm fired"));
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
      digitalWrite(LIGHTS_PIN, HIGH);
      break;

     case DOOR_OPEN:
      printAction(F("Opening door"));
      digitalWrite(MOTOR_DIR_PIN, LOW);
      digitalWrite(MOTOR_PWM_PIN, HIGH);
      delay(MOTOR_UP_SECONDS * 1000);
      digitalWrite(MOTOR_PWM_PIN, LOW);
      break;

     case AM_LIGHTS_OFF:
      printAction(F("Turning off morning lights"));
      digitalWrite(LIGHTS_PIN, LOW);
      break;

     case PM_LIGHTS_ON:
      printAction(F("Turning on evening lights"));
      digitalWrite(LIGHTS_PIN, HIGH);
      break;

     case DOOR_CLOSE:
      printAction(F("Closing door"));
      digitalWrite(MOTOR_DIR_PIN, HIGH);
      digitalWrite(MOTOR_PWM_PIN, HIGH);
      delay(MOTOR_DOWN_SECONDS * 1000);
      digitalWrite(MOTOR_PWM_PIN, LOW);
      break;

     case PM_LIGHTS_OFF:
      printAction(F("Turning off evening lights"));
      digitalWrite(LIGHTS_PIN, LOW);
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
  short offset = timeParams.GMTOffsetHours + timeParams.CurrentDSTOffset();
  String out = F("Setting alarm to 20");
  out.concat(toSet.Year);
  out.concat('-');
  out.concat(zeroPad(toSet.Month));
  out.concat('-');
  out.concat(zeroPad(toSet.Day));
  out.concat('T');
  out.concat(zeroPad(toSet.Hour));
  out.concat(':');
  out.concat(zeroPad(toSet.Minute));
  out.concat(':');
  out.concat(zeroPad(toSet.Second));
  out.concat((offset < 0 ? '-' : '+'));
  out.concat(abs(offset));
  out.concat(F(":00"));
  printAction(out);
  Clock.setAlarm(toSet, DS3231_Simple::ALARM_MATCH_SECOND_MINUTE_HOUR);
}

String zeroPad(byte s) {
  if (s < 10) {
    String out = "0";
    out.concat(s);
    return out;
  }

  return String(s);
}

void printAction(String action) {
  Clock.printTo(Serial);
  Serial.print(F(": "));
  Serial.println(action);
}

void alarmIsr() {
    alarmIsrWasCalled = true;
}
