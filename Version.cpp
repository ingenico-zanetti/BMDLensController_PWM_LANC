#include "GlobalConfiguration.hpp"

static const char *const FW_VERSION = "BMD Lens Controller PWM+LANC v4.0.0, built on " __DATE__ " at " __TIME__;

const char *getFWVersion(void){
  return(FW_VERSION);
}
