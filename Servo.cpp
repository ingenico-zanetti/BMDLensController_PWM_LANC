#include <Arduino.h>
#include <EEPROM.h>

#include "Servo.hpp"
#include "Lens.hpp"


Servo zoomServo( &zoomSettings,  "ZOOM",  0 * sizeof(ServoSettings));
Servo irisServo( &irisSettings,  "IRIS",  1 * sizeof(ServoSettings));
Servo focusServo(&focusSettings, "FOCUS", 2 * sizeof(ServoSettings));

Servo *getServo(int c){
  switch(c){
    case 'Z':
      return &zoomServo;
    break;
    case 'I':
      return &irisServo;
    break;
    case 'F':
      return &focusServo;
    break;
  }
  return(NULL);
}

void Servo::loadSettingsFromMemory(const ServoSettings *settings){
  memset(setPoints, 0, sizeof(setPoints));
  setPointCount = 0;
  while(settings->setPoints[setPointCount].adcValue){
    setPoints[setPointCount] = settings->setPoints[setPointCount];
    setPointCount++;
  }
  timeoutScale = settings->parameters.timeoutScale;
  pwmScale     = settings->parameters.pwmScale;
  pwmRatioMin  = settings->parameters.minSpeed;
}

void Servo::loadSettingsFromFW(void){
  loadSettingsFromMemory(servoSettingsFromFW);
}

bool Servo::loadSettingsFromEEPROM(void){
  bool raiseError = false;
  ServoSettings temp;
  memset(&temp, 0, sizeof(temp));
  EEPROM.get(eepromOffset, temp);
  unsigned short int adc = temp.setPoints[0].adcValue;
  if((0 < adc) && (adc < 0x0FFF)){
    loadSettingsFromMemory(&temp);
  }else{
    raiseError = true;
  }
  return raiseError;
}

void Servo::storeSettingsToEEPROM(){
  ServoSettings temp;
  memset(&temp, 0, sizeof(temp));
  int count = 0;
  while(setPoints[count].adcValue){
    temp.setPoints[count] = setPoints[count];
    count++;
  }
  temp.parameters.timeoutScale = timeoutScale;
  temp.parameters.pwmScale = pwmScale;
  temp.parameters.minSpeed = pwmRatioMin;
  EEPROM.put(eepromOffset, temp);
}


Servo::Servo(const ServoSettings *s, const char *name, unsigned int offset){
  servoSettingsFromFW = s;
  loadSettingsFromFW();
  eepromOffset = offset;
  loadSettingsFromEEPROM();
  szName = name;
  remainingTimeMs = 0;
  adcPin = -1;
  pwmPin = -1;
  dirPin = -1;
  dirPinPolarity = -1;
  adcMinValue = 4095;
  adcMaxValue = 0;
  pwmRatioMax = 0xFF; // 8-bit PWM
  filter = SlidingWindow(4);
  direction = DIRECTION_STOPPED;
  timeout = 0;
}

void Servo::print(const char *szUnit){
  for(int i = 0 ; i < setPointCount ; i++){
    char setPointSettingString[8];
    setPointSettingToString(setPointSettingString, setPoints + i);
    Serial.printf("setPoints[%2d]={%5s%2s, %4d steps}" "\n",
        i,
        setPointSettingString,
        szUnit,
        setPoints[i].adcValue
        );
  }
  Serial.printf("parameters={.pwmScale=%3u, .timeoutScale=%3u, .minSpeed=%3u}" "\n", pwmScale, timeoutScale, pwmRatioMin);
}

SetPoint *Servo::getSetPoints(int *actualCount){
  if(NULL != actualCount){
    *actualCount = setPointCount;
  }
  return(setPoints);
}

char *Servo::setPointSettingToString(char *szString, SetPoint *setPoint){
  int l = strlen(itoa(setPoint->setting, szString, 10));
  szString[l + 1] = '\0';
  szString[l] = szString[l - 1];
  szString[l - 1] = '.';
  return(szString);
}

int Servo::getSetPointIndex(unsigned short setting){
  int index = setPointCount;
  while(index--){
    if(setPoints[index].setting == setting){
      return(index);
    }
  }
  return(-1);
}

int Servo::getSetPointPreviousIndex(unsigned short setting){
  int index = setPointCount;
  while(index--){
    if(setPoints[index].setting < setting){
      return(index);
    }
  }
  return(-1);
}

bool Servo::setSetPoint(unsigned short setting, unsigned short adcValue){
  bool raiseError = true;
  int index = getSetPointIndex(setting);
  if(index != -1){
    setPoints[index].adcValue = adcValue;
    raiseError = false;
  }
  return raiseError;
}

unsigned short Servo::readAdc(void){
  adcValue = filter.input(analogRead(adcPin));
  if(adcValue < adcMinValue){
    adcMinValue = adcValue;
  }
  if(adcValue > adcMaxValue){
    adcMaxValue = adcValue;
  }
  return(adcValue);
}

unsigned short Servo::getAdcValue(void){
  return(adcValue);
}

unsigned short Servo::getAdcMinValue(void){
  return(adcMinValue);
}

unsigned short Servo::getAdcMaxValue(void){
  return(adcMaxValue);
}

void Servo::resetMinMax(void){
  adcMinValue = adcMaxValue = adcValue;
}

const char *Servo::getName(void){
  return szName;
}

int Servo::setMode(int newMode){
  switch(newMode){
    case MODE_ADC:
      mode = MODE_ADC;
      break;
    case MODE_TIMED_MOVE:
      mode = MODE_TIMED_MOVE;
    break;
    default:
    case MODE_DURATION:
      mode = MODE_DURATION;
    break;
  }
  return mode;
}

int Servo::getMode(void){
  return(mode);
}

void Servo::setDirection(bool dir){
  // Serial.printf("%s(%d)" "\n", __func__, dir);
  if(dir){
      direction = DIRECTION_FORWARD;
  } else {
      direction = DIRECTION_BACKWARD;
  }
}

bool Servo::setTimeMs(int t){
  bool raiseError = true;
  if(t > 0){
    mode = MODE_DURATION;
    remainingTimeMs = t + 1;
    analogWrite(pwmPin, pwmRatioMax);
    timeout = 0;
    // Serial.printf("%s(%d): trigger %sward motion for %d loop with pwmRatio %d" "\n", __func__, t, forward ? "for" : "back", remainingTimeMs, pwmRatioMax);
    raiseError = false;
  }
  return raiseError;
}

int Servo::getTimeMs(void){
  return remainingTimeMs;
}

unsigned int Servo::updatePWMRatio(void){
  // How far are we ?
  unsigned int absoluteDelta;
  int delta = targetAdcValue - adcValue;
  enum Direction_e oldDirection = direction;
  if(delta > 0){
    direction = DIRECTION_BACKWARD;
    absoluteDelta = (unsigned int)(delta);
  }else{
    direction = DIRECTION_FORWARD;
    absoluteDelta = (unsigned int)(-delta);
  }
  if(direction != oldDirection){
    digitalWrite(dirPin, direction);
  }
  // PWM is proportionnal to the remaining distance (in ADC steps)
  unsigned int newPwmRatio = (absoluteDelta / pwmScale);
  // User provided "speed limit"
  if(newPwmRatio > pwmRatioMax){
    newPwmRatio = pwmRatioMax;
  }
  if(newPwmRatio != pwmRatio){
    // Serial.printf("%s:: update PWM Ratio from %d to %d" "\n", szName, pwmRatio, newPwmRatio);
    pwmRatio = newPwmRatio;
    if(0 == pwmRatio){
      // We are "close enough" aka "in the window"
      stopMotor("WINDOW");
    }else{
      analogWrite(pwmPin, pwmRatio);
    }
  }
  return pwmRatio;
}

bool Servo::setDeltaAdc(int delta){
  unsigned int target = adcValue + delta;
  // Serial.printf("%s::%s(%d)=>target=%u" "\n", szName, __func__, delta, target);
  if(0 == delta){
    return false;
  }
  bool raiseError = false;
  if(isAdcTargetValid(target)){
    mode = MODE_ADC;
    targetAdcValue = target;
    updatePWMRatio();
    if(pwmRatio > 0){
      if(delta < 0){
        delta = -delta;
      }
      timeout = (delta * timeoutScale) / pwmRatio;
    }else{
      timeout = 1;
    }
    // Serial.printf("%s:timeout=%ums" "\n", szName, timeout);
  }else{
    raiseError = true;
  }
  return raiseError;
}

bool Servo::setTargetAdcValue(int value){
  // Serial.printf("%s(%d)" "\n", __func__, value);
  bool raiseError = false;
  if(isAdcTargetValid(value)){
    raiseError = setDeltaAdc(value - adcValue);
  }else{
    raiseError = true;
  }
  return raiseError;
}

/**
 * This is always called after a valid call to setTargetAdcValue() or setDeltaAdc(),
 * so targetAdcValue is our aim. Once its value is copied into timed_move_context.stopADC,
 * we need to set targetAdcValue to the current ADC value to start the process.
 * Successive calls to run() will update targetAdcValue, then the calls to runPWM() and updatePWMRatio()
 * should do their job just as if we were in MODE_ADC, except with a moving targetAdcValue
 */
bool Servo::timedMoveInit(uint32_t milliseconds){
  // Serial.printf("%s::%s(%dms)" "\n", szName, __func__, milliseconds);
  mode = MODE_TIMED_MOVE;
  if(targetAdcValue == adcValue){
    timed_move_context.complete = true;
  }else{
    timed_move_context.complete = false;
    timed_move_context.startADC = adcValue;
    timed_move_context.stopADC = targetAdcValue;
    timed_move_context.msIncrement = milliseconds;
    if (adcValue > targetAdcValue) {
      timed_move_context.adcIncrement = (adcValue - targetAdcValue);
      timed_move_context.targetADCIncrement = -1;
      direction = DIRECTION_FORWARD;
    }
    else {
      timed_move_context.adcIncrement = (targetAdcValue - adcValue);
      timed_move_context.targetADCIncrement = +1;
      direction = DIRECTION_BACKWARD;
    }
    timed_move_context.targetADC = (int32_t)adcValue;
    targetAdcValue = adcValue;
  }
  return(timed_move_context.complete);
}

void Servo::stopMotor(const char *szReason){
  (void)szReason;
  // Serial.printf("Stop motor %s on %s" "\n", szName, szReason);
  pwmRatio = 0;
  direction = DIRECTION_STOPPED;
  analogWrite(pwmPin, 0);
  digitalWrite(dirPin, 0);
  // Reset min/max
  resetMinMax();
  // Serial.printf("%s:%4d" "\n", getName(), getAdcValue());
}

int Servo::run(void){
  if(Servo::MODE_TIMED_MOVE == mode){
    if (false == timed_move_context.complete) {
      timed_move_context.decision += timed_move_context.adcIncrement;
      while (timed_move_context.decision >= timed_move_context.msIncrement) {
        timed_move_context.decision -= timed_move_context.msIncrement;
        timed_move_context.targetADC += timed_move_context.targetADCIncrement;
      }
      if (timed_move_context.targetADC == (int32_t)timed_move_context.stopADC) {
        timed_move_context.complete = true;
        stopMotor("TIMED_MOVE complete");
        // Create an absolute move to the requested position
        // This helps with precision of stop
        // and with unrealistic timings
        setTargetAdcValue(timed_move_context.stopADC);
      }else{
        targetAdcValue = timed_move_context.targetADC;
        updatePWMRatio();
      }
    }
    return(timed_move_context.complete);
  }else{
    if(timeout > 0){
      if(0 == --timeout){
          stopMotor("TIMEOUT");
          mode = Servo::MODE_DURATION;
      }
    }
    if(Servo::MODE_DURATION == mode){
      if(remainingTimeMs > 0){
        if(--remainingTimeMs == 0){
          stopMotor("TIME");
        }
      }
      return(remainingTimeMs);
    }else{
      updatePWMRatio();
      return(adcValue);
    }
  }
}

void Servo::setPins(int adc, int pwm, int dir, int dirPolarity){
  adcPin = adc;
  // Fill the noise filter
  int i = filter.getFilterLength();
  while(i--){
    adcValue = filter.input(analogRead(adcPin));
  }
  analogWriteResolution(8);
  analogWriteFrequency(10000);
  pwmPin = pwm; analogWrite(pwmPin, 16);
  dirPinPolarity = dirPolarity;
  dirPin = dir; digitalWrite(dirPin, dirPinPolarity); pinMode(dirPin, OUTPUT);
}

SetPoint *Servo::getFirstSetPoint(void){
  return(setPoints + 0);
}

SetPoint *Servo::getLastSetPoint(){
  if(setPointCount > 1){
    return(setPoints + (setPointCount - 1));
  }else{
    return(setPoints + 0);
  }
}

unsigned int Servo::setPwmRatioMax(unsigned int max){
  unsigned int oldPwmRatio = pwmRatio;
  if(max > PWM_RATIO_MAX){
    pwmRatioMax = PWM_RATIO_MAX;
  }else{
    pwmRatioMax = max;
  }
  if(pwmRatioMax < pwmRatioMin){
    pwmRatioMax = pwmRatioMin;
  }
  if(pwmRatio > pwmRatioMax){
    pwmRatio = pwmRatioMax;
  }
  if(oldPwmRatio != pwmRatio){
    analogWrite(pwmPin, pwmRatio);
  }
  return pwmRatioMax;
}

unsigned int Servo::setPwmRatioMin(unsigned int min){
  if(min > pwmRatioMax){
    pwmRatioMin = pwmRatioMax;
  }else{
    pwmRatioMin = min;
  }
  if(0 == pwmRatioMin){
    pwmRatioMin++;
  }
  return pwmRatioMin;
}

bool Servo::isAdcTargetValid(unsigned int adcValue){
  unsigned short min = getFirstSetPoint()->adcValue;
  unsigned short max = getLastSetPoint()->adcValue;
  bool returnValue = ((min <= adcValue) && (adcValue <= max)) || ((max <= adcValue) && (adcValue <= min));
  return returnValue;
}

bool Servo::isSettingValid(unsigned int setting){
  unsigned short min = getFirstSetPoint()->setting;
  unsigned short max = getLastSetPoint()->setting;
  bool returnValue = ((min <= setting) && (setting <= max)) || ((max <= setting) && (setting <= min));
  // Serial.printf("%s:%s(%d)=>%d" "\n", szName, __func__, setting, returnValue);
  return returnValue;
}

/*
 * .9 is valid, equivalent to 0.9
 * .0 is valid, equivalent to 0.0 (but should not occur)
 * 1. is valid, equivalent to 1.0
 */
bool Servo::stringToSetPointSetting(const char *start, int sLen, SetPoint *setPoint){
  bool raiseError = false;
  if((2 <= sLen) && (sLen <= 5)){
    unsigned int value = 0;
    const char *p = start;
    int i = sLen;
    int nonDigit = 0;
    bool doMultiply = true;
    while(i--){
      int c = *p++;
      if(doMultiply){
        value *= 10;
      }
      if(isdigit(c)){
        value += (c - '0');
      }else{
        doMultiply = false;
        nonDigit++;
      }
    }
    if(nonDigit != 1){
      raiseError = true;
    }else{
      setPoint->setting = value;
    }
  }else{
    raiseError = true;
  }
  return raiseError;
}

bool Servo::getAdcValueFromSetting(SetPoint *setPoint){
  bool raiseError = false;
  unsigned short setting = setPoint->setting;
  int index = getSetPointIndex(setting);
  if(-1 == index){
    if(isSettingValid(setting)){
      index = getSetPointPreviousIndex(setting);
      int beforeSetting  = (int)setPoints[index].setting;
      int beforeAdcValue = (int)setPoints[index].adcValue;
      int afterSetting   = (int)setPoints[index + 1].setting;
      int afterAdcValue  = (int)setPoints[index + 1].adcValue;
      // interpolate between 2 known settings
      setPoint->adcValue = (unsigned short)(beforeAdcValue + ((setting - beforeSetting) * (afterAdcValue - beforeAdcValue)) / (afterSetting - beforeSetting));
    }else{
      raiseError = true;
    }
  }else{
    setPoint->adcValue = setPoints[index].adcValue;
  }
  return raiseError;
}

bool Servo::setPwmScale(unsigned char scale){
  bool raiseError = false;
  if(scale > 0){
    pwmScale = (unsigned int)scale;
  }else{
    raiseError = true;
    pwmScale = 1;
  }
  return raiseError;
}

bool Servo::setTimeoutScale(unsigned char scale){
  bool raiseError = false;
  if(scale > 0){
    timeoutScale = (unsigned int)scale;
  }else{
    raiseError = true;
    timeoutScale = 1;
  }
  return raiseError;
}
