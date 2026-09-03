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
// static const char *useATW =  "(use AT&W to make the setting persistent)";

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
    // Time to take a decision ...
    // The first value should allow to distinguish between most of the case
    if(values[0].count > 0){
      // We have a number
      // Is it a SetPoint ?
      if(values[0].hasDot){
        SetPoint setPoint = {0, 0};
        Serial.printf("starts with a SetPoint, ");
        if(Servo::stringToSetPointSetting(values[0].stringStart, values[0].count, &setPoint)){
          Serial.printf("but the syntax is not correct." "\n");
          raiseError = true;
        }else{
          if(servo->getAdcValueFromSetting(&setPoint)){
            Serial.printf("but the SetPoint is not valid." "\n");
            raiseError = true;
          }else{
            Serial.printf("and the SetPoint is valid." "\n");
          }
        }
      }else{
        uint32_t uintValue = (uint32_t)values[0].value;
        // Do we have a sign ?
        if(0 == values[0].sign){
          // Absolute value
          if(uintValue <= (uint32_t)8){
            if(0 != uintValue){
              char dirChar = values[0].nextChar;
              int sign = 0;
              if('-' == dirChar){
                sign = -1;
              }
              if('+' == dirChar){
                sign = +1;
              }
              if(0 == sign){
                raiseError = true;
                Serial.printf("SpeedMode: request %d but no direction" "\n", uintValue);
              }else{
                Serial.printf("SpeedMode: request %d with direction %+d" "\n", uintValue, sign);
              }
            }else{
              Serial.printf("SpeedMode: request to stop" "\n");
            }
          }else{
            Serial.printf("looks like an absolute ADC request to %d" "\n", uintValue);
          }
        }else{
          // starts with a sign, so either delta ADC or duration
          if('M' == values[0].nextChar){
            // millisecond move request
            Serial.printf("open-loop move for %d ms into direction %+d" "\n", uintValue, values[0].sign);
          }else{
            Serial.printf("deltaADC move for %d steps into direction %+d" "\n", uintValue, values[0].sign);
          }
        }
      }
    }else{
      raiseError = true;
      Serial.printf("no number parsed" "\n");
    }
  }
    


#if 0

    bool parameterIsSetting = false;
    SetPoint setPoint = {0, 0};
    if(hasFirstValue){
      if('M' == *end){
        mode = Servo::MODE_ADC;
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
      if(numberOfComas >= 1){
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
          if((0 < max) && (max <= PWM_RATIO_HARD_LIMIT)){
            servo->setPwmRatioMax(max);
          }else{
            raiseError = true;
          }
        }
      }
    }
    if(!raiseError && parameterIsSetting && (2 == numberOfComas)){
      // AT+I=5.6,,     set the adcValue for setting 5.6 to the current adcValue for iris
      // AT+I=5.6,,2300 set the adcValue for setting 5.6 to 2300
      // AT&W is still required for the setting to be stored in Flash
      char *lastComa = strrchr(szString, ',');
      nptr = lastComa + 1;
      if('\0' == lastComa[1]){
        raiseError = servo->setSetPoint(setPoint.setting, servo->getAdcValue());
        stream->printf("%s: setting %d with current adcValue %d instead of %d => %d %s" "\n", servo->getName(), setPoint.setting, servo->getAdcValue(), setPoint.adcValue, raiseError, useATW);
      }else{
        nptr = lastComa + 1;
        float thirdValue = strtof(nptr, &end);
        if(end != nptr){
          raiseError = servo->setSetPoint(setPoint.setting, (unsigned short)thirdValue);
          stream->printf("%s: setting %d with provided adcValue %d instead of %d => %d %s" "\n", servo->getName(), setPoint.setting, (unsigned short)thirdValue, setPoint.adcValue, raiseError, useATW);
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
#endif
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
    }else if(('X' == axis) && (plusRead == sub)){
      raiseError = false;
      extender.printState(stream);
    }
  }
  // Serial.printf("%s(\"%s\")=>%d" "\n", __func__, szString, raiseError);
  return (raiseError);
}
