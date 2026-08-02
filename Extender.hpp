#ifndef __EXTENDER_HPP_INCLUDED__
#define __EXTENDER_HPP_INCLUDED__

#include <Arduino.h>
#include "SlidingWindow.hpp"

class Extender {
  public:
    Extender(void);
    void updateState(unsigned short s);
    void printState(Stream *s);

  private:
    SlidingWindow filter;
    unsigned short filtered;

};

extern Extender extender;

#endif // __EXTENDER_HPP_INCLUDED__
