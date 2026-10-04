#ifndef __SERVO_AND_LENS_HPP_INCLUDED__
#define __SERVO_AND_LENS_HPP_INCLUDED__

#include "Arduino.h"

#include <assert.h>

#define MAX_SET_POINTS (16) // DON'T CHANGE !!!

#define FLAG_POSITIVE_DIRECTION (1 << 0)  // When the direction pin is HIGH, ADC is expected to increase

typedef struct {
    unsigned short int setting;  // 10 times the actual value: 6.4 is stored as 64, 999.9 is stored as 9999 and the max possible represented value is 6553.5 (meter, millimeter, diaphragm, ...)
    unsigned short int adcValue;
} SetPoint;

typedef struct {
  SetPoint setPoints[MAX_SET_POINTS];
  struct __attribute__((packed)) {
    float pidP;
    float pidI;
    float pidD;
    uint32_t flags;
  }parameters;
} ServoSettings;

static_assert(sizeof(ServoSettings) == 80, "sizeof(ServoSettings) must be 80 !");

#endif // __SERVO_HPP_INCLUDED__
