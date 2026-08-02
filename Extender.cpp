#include "Extender.hpp"

Extender::Extender(void){
  filter = SlidingWindow("X", 4);
  filtered = 0;
}

void Extender::updateState(unsigned short s){
  filtered = filter.input(s);
}

void Extender::printState(Stream *stream){
  stream->printf("X=%d" "\n", filtered);
}

Extender extender;
