#include "SlidingWindow.hpp"

SlidingWindow::SlidingWindow(unsigned int log_size){
  if(log_size > __SLIDING_WINDOW_MAX_LOG_SIZE__){
    log_size = __SLIDING_WINDOW_MAX_LOG_SIZE__;
  }
  this->log_size = log_size;
  actual_size = (1 << log_size);
  somme = 0;
  index = actual_size;
  while(--index){
    values[index] = 0;
  }
}

SlidingWindow::SlidingWindow(void){
  SlidingWindow(__SLIDING_WINDOW_MAX_LOG_SIZE__);
}

unsigned short SlidingWindow::input(unsigned short newValue){
  somme -= (unsigned int)values[index];
  somme += (unsigned int)newValue;
  values[index] = newValue;
  if(index){
    index--;
  }else{
    index = actual_size - 1;
  }
  return(somme >> log_size);
}

unsigned char SlidingWindow::getFilterLength(void){
  return(actual_size);
}
