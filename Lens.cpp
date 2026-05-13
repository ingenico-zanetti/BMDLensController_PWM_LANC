#include "Lens.hpp"
#include "GlobalConfiguration.hpp"

#if defined(__HA18X5POINT5BEMD_M6__)
// Actually, an HA18x5.5BERM-M6 retrofitted with a BMD unit from another Fujinon BMD lens

const char *szLensName = "FUJINON HA18x5.5BEMD-M6";

const ServoSettings zoomSettings = {
  .setPoints = {
    { .setting =   55, .adcValue = 2927 },
    { .setting =  100, .adcValue = 2590 },
    { .setting =  200, .adcValue = 2225 },
    { .setting =  500, .adcValue = 1806 },
    { .setting = 1000, .adcValue = 1484 },
    { 0, 0}
  },
  .parameters = {
    .pwmScale = 4,
    .timeoutScale = 128,
    .minSpeed = 3,
    .rfu = 0
  }
};


const ServoSettings irisSettings = {
  .setPoints = {
    { .setting =  18, .adcValue = 2842},
    { .setting =  28, .adcValue = 2631},
    { .setting =  40, .adcValue = 2396},
    { .setting =  56, .adcValue = 2179},
    { .setting =  80, .adcValue = 1960},
    { .setting = 110, .adcValue = 1736},
    { .setting = 160, .adcValue = 1532},
    { 0, 0}
  },
  .parameters = {
    .pwmScale = 10,
    .timeoutScale = 64,
    .minSpeed = 1,
    .rfu = 0
  }
};

const ServoSettings focusSettings = {
  .setPoints = {
    { .setting =    4, .adcValue = 1143},
    { .setting =    5, .adcValue = 1432},
    { .setting =    6, .adcValue = 1639},
    { .setting =    8, .adcValue = 1877},
    { .setting =   10, .adcValue = 2024},
    { .setting =   20, .adcValue = 2328},
    { .setting =   50, .adcValue = 2541},
    { .setting = 9990, .adcValue = 2692}, // infinity
    { .setting = 9999, .adcValue = 2815}, // and beyond
    { 0, 0}
  },
  .parameters = {
    .pwmScale = 6,
    .timeoutScale = 128,
    .minSpeed = 2,
    .rfu = 0
  }
};


#elif defined(__HA18x7POINT6BEMD_M1B__)
// Actually, an HA18x7.6BERM-M1B retrofitted with a BMD unit from another Fujinon BMD lens
const char *szLensName = "FUJINON HA18x7.6BEMD-M1B";

const ServoSettings zoomSettings = {
  .setPoints = {
    { .setting =   76, .adcValue = 2997 },
    { .setting =  150, .adcValue = 2619 },
    { .setting =  300, .adcValue = 2332 },
    { .setting =  600, .adcValue = 2049 },
    { .setting = 1370, .adcValue = 1708 },
    { 0, 0}
  },
  .parameters = {
    .pwmScale = 4,
    .timeoutScale = 100,
    .minSpeed = 1,
    .rfu = 0
  }
};


const ServoSettings irisSettings = {
  .setPoints = {
    { .setting =  18, .adcValue = 2906},
    { .setting =  28, .adcValue = 2595},
    { .setting =  40, .adcValue = 2291},
    { .setting =  56, .adcValue = 2052},
    { .setting =  80, .adcValue = 1823},
    { .setting = 110, .adcValue = 1611},
    { .setting = 160, .adcValue = 1424},
    { 0, 0}
  },
  .parameters = {
    .pwmScale = 10,
    .timeoutScale = 32,
    .minSpeed = 4,
    .rfu = 0
  }
};

const ServoSettings focusSettings = {
  .setPoints = {
    { .setting =    6, .adcValue = 1202},
    { .setting =    7, .adcValue = 1377},
    { .setting =    8, .adcValue = 1572},
    { .setting =    9, .adcValue = 1717},
    { .setting =   10, .adcValue = 1852},
    { .setting =   12, .adcValue = 1997},
    { .setting =   15, .adcValue = 2156},
    { .setting =   20, .adcValue = 2314},
    { .setting =   30, .adcValue = 2494},
    { .setting =   50, .adcValue = 2623},
    { .setting =  100, .adcValue = 2713}, // actually 30 feet
    { .setting = 9990, .adcValue = 2829}, // infinity
    { .setting = 9999, .adcValue = 2863}, // and beyond
    { 0, 0}
  },
  .parameters = {
    .pwmScale = 6,
    .timeoutScale = 64,
    .minSpeed = 4,
    .rfu = 0
  }
};

#elif defined(__S20X6POINT4BMD_DSD__)

const char *szLensName = "FUJINON S20x6.4BMD-DSD";

const ServoSettings zoomSettings = {
  .setPoints = {
    { .setting =   64, .adcValue = 3030 },
    { .setting =  150, .adcValue = 2560 },
    { .setting =  300, .adcValue = 2280 },
    { .setting =  600, .adcValue = 2010 },
    { .setting = 1280, .adcValue = 1725 },
    { 0, 0}
  },
  .parameters = {
    .pwmScale = 4,
    .timeoutScale = 100,
    .minSpeed = 1,
    .rfu = 0
  }
};


const ServoSettings irisSettings = {
  .setPoints = {
    { .setting =  14, .adcValue = 2985},
    { .setting =  20, .adcValue = 2750},
    { .setting =  28, .adcValue = 2490},
    { .setting =  40, .adcValue = 2265},
    { .setting =  56, .adcValue = 2070},
    { .setting =  80, .adcValue = 1872},
    { .setting = 110, .adcValue = 1665},
    { .setting = 160, .adcValue = 1438},
    { 0, 0}
  },
  .parameters = {
    .pwmScale = 10,
    .timeoutScale = 32,
    .minSpeed = 4,
    .rfu = 0
  }
};

const ServoSettings focusSettings = {
  .setPoints = {
    { .setting =    9, .adcValue = 1293},
    { .setting =   12, .adcValue = 1666},
    { .setting =   15, .adcValue = 1905},
    { .setting =   20, .adcValue = 2140},
    { .setting =   30, .adcValue = 2375},
    { .setting =   50, .adcValue = 2566},
    { .setting =  100, .adcValue = 2708},
    { .setting = 9990, .adcValue = 2852}, // infinity
    { .setting = 9999, .adcValue = 2880}, // and beyond
    { 0, 0}
  },
  .parameters = {
    .pwmScale = 6,
    .timeoutScale = 64,
    .minSpeed = 4,
    .rfu = 0
  }
};

#elif defined(__T14X5POINT5DA_D24__)

// Fujinon AW-LZ14MD55 (T14x5.5DA-D24, 1/3" C-Mount)
// Beware: the test device had been fully disassembled, so some ranges might be shifted
const char *szLensName = "FUJINON AW-LZ14MD55 / T14x5.5DA-D24";

const ServoSettings zoomSettings = {
  .setPoints = {
    { .setting =   55, .adcValue = 3029 },
    { .setting =  100, .adcValue = 2498 },
    { .setting =  200, .adcValue = 1989 },
    { .setting =  400, .adcValue = 1533 },
    { .setting =  770, .adcValue = 1152 },
    { 0, 0}
  },
  .parameters = {
    .pwmScale = 4,
    .timeoutScale = 100,
    .minSpeed = 1,
    .rfu = 0
  }
};

const ServoSettings irisSettings = {
  .setPoints = {
    { .setting =  14, .adcValue = 3010},
    { .setting =  20, .adcValue = 2730},
    { .setting =  28, .adcValue = 2497},
    { .setting =  40, .adcValue = 2275},
    { .setting =  56, .adcValue = 2069},
    { .setting =  80, .adcValue = 1843},
    { .setting = 110, .adcValue = 1633},
    { .setting = 160, .adcValue = 1369},
    { 0, 0}
  },
  .parameters = {
    .pwmScale = 4,
    .timeoutScale = 32,
    .minSpeed = 2,
    .rfu = 0
  }
};

const ServoSettings focusSettings = {
  .setPoints = {
    { .setting =   10, .adcValue = 1296}, // 1.0m
    { .setting =   12, .adcValue = 1600}, // 1.2m
    { .setting =   15, .adcValue = 1913}, // 1.5m
    { .setting =   20, .adcValue = 2235}, // 2.0
    { .setting =   30, .adcValue = 2548}, // 3.0m
    { .setting =   50, .adcValue = 2814}, // 5.0m
    { .setting =  100, .adcValue = 2980}, // 10.0m
    { .setting = 9990, .adcValue = 3118}, // infinity
    { .setting = 9999, .adcValue = 3127}, // and beyond
    { 0, 0}
  },
  .parameters = {
    .pwmScale = 4,
    .timeoutScale = 64,
    .minSpeed = 3,
    .rfu = 0
  }
};

#elif defined(__A17X9BMD_D24__)
const char *szLensName = "FUJINON A17x9BMD-D24";

const ServoSettings zoomSettings = {
  .setPoints = {
    { .setting =   90, .adcValue = 2640 },
    { .setting =  200, .adcValue = 2146 },
    { .setting =  400, .adcValue = 1853 },
    { .setting =  800, .adcValue = 1584 },
    { .setting = 1530, .adcValue = 1320 },
    { 0, 0}
  },
  .parameters = {
    .pwmScale = 4,
    .timeoutScale = 100,
    .minSpeed = 1,
    .rfu = 0
  }
};

const ServoSettings irisSettings = {
  .setPoints = {
    { .setting =  19, .adcValue = 3040},
    { .setting =  28, .adcValue = 2767},
    { .setting =  40, .adcValue = 2516},
    { .setting =  56, .adcValue = 2225},
    { .setting =  80, .adcValue = 1993},
    { .setting = 110, .adcValue = 1786},
    { .setting = 160, .adcValue = 1571},
    { 0, 0}
  },
  .parameters = {
    .pwmScale = 4,
    .timeoutScale = 32,
    .minSpeed = 2,
    .rfu = 0
  }
};

const ServoSettings focusSettings = {
  .setPoints = {
    { .setting =    9, .adcValue = 1323},
    { .setting =   12, .adcValue = 1721},
    { .setting =   15, .adcValue = 1961},
    { .setting =   20, .adcValue = 2190},
    { .setting =   30, .adcValue = 2414},
    { .setting =   50, .adcValue = 2614},
    { .setting =  100, .adcValue = 2770},
    { .setting = 9990, .adcValue = 2912}, // infinity
    { .setting = 9999, .adcValue = 2963}, // and beyond
    { 0, 0}
  },
  .parameters = {
    .pwmScale = 6,
    .timeoutScale = 64,
    .minSpeed = 4,
    .rfu = 0
  }
};

#elif defined(__S19X6POINT5BMD_D4M__)

const char *szLensName = "FUJINON AT2 S19x6.5BMD-D4M";

const ServoSettings zoomSettings = {
  .setPoints = {
    { .setting =   65, .adcValue = 3030 },
    { .setting =  150, .adcValue = 2560 },
    { .setting =  300, .adcValue = 2280 },
    { .setting =  600, .adcValue = 2010 },
    { .setting = 1230, .adcValue = 1725 },
    { 0, 0}
  },
  .parameters = {
    .pwmScale = 4,
    .timeoutScale = 100,
    .minSpeed = 1,
    .rfu = 0
  }
};


const ServoSettings irisSettings = {
  .setPoints = {
    { .setting =  14, .adcValue = 2985},
    { .setting =  20, .adcValue = 2750},
    { .setting =  28, .adcValue = 2490},
    { .setting =  40, .adcValue = 2265},
    { .setting =  56, .adcValue = 2070},
    { .setting =  80, .adcValue = 1872},
    { .setting = 110, .adcValue = 1665},
    { .setting = 160, .adcValue = 1438},
    { 0, 0}
  },
  .parameters = {
    .pwmScale = 10,
    .timeoutScale = 32,
    .minSpeed = 4,
    .rfu = 0
  }
};

const ServoSettings focusSettings = {
  .setPoints = {
    { .setting =    9, .adcValue = 1293},
    { .setting =   12, .adcValue = 1666},
    { .setting =   15, .adcValue = 1905},
    { .setting =   20, .adcValue = 2140},
    { .setting =   30, .adcValue = 2375},
    { .setting =   50, .adcValue = 2566},
    { .setting =  100, .adcValue = 2708},
    { .setting = 9990, .adcValue = 2852}, // infinity
    { .setting = 9999, .adcValue = 2880}, // and beyond
    { 0, 0}
  },
  .parameters = {
    .pwmScale = 6,
    .timeoutScale = 64,
    .minSpeed = 4,
    .rfu = 0
  }
};

#elif defined(__HSS18X5POINT5BMD_D18__)

const char *szLensName = "FUJINON HSs18x5.5BMD-D18";

const ServoSettings zoomSettings = {
  .setPoints = {
    { .setting =   55, .adcValue = 3004 },
    { .setting =  100, .adcValue = 2655 },
    { .setting =  200, .adcValue = 2366 },
    { .setting =  500, .adcValue = 1978 },
    { .setting = 1000, .adcValue = 1701 },
    { 0, 0}
  },
  .parameters = {
    .pwmScale = 4,
    .timeoutScale = 128,
    .minSpeed = 3,
    .rfu = 0
  }
};


const ServoSettings irisSettings = {
  .setPoints = {
    { .setting =  14, .adcValue = 3116},
    { .setting =  20, .adcValue = 2820},
    { .setting =  28, .adcValue = 2640},
    { .setting =  40, .adcValue = 2397},
    { .setting =  56, .adcValue = 2164},
    { .setting =  80, .adcValue = 1948},
    { .setting = 110, .adcValue = 1724},
    { .setting = 160, .adcValue = 1515},
    { 0, 0}
  },
  .parameters = {
    .pwmScale = 10,
    .timeoutScale = 64,
    .minSpeed = 1,
    .rfu = 0
  }
};

const ServoSettings focusSettings = {
  .setPoints = {
    { .setting =    6, .adcValue = 1173},
    { .setting =    7, .adcValue = 1361},
    { .setting =    8, .adcValue = 1566},
    { .setting =    9, .adcValue = 1708},
    { .setting =   10, .adcValue = 1834},
    { .setting =   12, .adcValue = 1990},
    { .setting =   15, .adcValue = 2148},
    { .setting =   20, .adcValue = 2310},
    { .setting =   30, .adcValue = 2463},
    { .setting =   50, .adcValue = 2594},
    { .setting = 9990, .adcValue = 2757}, // infinity
    { .setting = 9999, .adcValue = 2813}, // and beyond
    { 0, 0}
  },
  .parameters = {
    .pwmScale = 6,
    .timeoutScale = 128,
    .minSpeed = 2,
    .rfu = 0
  }
};

#elif defined(__A10X4POINT8BEMD_G28__)

const char *szLensName = "FUJINON A10x4.8BEMD-G28"; // Actually an A10x4.8BEVM-G28 retrofitted with the BMD from an A16x9.5BMD-D8

const ServoSettings zoomSettings = {
  .setPoints = {
    { .setting =  48, .adcValue = 3004 },
    { .setting = 100, .adcValue = 2655 },
    { .setting = 200, .adcValue = 2366 },
    { .setting = 300, .adcValue = 1978 },
    { .setting = 480, .adcValue = 1701 },
    { 0, 0}
  },
  .parameters = {
    .pwmScale = 4,
    .timeoutScale = 128,
    .minSpeed = 3,
    .rfu = 0
  }
};


const ServoSettings irisSettings = {
  .setPoints = {
    { .setting =  18, .adcValue = 3116},
    { .setting =  28, .adcValue = 2640},
    { .setting =  40, .adcValue = 2397},
    { .setting =  56, .adcValue = 2164},
    { .setting =  80, .adcValue = 1948},
    { .setting = 110, .adcValue = 1724},
    { .setting = 160, .adcValue = 1515},
    { 0, 0}
  },
  .parameters = {
    .pwmScale = 10,
    .timeoutScale = 64,
    .minSpeed = 1,
    .rfu = 0
  }
};

const ServoSettings focusSettings = {
  .setPoints = {
    { .setting =    3, .adcValue = 1173},
    { .setting =    4, .adcValue = 1361},
    { .setting =    6, .adcValue = 1566},
    { .setting =   10, .adcValue = 1834},
    { .setting =   15, .adcValue = 2148},
    { .setting =   30, .adcValue = 2463},
    { .setting = 9990, .adcValue = 2757}, // infinity
    { .setting = 9999, .adcValue = 2813}, // and beyond
    { 0, 0}
  },
  .parameters = {
    .pwmScale = 6,
    .timeoutScale = 128,
    .minSpeed = 2,
    .rfu = 0
  }
};

#else
#error "No Lens Selected !"
#endif
