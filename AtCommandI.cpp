#include <Arduino.h>

#include "Servo.hpp"
#include "Lens.hpp"
#include "Version.hpp"
#include "GlobalConfiguration.hpp"
#include "AtCommand.hpp"

bool handleATI(Stream *stream, const char *szString, int length) {
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
      stream->println(getFWVersion());
      break;
    case 1:{
        char minZoomString[8];
        char maxZoomString[8];
        char minIrisString[8];
        char maxIrisString[8];
        char minFocusString[8];
        char maxFocusString[8];
        zoomServo.setPointSettingToString(minZoomString, zoomServo.getFirstSetPoint());
        zoomServo.setPointSettingToString(maxZoomString, zoomServo.getLastSetPoint());
        irisServo.setPointSettingToString(minIrisString, irisServo.getFirstSetPoint());
        irisServo.setPointSettingToString(maxIrisString, irisServo.getLastSetPoint());
        focusServo.setPointSettingToString(minFocusString, focusServo.getFirstSetPoint());
        focusServo.setPointSettingToString(maxFocusString, focusServo.getLastSetPoint());
        stream->printf("%s:" " Zoom in [%s..%s]mm," " Iris in [%s..%s]," " Focus in [%s..%s]m" "\n", szLensName, minZoomString, maxZoomString, minIrisString, maxIrisString, minFocusString, maxFocusString);
      }
      break;
      case 2:
        stream->printf("%s:" "\n", zoomServo.getName());
        zoomServo.print(stream, "mm");
        stream->printf("%s:" "\n", focusServo.getName());
        focusServo.print(stream, "m");
        stream->printf("%s:" "\n", irisServo.getName());
        irisServo.print(stream, "");
      break;
  }
  return(raiseError);
}
