#ifndef __SERVO_HPP_INCLUDED__
#define __SERVO_HPP_INCLUDED__

#include "SlidingWindow.hpp"
#include "ServoAndLens.hpp"
#include "GlobalConfiguration.hpp"

class PIDController {
public:
    float kP, kI, kD;
    float prevError;
    float integral;
    float minOutput, maxOutput;
    float setpoint;

public:
    PIDController(): prevError(0.0f), integral(0.0f), minOutput(0.0f), maxOutput(0.0f) {}

    float compute(float currentVal, float dt, float *diff) {
        if (dt <= 0.0f) return 0.0f;

        // 1. Calculate Error
        float error = setpoint - currentVal;
        *diff = error;

        // 2. Proportional Term
        float pOut = kP * error;

        // 3. Integral Term (with clamping to prevent Windup)
        integral += error * dt;
        float iOut = kI * integral;

        // 4. Derivative Term (rate of change)
        float derivative = (error - prevError) / dt;
        float dOut = kI * derivative;

        // 5. Total Output
        float output = pOut + iOut + dOut;

#if (SERVO_LOOP_DIVIDER > 1)
    char floatAsString[32];
    dtostrf(setpoint, 6, 3, floatAsString);
    Serial.printf("compute:sp=%s,", floatAsString);
    dtostrf(currentVal, 6, 3, floatAsString);
    Serial.printf("cV=%s,", floatAsString);
    dtostrf(error, 6, 3, floatAsString);
    Serial.printf("err=%s,", floatAsString);
    dtostrf(pOut, 6, 3, floatAsString);
    Serial.printf("pOut=%s,", floatAsString);
    dtostrf(iOut, 6, 3, floatAsString);
    Serial.printf("iOut=%s,", floatAsString);
    dtostrf(dOut, 6, 3, floatAsString);
    Serial.printf("dOut=%s,", floatAsString);
    dtostrf(output, 6, 3, floatAsString);
    Serial.printf("=>%s" "\n", floatAsString);
#endif

        // Clamp Output to Actuator Limits
        if (output > maxOutput) {
            output = maxOutput;
        } else if (output < minOutput) {
            output = minOutput;
        }

        prevError = error;
        return output;
    }

    void reset() {
        prevError = 0.0f;
        integral = 0.0f;
    }
};

class Servo {
  public:
    void pidOutputToPWM(float pidOutput, float diff);
    static const int PWM_RATIO_HARD_LIMIT = 0xC0; // 8-bit PWM, but beyond 0xC0, the hardware behaviour is not predictable
//    static const int PWM_RATIO_HARD_LIMIT = 0x40; // 8-bit PWM, but beyond 0xC0, the hardware behaviour is not predictable

    // possible mode, either through UART/CDC-ACM coammands or LANC
    static const int MOVE_MODE_NONE       = -1; // no move requested
    static const int MOVE_MODE_ADC        = 0; // move up-to a given position
    static const int MOVE_MODE_DURATION   = 1; // move for a given time
    static const int MOVE_MODE_TIMED_MOVE = 2; // move to a position in a given time
    static const int MOVE_MODE_SPEED      = 3; // move at a as constant as possible speed 

    // cuurrent direction of rotation
    static const int DIRECTION_BACKWARD = -1;
    static const int DIRECTION_STOPPED  = 0;
    static const int DIRECTION_FORWARD  = 1;

  private:
    SetPoint setPoints[MAX_SET_POINTS];
    uint16_t flags;
    int setPointCount;
    uint16_t adcValue;    // as read from the ADC converter and smoothed by the sliding window filter
    uint16_t adcLowestValue; // lowest value from the settings
    uint16_t adcHighestValue; // highest value from the settings
    const char *szName;
    int direction;
    int adcPin;
    int pwmPin;
    int dirPin;
    int dirPinPolarity;
    int mode;
    SlidingWindow filter;
    unsigned int targetAdcValue;
    struct {
      uint32_t toUse;
      uint32_t programmed;
      uint32_t max;
    } pwmRatio;

    bool updateTarget(void);

    int getSetPointIndexFromSetting(unsigned short setting);
    int getSetPointPreviousIndexFromSetting(unsigned short setting);
    int getSetPointPreviousIndexFromAdc(unsigned short adc);

    unsigned int eepromOffset;
    const ServoSettings *servoSettingsFromFW;
    void loadSettingsFromMemory(const ServoSettings *settings);
    void updateBoundaries(void);


    struct {
      uint32_t startADC;           // where we started from
      uint32_t stopADC;            // ADC position we aim for
      uint32_t msIncrement;        // time increment for decision
      uint32_t adcIncrement;       // adc increment for decision ; also the absolute value of (stopADC - startADC)
      uint32_t decision;           // used to update targetADC based on msIncrement and adcIncrement
      int32_t  targetADC;          // where we would like to be
      int32_t  targetADCIncrement; // +1/-1
      bool     complete;
    } timed_move_context;

    struct {
      uint32_t msBetweenTargetAdcIncrement; // Update target ADC every msBetweenTargetAdcIncrement millisecond ; for example once every 16 ms
      uint32_t msWaited;                    // current value trying to reach the above threshold
      int32_t  targetADC;                   // where we would like to be
      int32_t  targetADCIncrement;          // +1/-1
    } speed_move_context;

    struct {
      int mode;
      uint32_t timeMs;

    } target_context;

    struct {
      uint32_t remainingTimeMs;
      // int direction;
    } open_loop_context;

    const char *lastErrorString;
    PIDController pid;
    
  public:

    Servo(const ServoSettings *s, const char *name, unsigned int offset);
    void print(Stream *stream, const char *szUnit);
    void setLastErrorString(const char *szString);
    void printLastErrorString(Stream *stream);
    
    static char *setPointSettingToString(char *szString, SetPoint *setPoint);
    static bool stringToSetPointSetting(const char *start, int sLen, SetPoint *setPoint);

    bool setSetPoint(unsigned short setting, unsigned short adcValue);
    bool getAdcValueFromSetting(SetPoint *setPoint);
    int getClosestSettingIndexFromAdcValue(unsigned short adcValue);

    SetPoint *getSetPoints(int *actualCount);
    unsigned short getAdcValue(void);
    unsigned short readAdc(void);
    const char *getName(void);

    void setPins(int adc, int pwm, int dir, int dirPolarity=0);
    void setDirection(bool = true);
    int setMode(int newMode);
    int getMode(void);

    bool setTimeMs(int t);
    int getTimeMs(void);

    bool setDeltaAdc(int t);
    bool setTargetAdcValue(int i);
    bool timedMoveInit(uint32_t milliseconds);

    int everyMilliSecond(void); // called every millisecond
    int16_t getKP(void);
    int16_t getKI(void);
    int16_t getKD(void);
    bool setKP(float value);
    bool setKI(float value);
    bool setKD(float value);

    bool setSpeedAndDirection(int speed, int direction);
    bool programOpenLoopMove(uint32_t durationMillisecond, int direction, int32_t pwm);
    bool programTargetADCMove(uint16_t targetADC, uint32_t pwmSetting, uint32_t moveTimeMillisecond);


    SetPoint *getFirstSetPoint(void);
    SetPoint *getLastSetPoint(void);
    bool isAdcTargetValid(uint16_t adcValue);
    bool isSettingValid(uint16_t setting);

    void stopMotor(const char *szReason);
    void reset(const char *szReason);

    unsigned int setPwmRatioMax(unsigned int max);

    void loadSettingsFromFW();

    bool loadSettingsFromEEPROM();
    void storeSettingsToEEPROM();
};

extern Servo zoomServo;
extern Servo irisServo;
extern Servo focusServo;
extern Servo *getServo(int c);

#endif // __SERVO_HPP_INCLUDED__
