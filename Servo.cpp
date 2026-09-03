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
  pid_context.kP = (float)(settings->parameters.pidP / 256.0);
  pid_context.kI = (float)(settings->parameters.pidI / 256.0);
  pid_context.kD = (float)(settings->parameters.pidD / 256.0);
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
  temp.parameters.pidP = getKP();
  temp.parameters.pidI = getKI();
  temp.parameters.pidD = getKD();
  temp.parameters.rfu = 0;
  
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
  pwmRatioMax = 0xC0; // 8-bit PWM, but beyond 0xC0, the behaviour is not predictable
  filter = SlidingWindow(name, 4);
  direction = Servo::DIRECTION_STOPPED;
}

void Servo::setPins(int adc, int pwm, int dir, int dirPolarity){
  adcPin = adc;
  #if 0
  // Fill the noise filter
  int i = filter.getFilterLength();
  while(i--){
    adcValue = filter.input(analogRead(adcPin));
  }
  #endif
  analogWriteResolution(8);
  analogWriteFrequency(16000);
  pwmPin = pwm; analogWrite(pwmPin, 0);
  dirPinPolarity = dirPolarity;
  dirPin = dir; digitalWrite(dirPin, dirPinPolarity); pinMode(dirPin, OUTPUT);
}

void Servo::print(Stream *stream, const char *szUnit){
  for(int i = 0 ; i < setPointCount ; i++){
    char setPointSettingString[8];
    setPointSettingToString(setPointSettingString, setPoints + i);
    stream->printf("setPoints[%2d]={%5s%2s, %4d steps}" "\n",
        i,
        setPointSettingString,
        szUnit,
        setPoints[i].adcValue
        );
    delay(40);
  }
  char floatString[32];
  stream->printf("parameters={.kP=");
  dtostrf(pid_context.kP, 6, 3, floatString);
  stream->printf("%s, kI=", floatString);
  dtostrf(pid_context.kI, 6, 3, floatString);
  stream->printf("%s, kD=", floatString);
  dtostrf(pid_context.kD, 6, 3, floatString);
  stream->printf("%s}" "\n", floatString);
  filter.print(stream);
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

int Servo::getSetPointIndexFromSetting(unsigned short setting){
  int index = setPointCount;
  while(index--){
    if(setPoints[index].setting == setting){
      return(index);
    }
  }
  return(-1);
}

int Servo::getSetPointPreviousIndexFromSetting(unsigned short setting){
  int index = setPointCount;
  while(index--){
    if(setPoints[index].setting < setting){
      return(index);
    }
  }
  return(-1);
}

int Servo::getSetPointPreviousIndexFromAdc(unsigned short adcValue){
  // Serial.printf("%s(%d)" "\n", __func__, adcValue);
  unsigned short min = getFirstSetPoint()->adcValue;
  unsigned short max = getLastSetPoint()->adcValue;
  if(min < max){
    // Ascending ADC values
    int index = setPointCount;
    while(index--){
      // Serial.printf("ASC:setPoints[%d].adcValue=%d" "\n", index, setPoints[index].adcValue);
      if(setPoints[index].adcValue < adcValue){
        return(index);
      }
    }
  }else{
    // Descending ADC values
    int index = setPointCount;
    while(index--){
      // Serial.printf("DES:setPoints[%d].adcValue=%d" "\n", index, setPoints[index].adcValue);
      if(setPoints[index].adcValue > adcValue){
        return(index);
      }
    }
  }
  return(-1);
}
bool Servo::setSetPoint(unsigned short setting, unsigned short adcValue){
  bool raiseError = true;
  int index = getSetPointIndexFromSetting(setting);
  if(index != -1){
    setPoints[index].adcValue = adcValue;
    raiseError = false;
  }
  return raiseError;
}

unsigned short Servo::readAdc(void){
  unsigned short newAdcValue = analogRead(adcPin);
  adcValue = filter.input(newAdcValue);
  return(adcValue);
}

unsigned short Servo::getAdcValue(void){
  return(adcValue);
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
  // Serial.printf("%s(%d)=>", __func__, dir);
  if(dir){
    // Serial.printf("DIRECTION_FORWARD");
    direction = DIRECTION_FORWARD;
  } else {
    // Serial.printf("DIRECTION_BACKWARD");
    direction = DIRECTION_BACKWARD;
  }
  // Serial.printf("\n");
}

bool Servo::setTimeMs(int t){
  bool raiseError = true;
  if(t > 0){
    mode = MODE_DURATION;
    remainingTimeMs = t + 1;

    int dir = dirPinPolarity;
    if(DIRECTION_BACKWARD == direction){
      dir ^= 1;
    }
    digitalWrite(dirPin, dir);
    pwmRatio = pwmRatioMax;
    analogWrite(pwmPin, pwmRatio);
    raiseError = false;
  }
  return raiseError;
}

int Servo::getTimeMs(void){
  return remainingTimeMs;
}

bool Servo::setDeltaAdc(int delta){
  unsigned int target = adcValue + delta;
  Serial.printf("%s::%s(%d)=>target=%u" "\n", szName, __func__, delta, target);
  if(0 == delta){
    return false;
  }
  bool raiseError = false;
  if(isAdcTargetValid(target)){
    mode = MODE_ADC;
    targetAdcValue = target;
    if(pwmRatio > 0){
      if(delta < 0){
        delta = -delta;
      }
    }
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
  // Serial.printf("%s:%4d" "\n", getName(), getAdcValue());
}

int Servo::everyMilliSecond(void){
  // Serial.printf("%s::run()" "\n", getName());
  updateTarget();
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
      }
    }
    return(timed_move_context.complete);
  }else{
    if(Servo::MODE_DURATION == mode){
      if(remainingTimeMs > 0){
        if(--remainingTimeMs == 0){
          stopMotor("TIME");
        }
      }
      return(remainingTimeMs);
    }else{
      return(adcValue);
    }
  }
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
  unsigned int oldMax = pwmRatioMax;
  if(max > PWM_RATIO_HARD_LIMIT){
    pwmRatioMax = PWM_RATIO_HARD_LIMIT;
  }else{
    pwmRatioMax = max;
  }
  if(pwmRatioMax != oldMax){
    // Serial.printf("%s::%s:pwmRatioMax != oldMax (%d != %d)" "\n", szName, __func__, pwmRatioMax, oldMax);
    if(0 != pwmRatio){
      // Serial.printf("%s::%s:running, set pwmRatio to %d" "\n", szName, __func__, pwmRatioMax);
      pwmRatio = pwmRatioMax; // what else ?
    }
  }
  if(0 != pwmRatio){ // Only if we are running, no matter the value has changed or not
    // Serial.printf("%s::%s:running, analogWrite(pwmPin=%d, pwmRatio=%d);" "\n", szName, __func__, pwmPin, pwmRatio);
    analogWrite(pwmPin, pwmRatio);
  }
  // Serial.printf("%s::%s(%d)", szName, __func__, max);
  // Serial.printf("=>%d" "\n", pwmRatioMax);
  return pwmRatioMax;
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
  int index = getSetPointIndexFromSetting(setting);
  if(-1 == index){
    if(isSettingValid(setting)){
      index = getSetPointPreviousIndexFromSetting(setting);
      int beforeSetting  = (int)setPoints[index].setting;
      int beforeAdcValue = (int)setPoints[index].adcValue;
      int afterSetting   = (int)setPoints[index + 1].setting;
      int afterAdcValue  = (int)setPoints[index + 1].adcValue;
      // linear interpolation between 2 known settings
      setPoint->adcValue = (unsigned short)(beforeAdcValue + ((setting - beforeSetting) * (afterAdcValue - beforeAdcValue)) / (afterSetting - beforeSetting));
    }else{
      raiseError = true;
    }
  }else{
    setPoint->adcValue = setPoints[index].adcValue;
  }
  return raiseError;
}

// find closest SetPoint index for a given ADC value
int Servo::getClosestSettingIndexFromAdcValue(unsigned short adc){
  int index  = getSetPointPreviousIndexFromAdc(adc);
  if(-1 == index){
    return(0);
  }else{
    int beforeAdcValue = (int)setPoints[index].adcValue;
    int afterAdcValue  = (int)setPoints[index + 1].adcValue;
    int deltaBefore = adc - beforeAdcValue;
    if(deltaBefore < 0){
      deltaBefore = -deltaBefore;
    }
    int deltaAfter = afterAdcValue - adc;
    if(deltaAfter < 0){
      deltaAfter = -deltaAfter;
    }
    // Serial.printf("%s:index=%d, deltaBefore %d, deltaAfter %d" "\n", __func__, index, deltaBefore, deltaAfter);
    if(deltaBefore < deltaAfter){
      return(index);
    }else{
      return(index + 1);
    }
  }
}

bool Servo::runPID(void){
  
  return(false);
}

bool Servo::setSpeedAndDirection(int speed, int direction){
  (void)speed;
  (void)direction;
  return(false);
}

int16_t Servo::getKP(void){
  return((int16_t)(pid_context.kP * 256.0));
}

int16_t Servo::getKI(void){
  return((int16_t)(pid_context.kI * 256.0));
}

int16_t Servo::getKD(void){
  return((int16_t)(pid_context.kD * 256.0));
}

static bool isValidPIDParameterValue(float value){
  return((127.0 <= value) && (value <= 127.0));
}

bool Servo::setKP(float value){
  bool raiseError = true;
  if(isValidPIDParameterValue(value)){
    pid_context.kP = value;
    raiseError = false;
  }
  return(raiseError);
}

bool Servo::setKI(float value){
  bool raiseError = true;
  if(isValidPIDParameterValue(value)){
    pid_context.kI = value;
    raiseError = false;
  }
  return(raiseError);
}

bool Servo::setKD(float value){
  bool raiseError = true;
  if(isValidPIDParameterValue(value)){
    pid_context.kD = value;
    raiseError = false;
  }
  return(raiseError);
}

bool Servo::updateTarget(void){
  return(false);
}
