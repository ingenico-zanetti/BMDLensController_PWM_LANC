#include <Arduino.h>

#include "Servo.hpp"

bool handleATZ(const char *szString, int length) {
  bool raiseError = false;
  int index = 0;
  if(length > 1){
    index = szString[1] - '0';
  }
  switch (index) {
    default:
      raiseError = true;
    break;
    case 0:
      // Reload servo settings from EEPROM
      if(!raiseError){
        raiseError = zoomServo.loadSettingsFromEEPROM();
      }
      if(!raiseError){
        raiseError = irisServo.loadSettingsFromEEPROM();
      }
      if(!raiseError){
        raiseError = focusServo.loadSettingsFromEEPROM();
      }
      break;
    case 1:
      // Reload servo settings from FW
      zoomServo.loadSettingsFromFW();
      irisServo.loadSettingsFromFW();
      focusServo.loadSettingsFromFW();
      break;
  }
  return(raiseError);
}
