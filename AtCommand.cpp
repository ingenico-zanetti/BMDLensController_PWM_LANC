#include "AtCommand.hpp"
#include <ctype.h>
#include <string.h>
#include "GlobalConfiguration.hpp"

bool errorCallback(const char *szString, int length){
  (void)szString;
  (void)length;
  return(true);
}

AtCommandAnalyzer::AtCommandAnalyzer(void){
  for(int i = 0 ; i < FIRST_LEVEL_COMMANDS ; i++){
    firstLevelCommands[i] = errorCallback;
  }
  init(MAX_COMMAND_SIZE);
}

char AtCommandAnalyzer::read(void){
  if(currentSize > 0){
    char c = *--rPtr;
    if(data == rPtr){
      rPtr = data + maxSize; // Wrap around
    }
    currentSize--;
    return(c);
  }
  return(0);
}

void AtCommandAnalyzer::init(uint32_t size){
  maxSize = size;
  reInit();
}

void AtCommandAnalyzer::reInit(void){
  wPtr = data + maxSize;
  rPtr = wPtr;
  currentSize = 0;
}

void AtCommandAnalyzer::checkAndStrip(void){
  // Serial.printf("%s()" "\n", __func__);
  char A = read();
  if('A' == A){
    char T = read();
    if('T' == T){
      char callbackBuffer[MAX_COMMAND_SIZE];
      int i = 0;
      while(currentSize > 1){
        callbackBuffer[i] = read();
        i++;
      }
      callbackBuffer[i] = '\0';
//      Serial.printf("callbackBuffer[]=\"%s\"" "\n", callbackBuffer);
      analyze(callbackBuffer);
    }
  }
  reInit();
}

void AtCommandAnalyzer::addChar(char car){
  *(--wPtr) = toupper(car) & 0x7F;
  if(data == wPtr){
    wPtr = data + maxSize; // Wrap around
  }
  if(currentSize == maxSize){
    rPtr = wPtr; // discard old data
  }else{
    currentSize++;
  }
  if(('\r' == car) || ('\n' == car)){
    checkAndStrip();
  }
}

void AtCommandAnalyzer::analyze(char *szString){
  char *start = szString;
  bool error = false;
  for(;;){
    bool endReached = false;
    char first = *start;
    if('\0' == first){
      break;
    }
    int length = 0;
    char *next = strchr(start, ';');
    if(NULL == next){
      endReached = true;
      length = strlen(start);
    }else{
      *next = '\0';
      length = (int)(next - start);
    }
    if(length > 0){
      error = firstLevelCommands[(int)first](start, length);
    }
    if(error || endReached){
      break;
    }
    start = next + 1;
  }
  if(error){
    Serial.printf("\r\nERROR\r\n");
  }else{
    Serial.printf("\r\nOK\r\n");
  }
}

void AtCommandAnalyzer::addCallback(char c, FirstLevelCommand command){
  int index = toupper(c);
  if((' ' < index) && (index < FIRST_LEVEL_COMMANDS)){
    firstLevelCommands[index] = command;
  }
}

AtCommandAnalyzer analyzerUSB;
AtCommandAnalyzer analyzerSerial;
