#include <Arduino.h>
#include "Servo.hpp"
#include "Extender.hpp"
#include "AtCommand.hpp"
#include "GlobalConfiguration.hpp"

#define MAX_PARAMETER_COUNT (4)

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

typedef bool (*plusSubFunction)(Stream *stream, Servo *servo, const char c, const char *szString, int comas);
static const char *useATW =  "(use AT&W to make the setting persistent)";

static bool plusUsage(Stream *stream, Servo *servo, const char c, const char *szString, int comas){
  (void)stream;
  (void)servo;
  (void)c;
  (void)szString;
  (void)comas;
  return(false);
}

static bool plusRead(Stream *stream, Servo *servo, const char c, const char *szString, int comas){
  (void)szString;
  (void)comas;
  (void)c;
  stream->printf("%s: [adc=%4d], .kP=%+3d, .kI=%+3d, .kD=%+3d" "\n", 
      servo->getName(),
      servo->getAdcValue(),
      servo->getKP(), servo->getKI(), servo->getKD()
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
 * Programming syntax for PID parameters
 * AT+X=Y,ddd
 * Examples:
 * AT+I=P,22.3  set the proportional coefficient to 22.3
 * AT+I=I,0.3   set the integral coefficient to 0.3
 * AT+I=D,-5.2  set the derivative coefficient to -5.2
 *
 * For all programming commands (SetPoints or PID parameters), AT&W is still required for the setting to be stored in Flash and survive power-cycle
 *
 */

typedef struct {
  const char *stringStart;
  int sign;
  int count;
  float value;
  bool hasDot;
  char nextChar;
} NumericalValue_t;

/**
 * static bool getNumericalValue(const char *szString, int *sign, float *value, char *nextChar);
 * breakout the given string into the following values:
 * - an optional sign: if the first char is '-', then *sign=-1, else if the first char is '+', then *sign=1, else *sign=0
 * - a float value
 * - the first char that is not in the float value ; this can be a ',' if there is another parameter, a '\0' is its the last value, or a unit ('S' for second or 'M' for millisecond)
 * - hasDot tells if a dot was found within the float value string
 * - return true if a float value could be parsed.
 */
static bool getNumericalValue(const char *szString, NumericalValue_t *value){
  value->stringStart = szString;
  int offset = 0;
  char firstChar = szString[offset];
  if('-' == firstChar){
    value->sign = -1;
    offset++;
  }else if ('+' == firstChar){
    offset++;
    value->sign = +1;
  }else{
    value->sign = 0;
  }
  char *endPtr = NULL;
  const char *startPtr = szString + offset;
  value->value = strtof(startPtr, &endPtr);
  value->nextChar = *endPtr;
  value->count = (endPtr - startPtr);
  if(value->count > 0){
    value->hasDot = (NULL != memchr(startPtr, '.', value->count));
  }else{
    value->hasDot = false;
  }
  return(value->count > 0);
}

static bool getPIDParameter(const char *szString, float *value){
  NumericalValue_t number;
  if(getNumericalValue(szString, &number)){
    *value = number.value;
    return(true);
  }
  return(false);
}

static bool plusWrite(Stream *stream, Servo *servo, const char c, const char *szString, int numberOfComas){
  // Serial.printf("%s(%c, \"%s\", %d)" "\n", __func__, c, szString, comas);
  (void)c;
  (void)stream;
  bool raiseError = false;
  int offset = 3; // skip +X=
  char first = szString[offset];
  if((1 == numberOfComas) && ('P' == first)){
    // kP parameter
    float value;
    if(getPIDParameter(szString + offset, &value)){
      raiseError = servo->setKP(value);
    }
  }else if((1 == numberOfComas) && ('I' == first)){
    // kI parameter
    float value = 0.0;
    if(getPIDParameter(szString + offset, &value)){
      raiseError = servo->setKI(value);
    }
  }else if((1 == numberOfComas) && ('D' == first)){
    // kD parameter
    float value = 0.0;
    if(getPIDParameter(szString + offset, &value)){
      raiseError = servo->setKD(value);
    }
  }else{
    // Process as many values as we have: (numberOfComas+1), but limited to 4
    NumericalValue_t values[MAX_PARAMETER_COUNT];
    int parameterCount = numberOfComas + 1;
    if(parameterCount > MAX_PARAMETER_COUNT){
      parameterCount = MAX_PARAMETER_COUNT;
    }
    int i = 0;
    const char *start = szString + offset;
    while(i < parameterCount){
      getNumericalValue(start, values + i);
      const char *comaPtr = strchr(start, ',');
      if(comaPtr != NULL){
          start = (comaPtr + 1);
      }else{
        break;
      }
      i++;
    }
    int32_t pwmSetting = -1;
    int32_t moveTimeMillisecond = -1;
    if(1 == numberOfComas){
      if(values[1].count > 0){
        if('S' == values[1].nextChar){
          // "timed" move
          moveTimeMillisecond = (int32_t)(1000.0 * values[1].value);
        }else{
          pwmSetting = (int32_t)(values[1].value);
          if((pwmSetting < 0) || (pwmSetting > Servo::PWM_RATIO_HARD_LIMIT)){
            return(true);
          }
        }
      }
    }
    // Time to take a decision ...
    // The first value should allow to distinguish between most of the case
    if(values[0].count > 0){
      // We have a number
      // Is it a SetPoint ?
      if(values[0].hasDot){
        SetPoint setPoint = {0, 0};
        Serial.printf("starts with a SetPoint, ");
        if(Servo::stringToSetPointSetting(values[0].stringStart, values[0].count, &setPoint)){
          servo->setLastErrorString("SetPoint the syntax is not correct.");
          return(true);
        }else{
          if(servo->getAdcValueFromSetting(&setPoint)){
            servo->setLastErrorString("SetPoint not valid.");
            return(true);
          }else{
            if(2 == numberOfComas){
              // It's a write of a new setting for SetPoints[index]
              // Is there a value in parameter 3 ?
              if(values[2].count > 0){
                // Syntax: AT+X=5.6,,1234
                // Use the provided value for the adcValue of this SetPoint
                raiseError = servo->setSetPoint(setPoint.setting, (unsigned short)(values[2].value));
                if(!raiseError){
                  stream->printf("%s: setting %d with provided adcValue %d instead of %d, %s" "\n", servo->getName(), setPoint.setting, (unsigned short)(values[2].value), setPoint.adcValue, useATW);
                }
              }else{
                // Syntax: AT+X=5.6,,
                // Use the current ADC value for the adcValue of this SetPoint
                raiseError = servo->setSetPoint(setPoint.setting, servo->getAdcValue());
                if(!raiseError){
                  stream->printf("%s: setting %d with current adcValue %d instead of %d, %s" "\n", servo->getName(), setPoint.setting, servo->getAdcValue(), setPoint.adcValue, useATW);
                }
              }
            }else{
              servo->programTargetADCMove(setPoint.adcValue, pwmSetting, moveTimeMillisecond);
            }
          }
        }
      }else{
        uint32_t uintValue = (uint32_t)values[0].value;
        // Do we have a sign ?
        if(0 == values[0].sign){
          // Absolute value
          // Is there a sign after the value
          char dirChar = values[0].nextChar;
          int signAfter = Servo::DIRECTION_STOPPED;
          if('-' == dirChar){
            signAfter = Servo::DIRECTION_BACKWARD;
          }
          if('+' == dirChar){
            signAfter = Servo::DIRECTION_FORWARD;
          }
          if(Servo::DIRECTION_STOPPED != signAfter){
            // Syntax: AT+X=<speed>{+-}
            if((0 < uintValue) && (uintValue <= (uint32_t)8)){
              Serial.printf("Request speed %d with direction %+d" "\n", uintValue, signAfter);
            }else{
              raiseError = true;
              servo->setLastErrorString("invalid speed, must be [1 .. 8]");
            }
          }else{
            if(0 != uintValue){
              // Syntax: AT+X=<target adc value>
              // Syntax: AT+X=<target adc value>,123
              // Syntax: AT+X=<target adc value>,12.3s
              raiseError = servo->programTargetADCMove(uintValue, pwmSetting, moveTimeMillisecond);
              
            }else{
              // Syntax: AT+X=0
              Serial.printf("SpeedMode: request to stop" "\n");
              servo->reset("Speed=0");
            }
          }
        }else{
          // starts with a sign, so either delta ADC or duration
          if('M' == values[0].nextChar){
            // open-loop millisecond move request
            // Syntax: AT+X=1234M
            // Syntax: AT+X=1234M,123
            servo->programOpenLoopMove(uintValue, values[0].sign, pwmSetting);
          }else{
            // Syntax: AT+X=+34
            // Syntax: AT+X=+34,123
            // Syntax: AT+X=+34,12.3S
            int targetADC = (int)servo->getAdcValue() + values[0].sign * uintValue;
            if(servo->isAdcTargetValid(targetADC)){
              servo->programTargetADCMove(targetADC, pwmSetting, moveTimeMillisecond);
            }
          }
        }
      }
    }else{
      if(-1 != pwmSetting){
        Serial.printf("pwmSetting=%d" "\n", pwmSetting);
      }else{
        raiseError = true;
        Serial.printf("missing first number or pwmSetting" "\n");
      }
    }
  }
  return(raiseError);
}

bool handlePlus(Stream *stream, const char *szString, int length) {
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
      raiseError = sub(stream, servo, axis, szString, comas);
      if(raiseError){
        servo->printLastErrorString(stream);
      }
    }else if(('X' == axis) && (plusRead == sub)){
      raiseError = false;
      extender.printState(stream);
    }
  }
  // Serial.printf("%s(\"%s\")=>%d" "\n", __func__, szString, raiseError);
  return (raiseError);
}
