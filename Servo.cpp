#include <Arduino.h>
#include <EEPROM.h>

#include "Servo.hpp"
#include "Lens.hpp"


Servo zoomServo( &zoomSettings,  "ZOOM",  0 * sizeof(ServoSettings));
Servo irisServo( &irisSettings,  "IRIS",  1 * sizeof(ServoSettings));
Servo focusServo(&focusSettings, "FOCUS", 2 * sizeof(ServoSettings));


Servo::Servo(const ServoSettings *s, const char *name, unsigned int offset){
  servoSettingsFromFW = s;
  loadSettingsFromFW();
  eepromOffset = offset;
  loadSettingsFromEEPROM();
  szName = name;
  adcPin = -1;
  pwmPin = -1;
  dirPin = -1;
  dirPinPolarity = -1;
  pwmRatio.max = PWM_RATIO_HARD_LIMIT;
  pwmRatio.toUse = pwmRatio.max;
  pwmRatio.programmed = 0;
  filter = SlidingWindow(name, 4);
  direction = Servo::DIRECTION_STOPPED;
  mode = MOVE_MODE_NONE;
  open_loop_context.remainingTimeMs = 0;
  lastErrorString = NULL;
  pid.minOutput = -(float)PWM_RATIO_HARD_LIMIT;
  pid.maxOutput = +(float)PWM_RATIO_HARD_LIMIT;
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

// Order is complicated:
// - SetPoints are sorted by setting (i.e 1.5m focus, 30mm zoom or f/5.6)
// - ADC values can increase with increasing setting or just the opposite
// - "positive" polarity on motor driver can lead to either increase or decrease in ADC value
// Here, we just want a qucik way to test the "within" condition, so the lowest value is the lowest ADC value
// The test valid(adcValue) becomes (adcLowestValue <= adcValue) && (adcValue <= adcHighestValue)
void Servo::updateBoundaries(void){
  adcLowestValue = setPoints[0].adcValue;
  adcHighestValue = setPoints[setPointCount - 1].adcValue;
  if(adcHighestValue < adcLowestValue){
    // Swap value to guaranty adcLowestValue < adcHighestValue
    auto temp = adcLowestValue;
    adcLowestValue = adcHighestValue;
    adcHighestValue = temp;
  }
}

void Servo::loadSettingsFromMemory(const ServoSettings *settings){
  memset(setPoints, 0, sizeof(setPoints));
  setPointCount = 0;
  while((setPointCount < MAX_SET_POINTS) && (0 != settings->setPoints[setPointCount].adcValue)){
    setPoints[setPointCount] = settings->setPoints[setPointCount];
    setPointCount++;
  }
  pid.kP = settings->parameters.pidP;
  pid.kI = settings->parameters.pidI;
  pid.kD = settings->parameters.pidD;
  flags = settings->parameters.flags;
  updateBoundaries();
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
  while((count < MAX_SET_POINTS) && (0 != setPoints[count].adcValue)){
    temp.setPoints[count] = setPoints[count];
    count++;
  }
  temp.parameters.pidP = pid.kP;
  temp.parameters.pidI = pid.kI;
  temp.parameters.pidD = pid.kD;
  temp.parameters.flags = flags;
  
  EEPROM.put(eepromOffset, temp);
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
  stream->printf("ADC range=[%d .. %d]" "\n", adcLowestValue, adcHighestValue);
  char floatString[32];
  stream->printf("parameters={.kP=");
  dtostrf(pid.kP, 6, 3, floatString);
  stream->printf("%s, kI=", floatString);
  dtostrf(pid.kI, 6, 3, floatString);
  stream->printf("%s, kD=", floatString);
  dtostrf(pid.kD, 6, 3, floatString);
  stream->printf("%s, flags=0x%X}" "\n", floatString, flags);
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
    case MOVE_MODE_NONE:
      mode = MOVE_MODE_NONE;
      break;
    case MOVE_MODE_ADC:
      mode = MOVE_MODE_ADC;
      break;
    case MOVE_MODE_DURATION:
      mode = MOVE_MODE_DURATION;
    break;
    case MOVE_MODE_TIMED_MOVE:
      mode = MOVE_MODE_TIMED_MOVE;
    break;
    default:
    case MOVE_MODE_SPEED:
      mode = MOVE_MODE_SPEED;
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
  bool wasNull = (0 == open_loop_context.remainingTimeMs);
  open_loop_context.remainingTimeMs = t;
  return wasNull;
}

int Servo::getTimeMs(void){
  return open_loop_context.remainingTimeMs;
}

bool Servo::setDeltaAdc(int delta){
  unsigned int target = adcValue + delta;
  Serial.printf("%s::%s(%d)=>target=%u" "\n", szName, __func__, delta, target);
  if(0 == delta){
    return false;
  }
  bool raiseError = false;
  if(isAdcTargetValid(target)){
    mode = MOVE_MODE_ADC;
    targetAdcValue = target;
    if(pwmRatio.toUse > 0){
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
  mode = MOVE_MODE_TIMED_MOVE;
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
  Serial.printf("Stop motor %s on %s" "\n", szName, szReason);
  pwmRatio.programmed = 0;
  direction = DIRECTION_STOPPED;
  mode = MOVE_MODE_NONE;
  analogWrite(pwmPin, pwmRatio.programmed);
  digitalWrite(dirPin, 0);
}

void Servo::reset(const char *szReason){
  if(szReason != NULL){
    stopMotor(szReason);
  }else{
    stopMotor("reset");
  }
}

int Servo::everyMilliSecond(void){
  if(MOVE_MODE_NONE == mode){
    return(0);
  }
  if(MOVE_MODE_SPEED == mode){
    updateTarget();
    runPid();
  }
  if(MOVE_MODE_ADC == mode){
    runPid();
  }
  
  if(MOVE_MODE_DURATION == mode){
    open_loop_context.remainingTimeMs--;
    if(open_loop_context.remainingTimeMs <= 0){
      reset("end of duration");
    }else{
      uint32_t decision = 0;
      if(DIRECTION_FORWARD == direction){ // FWD/BWD
        decision++; // 8
      }
      decision <<= 1;
      if(FLAG_POSITIVE_DIRECTION & flags){ // NEG/POS
        decision++; // 4
      }
      decision <<= 1;
      if(adcHighestValue <= adcValue){ // Big
        decision++; // 2 
      }
      decision <<= 1;
      if(adcValue <= adcLowestValue){ // Low
        decision++; // 1
      }
      switch(decision){
        case 0x2:
            reset("BWD+NEG+Big");
        break;
        case 0xE:
            reset("FWD+POS+Big");
        break;
        case 0x5:
            reset("BWD+POS+Low");
        break;
        case 0x9:
            reset("FWD+NEG+Low");
        break;
        default:
        break;
      }
    }
  }
#if 0
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
#endif
  return(0);
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
  unsigned int oldMax = pwmRatio.max;
  if(max > PWM_RATIO_HARD_LIMIT){
    pwmRatio.max = PWM_RATIO_HARD_LIMIT;
  }else{
    pwmRatio.max = max;
  }
  if(pwmRatio.max != oldMax){
    // Serial.printf("%s::%s:pwmRatioMax != oldMax (%d != %d)" "\n", szName, __func__, pwmRatioMax, oldMax);
    if(0 != pwmRatio.programmed){
      // Serial.printf("%s::%s:running, set pwmRatio to %d" "\n", szName, __func__, pwmRatioMax);
      pwmRatio.programmed = pwmRatio.max; // what else ?
      analogWrite(pwmPin, pwmRatio.programmed);
    }
  }
  // Serial.printf("%s::%s(%d)", szName, __func__, max);
  // Serial.printf("=>%d" "\n", pwmRatioMax);
  pwmRatio.toUse = pwmRatio.max;
  return pwmRatio.max;
}

bool Servo::isAdcTargetValid(uint16_t adcValue){
  return((adcLowestValue <= adcValue) && (adcValue <= adcHighestValue));
}

bool Servo::isSettingValid(uint16_t setting){
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

bool Servo::updatePidTarget(uint16_t newTargetAdc){
#if SERVO_LOOP_DIVIDER > 1
  Serial.printf("%s(%d)" "\n", __func__, newTargetAdc);
#endif
  if((adcLowestValue <= newTargetAdc) && (newTargetAdc <= adcHighestValue)){
    pid.setpoint = (float)newTargetAdc;
#if SERVO_LOOP_DIVIDER > 1
  Serial.printf("pid.setpoint=%d", newTargetAdc);
#endif
    return true;
  }
  return false;
}

void Servo::runPid(void){
  float currentADCFloat = (float)getAdcValue();
  float timeStep = 0.001f;
#if SERVO_LOOP_DIVIDER > 1
  timeStep *= (float)SERVO_LOOP_DIVIDER;
#endif
  float diff = 0.0f;
  float pidOutput = pid.compute(currentADCFloat, timeStep, &diff);
  pidOutputToPWM(pidOutput, diff);
}

bool Servo::setSpeedAndDirection(int speed, int direction){
  reset("new Speed and Direction");
/*
    struct {
      uint32_t msBetweenTargetAdcIncrement; // Update target ADC every msBetweenTargetAdcIncrement millisecond ; for example once every 16 ms
      uint32_t msWaited;                    // current value trying to reach the above threshold
      int32_t  targetADC;                   // where we would like to be
      int32_t  targetADCIncrement;          // +1/-1
    } speed_move_context;
*/

  bool raiseError = false;
  speed_move_context.msBetweenTargetAdcIncrement = 1 << (8 - speed); // 8 => (1 << 0) => 1, 7 => (1 << 1) => 2, .. , 1 => (1 << 7) => 128
  speed_move_context.msWaited = 0;

  int increment = -1;  
  if(DIRECTION_BACKWARD == direction){
    increment = +1;
  }
  if(0 == (flags & FLAG_POSITIVE_DIRECTION)){
    increment = -increment;
  }
  speed_move_context.targetADCIncrement = increment;
  speed_move_context.targetADC = getAdcValue();

  pid.reset();
  mode = MOVE_MODE_SPEED;
  updateTarget();
  runPid();
  return(raiseError);
}

static bool isValidPIDParameterValue(float value){
  return((-127.0 <= value) && (value <= 127.0));
}

bool Servo::setKP(float value){
  bool raiseError = true;
  if(isValidPIDParameterValue(value)){
    pid.kP = value;
    raiseError = false;
  }
  char floatAsString[32];
  dtostrf(value, 6, 3, floatAsString);
  Serial.printf("%s(%s)=>%d" "\n", __func__, floatAsString, raiseError);
  return(raiseError);
}

bool Servo::setKI(float value){
  bool raiseError = true;
  if(isValidPIDParameterValue(value)){
    pid.kI = value;
    raiseError = false;
  }
  char floatAsString[32];
  dtostrf(value, 6, 3, floatAsString);
  Serial.printf("%s(%s)=>%d" "\n", __func__, floatAsString, raiseError);
  return(raiseError);
}

bool Servo::setKD(float value){
  bool raiseError = true;
  if(isValidPIDParameterValue(value)){
    pid.kD = value;
    raiseError = false;
  }
  char floatAsString[32];
  dtostrf(value, 6, 3, floatAsString);
  Serial.printf("%s(%s)=>%d" "\n", __func__, floatAsString, raiseError);
  return(raiseError);
}

bool Servo::updateTarget(void){
  if(MOVE_MODE_SPEED == mode){
#if SERVO_LOOP_DIVIDER > 1
    Serial.printf("%s():SPEED,msB=%d,msW=%d,tADC=%d=>", __func__, speed_move_context.msBetweenTargetAdcIncrement, speed_move_context.msWaited, speed_move_context.targetADC);
#endif
    if(
      ((speed_move_context.targetADCIncrement > 0) && (speed_move_context.targetADC >= adcHighestValue)) || 
      ((speed_move_context.targetADCIncrement < 0) && (speed_move_context.targetADC <= adcLowestValue))
    ){
      speed_move_context.targetADCIncrement = 0;
    }
    if(0 != speed_move_context.targetADCIncrement){
      speed_move_context.msWaited++;
      if(speed_move_context.msBetweenTargetAdcIncrement == speed_move_context.msWaited){
        speed_move_context.msWaited = 0;
        speed_move_context.targetADC += speed_move_context.targetADCIncrement;
#if SERVO_LOOP_DIVIDER > 1
        Serial.printf("%s():SPEED,targetADC is now %d" "\n", __func__, speed_move_context.targetADC);
#endif
        updatePidTarget(speed_move_context.targetADC);
      }
    }
#if SERVO_LOOP_DIVIDER > 1
    else{
      Serial.printf("targetADCIncrement == 0,");
    }
#endif

#if SERVO_LOOP_DIVIDER > 1
    Serial.printf("msB=%d,msW=%d,tADC=%d" "\n", speed_move_context.msBetweenTargetAdcIncrement, speed_move_context.msWaited, speed_move_context.targetADC);
#endif
  }
  return(false);
}

bool Servo::programOpenLoopMove(uint32_t durationMillisecond, int direction, int32_t pwm){
  reset("new open-loop move");
  bool raiseError = true;
  if(durationMillisecond > 0){
    mode = MOVE_MODE_DURATION;
    open_loop_context.remainingTimeMs = durationMillisecond;
    this->direction = direction;

    int dir = dirPinPolarity;
    if(DIRECTION_BACKWARD == direction){
      dir ^= 1;
    }
    if((0 < pwm) && (pwm <= PWM_RATIO_HARD_LIMIT)){
      pwmRatio.toUse = (uint32_t)pwm;
    }
    pwmRatio.programmed = pwmRatio.toUse;
    digitalWrite(dirPin, dir);
    analogWrite(pwmPin, pwmRatio.programmed);
    raiseError = false;
  }
  return raiseError;
}

void Servo::pidOutputToPWM(float pidOutput, float diff){
  if(diff < 0.0f){
    diff = -diff;
  }
  if(diff < 3.0f){
    if(MOVE_MODE_SPEED == mode){
    }else{
      reset("Target ADC reached (diff < 3)");
    }
  }else{
    int dir = dirPinPolarity;
    if(pidOutput < 0.0f){
      dir ^= 1;
      pidOutput = -pidOutput;
    }
    uint16_t pwm = (uint16_t)(pidOutput + 0);
    if(pwm > pwmRatio.toUse){
      pwm = pwmRatio.toUse;
    }
    pwmRatio.programmed = pwm;
    digitalWrite(dirPin, dir);
    analogWrite(pwmPin, pwmRatio.programmed);
  }
}

bool Servo::programTargetADCMove(uint16_t targetADC, uint32_t pwmSetting, uint32_t moveTimeMillisecond){
  (void)targetADC;
  (void)pwmSetting;
  (void)moveTimeMillisecond;

  bool raiseError = false;

  if(isAdcTargetValid(targetADC)){
    reset("new target-ADC move");
    pid.reset();
    mode = MOVE_MODE_ADC;
    updatePidTarget(targetADC);
    runPid();
  }else{
    setLastErrorString("programTargetADCMove(invalid target ADC)");
    raiseError = true;
  }
  return(raiseError);
}
void Servo::setLastErrorString(const char *szString){
  lastErrorString = szString;
}

void Servo::printLastErrorString(Stream *stream){
  if((stream != NULL) && (lastErrorString != NULL)){
    stream->printf("\r\n" "%s" "\r\n", lastErrorString);
  }
}
