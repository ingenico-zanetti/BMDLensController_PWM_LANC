#ifndef __SERVO_AND_LENS_HPP_INCLUDED__
#define __SERVO_AND_LENS_HPP_INCLUDED__

#include <assert.h>

#define MAX_SET_POINTS (15) // DON'T CHANGE !!!

typedef struct {
    unsigned short int setting;  // 10 times the actual value: 6.4 is stored as 64, 999.9 is stored as 9999 and the max possible represented value is 6553.5 (meter, millimeter, diaphragm, ...)
    unsigned short int adcValue;
} SetPoint;

typedef struct {
  SetPoint setPoints[MAX_SET_POINTS];
  struct __attribute__((packed)) {
    unsigned char pwmScale;
    unsigned char timeoutScale;
    unsigned char minSpeed;
    unsigned char rfu;
  }parameters;
} ServoSettings;

static_assert(sizeof(ServoSettings) == 64, "sizeof(ServoSettings) must be 64 !");

#endif // __SERVO_HPP_INCLUDED__
