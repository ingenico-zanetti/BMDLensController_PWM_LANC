#include <Arduino.h>
#include "Servo.hpp"
#include "GlobalConfiguration.hpp"
#include "AtCommand.hpp"

bool handleAmpersAnd(const char *szString, int length){
  bool raiseError = false;
  if(2 == length){
    switch(szString[1]){
      case 'V':
        Serial.printf("%s:[%4d .. %4d .. %4d]" "\n", zoomServo.getName(), zoomServo.getAdcMinValue(), zoomServo.getAdcValue(), zoomServo.getAdcMaxValue()); // zoomServo.print("mm"); Serial.println();
        Serial.printf("%s:[%4d .. %4d .. %4d]" "\n", irisServo.getName(), irisServo.getAdcMinValue(), irisServo.getAdcValue(), irisServo.getAdcMaxValue()); // irisServo.print(""); Serial.println();
        Serial.printf("%s:[%4d .. %4d .. %4d]" "\n", focusServo.getName(), focusServo.getAdcMinValue(), focusServo.getAdcValue(), focusServo.getAdcMaxValue()); // focusServo.print("m"); Serial.println();
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
