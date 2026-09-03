#include "SlidingWindow.hpp"

SlidingWindow::SlidingWindow(const char *name, unsigned int log_size){
  if(log_size > __SLIDING_WINDOW_MAX_LOG_SIZE__){
    log_size = __SLIDING_WINDOW_MAX_LOG_SIZE__;
  }
  this->log_size = log_size;
  actual_size = (1 << log_size);
  somme = 0;
  index = actual_size - 1;
  for(unsigned int i = 0 ; i < actual_size ; i++){
    values[i] = 0;
  }
  szName = name;
}

uint16_t SlidingWindow::input(uint16_t newValue){
  somme -= (unsigned long int)values[index];
  somme += (unsigned long int)newValue;
  values[index] = newValue;
  if(index > 0){
    index--;
  }else{
    index = actual_size - 1;
  }
  return(uint16_t)(somme >> log_size);
}

unsigned int SlidingWindow::getFilterLength(void){
  return(actual_size);
}

void SlidingWindow::print(Stream *stream){
  stream->printf("[%s] size: %d, [", szName, actual_size);
  for(unsigned int i = 0 ; i < actual_size ; i++){
    stream->printf("%u ", values[i]);
  }
  stream->printf("], filtered=%u" "\n", (uint32_t)(somme >> log_size));
}
