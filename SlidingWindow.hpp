/*
 * Sliding window to filter noise on ADC
 */
#ifndef __SLIDING_WINDOW_HPP_INCLUDED__
#define __SLIDING_WINDOW_HPP_INCLUDED__

#include "Arduino.h"

#define __SLIDING_WINDOW_MAX_LOG_SIZE__ (4)
#define __SLIDING_WINDOW_MAX_SIZE__     (1 << __SLIDING_WINDOW_MAX_LOG_SIZE__)

class SlidingWindow {
  public:
    SlidingWindow(void){};
    SlidingWindow(const char *name, unsigned int s);
    unsigned short input(uint16_t newValue);
    unsigned int getFilterLength(void);
    void print(Stream *s);
  private:
    const char *szName;
    uint16_t values[(1 << __SLIDING_WINDOW_MAX_LOG_SIZE__)];
    int index;
    uint32_t somme;
    unsigned int actual_size;
    unsigned int log_size;
};

#endif // __SLIDING_WINDOW_HPP_INCLUDED__
