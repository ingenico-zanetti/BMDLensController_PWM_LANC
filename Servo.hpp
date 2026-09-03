#ifndef __SERVO_HPP_INCLUDED__
#define __SERVO_HPP_INCLUDED__

#include "SlidingWindow.hpp"
#include "ServoAndLens.hpp"
#include "GlobalConfiguration.hpp"

#define PWM_RATIO_HARD_LIMIT (0xC0)

class Servo {
  public:
    // possible mode, either through UART/CDC-ACM coammands or LANC
    static const int MODE_ADC        = 0; // move up-to a given position
    static const int MODE_DURATION   = 1; // move for a given time
    static const int MODE_TIMED_MOVE = 2; // move to a position in a given time
    static const int MODE_SPEED      = 3; // move at as constant as possible speed 

    // cuurrent direction of rotation
    static const int DIRECTION_BACKWARD = -1;
    static const int DIRECTION_STOPPED  = 0;
    static const int DIRECTION_FORWARD  = 1;

  private:
    SetPoint setPoints[MAX_SET_POINTS];
    int setPointCount;
    unsigned short int adcValue;    // as read from the ADC converter and smoothed by the sliding window filter
    // unsigned short int adcMinValue; // for statistical purpose only ; might be removed, as you can now print the sliding window
    // unsigned short int adcMaxValue; // for statistical purpose only ; might be removed, as you can now print the sliding window
    const char *szName;
    int remainingTimeMs;
    int direction;
    int adcPin;
    int pwmPin;
    int dirPin;
    int dirPinPolarity;
    int mode;
    SlidingWindow filter;
    unsigned int targetAdcValue;
    unsigned int pwmRatio;
    unsigned int pwmRatioMax;

    bool updateTarget(void);

    int getSetPointIndexFromSetting(unsigned short setting);
    int getSetPointPreviousIndexFromSetting(unsigned short setting);
    int getSetPointPreviousIndexFromAdc(unsigned short adc);

    unsigned int eepromOffset;
    const ServoSettings *servoSettingsFromFW;
    void loadSettingsFromMemory(const ServoSettings *settings);

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
      int mode;
      uint32_t timeMs;

    } target_context;

    struct {
      float kP;
      float kI;
      float kD;
      float previousError;
      float errorIntegral;
      float dt;
      float minOutput;
      float maxOutput;
    } pid_context;
    
  public:

    Servo(const ServoSettings *s, const char *name, unsigned int offset);
    void print(Stream *stream, const char *szUnit);
    
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
    bool runPID(void);
    int16_t getKP(void);
    int16_t getKI(void);
    int16_t getKD(void);
    bool setKP(float value);
    bool setKI(float value);
    bool setKD(float value);
    bool setSpeedAndDirection(int speed, int direction);
    // bool updateTarget(void);


    SetPoint *getFirstSetPoint(void);
    SetPoint *getLastSetPoint(void);
    bool isAdcTargetValid(unsigned int adcValue);
    bool isSettingValid(unsigned int setting);

    void stopMotor(const char *szReason);

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
