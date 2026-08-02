#include "SlidingWindow.hpp"

SlidingWindow::SlidingWindow(const char *name, unsigned int log_size){
  if(log_size > __SLIDING_WINDOW_MAX_LOG_SIZE__){
    log_size = __SLIDING_WINDOW_MAX_LOG_SIZE__;
  }
  this->log_size = log_size;
  actual_size = (1 << log_size);
  somme = 0;
  index = actual_size - 1;
  for(int i = 0 ; i < actual_size ; i++){
    values[i] = 0;
  }
  szName = name;
}

unsigned short SlidingWindow::input(unsigned short newValue){
  somme -= (unsigned long int)values[index];
  somme += (unsigned long int)newValue;
  values[index] = newValue;
  if(index > 0){
    index--;
  }else{
    index = actual_size - 1;
  }
  return(somme >> log_size);
}

unsigned char SlidingWindow::getFilterLength(void){
  return(actual_size);
}

void SlidingWindow::print(Stream *stream){
  stream->printf("[%s] size: %d, [", szName, actual_size);
  for(int i = 0 ; i < actual_size ; i++){
    stream->printf("%u ", values[i]);
  }
  stream->printf("], somme=%lu" "\n", somme);
}
