#include <Arduino.h>
#include "Servo.hpp"
#include "AtCommand.hpp"
#include "GlobalConfiguration.hpp"

static int countComas(const char *szString, int length){
  int count = 0;
  const char *p = szString;
  while(length--){
    if(',' == *p++){
      count++;
    }
  }
  // Serial.printf("%s(\"%s\")=>%d" "\n", __func__, szString, count);
  return(count);
}

typedef bool (*plusSubFunction)(Servo *servo, const char c, const char *szString, int comas);
static const char *useATW =  "(use AT&W to make the setting persistent)";

static bool plusUsage(Servo *servo, const char c, const char *szString, int comas){
  (void)servo;
  (void)c;
  (void)szString;
  (void)comas;
  return(false);
}

static bool plusRead(Servo *servo, const char c, const char *szString, int comas){
  (void)szString;
  (void)comas;
  (void)c;
  Serial.printf("%s: [%4d .. %4d .. %4d], .P=%3d, .T=%3d, .M=%2d" "\n", 
      servo->getName(),
      servo->getAdcMinValue(), servo->getAdcValue(), servo->getAdcMaxValue(),
      servo->getPwmScale(), servo->getTimeoutScale(), servo->getPwmRatioMin()
  );
  return(false);
}

/*
 * Servo write command. Several syntaxes allowed, with X in Z (for zoom), I (for iris) or F (for focus):
 * AT+X=ddd.d      => move X axis to (possibly interpolated) setPoint <ddd.d> ; syntax is flexible, but a dot is required and at least one digit
 * Example:
 * AT+Z=5.5
 * AT+Z=6.4
 * AT+Z=1.
 * AT+F=.9
 * AT+X=dddd        => move X axis to the position where ADC is <dddd>
 * Example:
 * AT+Z=1800
 * AT+X={+|-}dddd   => move X axis to the position currentADC + delta <dddd>
 * Example:
 * AT+Z=+110
 * AT+Z=-100
 * AT+X={+|-}dddM   => move X axis for ddd milliseconds, backward or forward
 * AT+F=+40m
 * AT+F=-100m
 * All the above syntaxes allow for an optional parameter, the maximum speed, in the range [1..16].
 * The speed setting will remaing active until changed
 * Notice, however, that some axis might not support the lowest speeds, but don't report any error in that case.
 * Example:
 * AT+Z=-100m,1     => will move zoom at the slowest possible speed
 * This optional parameter can also be used without specifying any move:
 * AT+Z=,3          => change the speed for future moves, but don't move
 *
 * Starting with v3.0.1, a new syntax for the speed allows to use it at a move duration request.
 * This is obtained be adding 's' after the speed definition ; this second parameter turns into a duration.
 * The duration, in second as the unit implies, is parsed as a floating point number.
 * This new mode is called TIMED MOVE (old TIME MOVE is renamed DURATION MOVE, a more precise terminology)
 * The servo will try to reach the requested position in the given time, respecting the maximum speed.
 * This is done through running a "classical" MODE_ADC equivalent, but with a moving targetAdcValue.
 * If the duration is unrealistic, the move will not be complete, so an absolute move to the target ADC is automatically inserted after a timed move.
 * This syntax also allows synchronized moves (if requested durations are realistic, of course).
 * For example:
 *
 * AT+Z=48.,2.3s;+F=10.,2.3s
 *
 * should reach 48mm zoom and 10m focus at the same time, in about 2.3s.
 *
 * Programming syntax for SetPoints
 * AT+X=ddd.d,,
 * AT+X=ddd.d,,dddd
 * Examples:
 * AT+I=5.6,,     set the adcValue for setting 5.6 to the current adcValue for iris
 * AT+I=5.6,,2300 set the adcValue for setting 5.6 to 2300 for iris
 *
 * Programming syntax for parameters
 * AT+X=Y,ddd
 * Examples:
 * AT+I=P,2     set the pwmScale to 2 for iris
 * AT+I=M,4     set the minimum speed to 4 (over 15) for iris
 * AT+I=T,64    set the timeout factor to 64 for iris
 *
 * For all programming commands, AT&W is still required for the setting to be stored in Flash and survive power-cycle
 *
 */
static bool getParameterValueFromString(const char *szString, unsigned char *value){
  bool raiseError = true;
  if(',' == szString[0]){
    const char *nptr = szString + 1;
    char *end = NULL;
    float floatValue = strtof(nptr, &end);
    if(end != nptr){
      if(NULL != value){
        *value = (unsigned char)floatValue;
      }
      raiseError = false;
    }
  }
  // Serial.printf("%s(\"%s\")=>%d" "\n", __func__, szString, raiseError);
  return(raiseError);
}

static bool plusWrite(Servo *servo, const char c, const char *szString, int comas){
  // Serial.printf("%s(%c, \"%s\", %d)" "\n", __func__, c, szString, comas);
  (void)c;
  bool raiseError = false;
  if(NULL != servo){
    float secondValue = 0.0f;
    bool hasSecondValue = false;

    char first = szString[3];
    if('R' == first){
      servo->resetMinMax();
    }else if('P' == first){
      // pwmScale parameter
      unsigned char value = 0;
      if(getParameterValueFromString(szString + 4, &value)){
        raiseError = true;
      }else{
        raiseError = servo->setPwmScale(value);
      }
    }else if('T' == first){
      // timeoutScale parameter
      unsigned char value = 0;
      if(getParameterValueFromString(szString + 4, &value)){
        raiseError = true;
      }else{
        raiseError = servo->setTimeoutScale(value);
      }
    }else if('M' == first){
      // minSpeed parameter
      unsigned char value = 0;
      if(getParameterValueFromString(szString + 4, &value)){
        raiseError = true;
      }else{
        raiseError = (servo->setPwmRatioMin(value) != value);
      }
    }else{
      int offset = 3; // skip +X=
      int sign = 0;
      if('-' == first){
        offset++;
        sign = -1;
      }else if('+' == first){
        offset++;
        sign = +1;
      }
      char *end = NULL;
      const char *nptr = szString + offset;
      float firstValue = strtof(nptr, &end);
      bool hasFirstValue = (end != nptr);
      int mode = Servo::MODE_ADC;
      bool parameterIsSetting = false;
      SetPoint setPoint = {0, 0};
      if(hasFirstValue){
        if('M' == *end){
          mode = Servo::MODE_DURATION;
        }else{
          int l = (end - nptr);
          const char *dot = (const char *)memchr(nptr, '.', l);
          if(NULL != dot){
            if(Servo::stringToSetPointSetting(nptr, l, &setPoint)){
              raiseError = true;
            }else{
              if(servo->getAdcValueFromSetting(&setPoint)){
                raiseError = true;
              }else{
                parameterIsSetting = true;
              }
            }
          }
        }
      }else{
        if(',' != nptr[0]){
          // First parameter is garbage: it should be either a valid number or empty
          raiseError = true;
        }
      }
      if(!raiseError){
        if(comas >= 1){
          char *p = strchr(szString + offset, ',');
          nptr = p + 1;
          secondValue = strtof(nptr, &end);
          hasSecondValue = (end != nptr);
        }
        if(hasSecondValue){
          if('S' == *end){
            // v3.0.1+: TIMED MOVE
            mode = Servo::MODE_TIMED_MOVE;
          }else{
            unsigned int max = (unsigned int)secondValue;
            if((0 < max) && (max <= PWM_RATIO_MAX)){
              servo->setPwmRatioMax(max);
            }else{
              raiseError = true;
            }
          }
        }
      }
      if(!raiseError && parameterIsSetting && (2 == comas)){
        // AT+I=5.6,,     set the adcValue for setting 5.6 to the current adcValue for iris
        // AT+I=5.6,,2300 set the adcValue for setting 5.6 to 2300
        // AT&W is still required for the setting to be stored in Flash
        char *lastComa = strrchr(szString, ',');
        nptr = lastComa + 1;
        if('\0' == lastComa[1]){
          raiseError = servo->setSetPoint(setPoint.setting, servo->getAdcValue());
          Serial.printf("%s: setting %d with current adcValue %d instead of %d => %d %s" "\n", servo->getName(), setPoint.setting, servo->getAdcValue(), setPoint.adcValue, raiseError, useATW);
        }else{
          nptr = lastComa + 1;
          float thirdValue = strtof(nptr, &end);
          if(end != nptr){
            raiseError = servo->setSetPoint(setPoint.setting, (unsigned short)thirdValue);
            Serial.printf("%s: setting %d with provided adcValue %d instead of %d => %d %s" "\n", servo->getName(), setPoint.setting, (unsigned short)thirdValue, setPoint.adcValue, raiseError, useATW);
          }else{
            raiseError = true;
          }
        }
      }else{
        if(!raiseError && hasFirstValue){
          if(sign != 0){
            if(Servo::MODE_DURATION == mode){
              int ms = (int)firstValue;
              // Serial.printf("%s(%s): duration move for %s: %d ms" "\n", __func__, szString, servo->getName(), ms);
              servo->setDirection(sign == +1);
              raiseError = servo->setTimeMs(ms);
            }else{
              int delta = sign * (int)firstValue;
              // Serial.printf("%s(%s): delta ADC mode for %s: %d step(s)" "\n", __func__, szString, servo->getName(), delta);
              raiseError = servo->setDeltaAdc(delta);
            }
          }else{
            if(parameterIsSetting){
              // Serial.printf("%s(%s): setting mode for %s: %d" "\n", __func__, szString, servo->getName(), setPoint.setting);
              raiseError = servo->setTargetAdcValue(setPoint.adcValue);
            }else{
              int position = (int)firstValue;
              // Serial.printf("%s(%s): absolute ADC mode for %s: %d" "\n", __func__, szString, servo->getName(), position);
              raiseError = servo->setTargetAdcValue(position);
            }
          }
          if(!raiseError && Servo::MODE_TIMED_MOVE == mode){
            uint32_t duration = (uint32_t)(secondValue* 1000.0f);
            servo->timedMoveInit(duration);
          }
        }
      }
    }
  }
  return(raiseError);
}

bool handlePlus(const char *szString, int length) {
  bool raiseError = true;
  int comas = countComas(szString, length);
  int commandLength = length;
  char axis = '\0';
  bool isEqual = false;
  Servo *servo = NULL;
  plusSubFunction sub = NULL;
  if(commandLength > 1){
    axis = szString[1];
    if(commandLength > 2){
      if('?' == szString[2]){
        sub = plusRead;
      }else{
        isEqual = ('=' == szString[2]);
      }
      if((commandLength > 3) && (isEqual)){
        if('?' == szString[3]){
          sub = plusUsage;
        }else{
          sub = plusWrite;
        }
      }
    }
  }
  if(sub){
    servo = getServo(axis);
    if(servo){
      raiseError = sub(servo, axis, szString, comas);
    }
  }
  // Serial.printf("%s(\"%s\")=>%d" "\n", __func__, szString, raiseError);
  return (raiseError);
}
