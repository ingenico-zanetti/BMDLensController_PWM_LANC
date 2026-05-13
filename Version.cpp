#include "GlobalConfiguration.hpp"

#ifdef __PAN_AND_TILT_SUPPORT__
static const char *const FW_VERSION = "BMD Lens Controller + Pan & Tilt v3.0.1, built on " __DATE__ " at " __TIME__;
#else
static const char *const FW_VERSION = "BMD Lens Controller v3.0.1, built on " __DATE__ " at " __TIME__;
#endif

const char *getFWVersion(void){
  return(FW_VERSION);
}
