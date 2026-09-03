#include <Arduino.h>
#include "Servo.hpp"
#include "GlobalConfiguration.hpp"
#include "AtCommand.hpp"

bool handleAmpersAnd(Stream *stream, const char *szString, int length){
  bool raiseError = false;
  if(2 == length){
    switch(szString[1]){
      case 'V':
        stream->printf("%s:[adc=%4d]" "\n", zoomServo.getName(),  zoomServo.getAdcValue());
        stream->printf("%s:[adc=%4d]" "\n", irisServo.getName(),  irisServo.getAdcValue());
        stream->printf("%s:[adc=%4d]" "\n", focusServo.getName(), focusServo.getAdcValue());
      break;
      case 'W':
        zoomServo.storeSettingsToEEPROM();
        irisServo.storeSettingsToEEPROM();
        focusServo.storeSettingsToEEPROM();
      break;
      default:
        raiseError = true;
      break;
    } 
  }else{
    raiseError = true;
  }
  // Serial.printf("%s(\"%s\", %d)=>%d" "\n", __func__, szString, length, raiseError);
  return(raiseError);
}
