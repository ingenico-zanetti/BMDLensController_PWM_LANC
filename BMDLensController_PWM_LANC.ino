#include "AtCommand.hpp"
#include "AtCommandI.hpp"
#include "AtCommandZ.hpp"
#include "AtCommandPlus.hpp"
#include "AtCommandAmpersAnd.hpp"
#include "Servo.hpp"
#include "Lens.hpp"
#include "GlobalConfiguration.hpp"
#include "Version.hpp"

#if 0
/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 48000000
  *            HCLK(Hz)                       = 48000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 2
  *            APB2 Prescaler                 = 1
  *            PLL_Source                     = HSE
  *            PLL_Mul                        = 6
  *            Flash Latency(WS)              = 2
  *            ADC Prescaler                  = 4
  *            USB Prescaler                  = 1
  * @param  None
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {};

  /* Initializes the CPU, AHB and APB busses clocks */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /* Initializes the CPU, AHB and APB busses clocks */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
    Error_Handler();
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC | RCC_PERIPHCLK_USB;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV4;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
    Error_Handler();
  }
}
#endif

/*
 * PIN configuration
 */
#define ZoomADC (PA4)  // ADC12_IN4 / BluePill.PIN09
#define IrisADC (PA5)  // ADC12_IN5 / BluePill.PIN10
#define FocusADC (PA6) // ADC12_IN6 / BluePill.PIN11
#ifdef __HAS_SPEED_CONTROL__
#define SpeedADC (PA7) // ADC12_IN7 / BluePill.PIN12
#endif

#define FocusPWM (PB9)  // TIM4_4 / BluePill.PIN37
#define FocusDIR (PB5) // GPIO / BluePill.PIN33

#define IrisPWM (PB8)   // TIM4_3 / BluePill.PIN36
#define IrisDIR (PB4)  // GPIO / BluePill.PIN32

#ifdef __HAS_EXTENDER_SWITCH__
#define ZoomExtSwitch (PB6) // SCL1 BluePill.PIN34
#endif

#define ZoomPWM (PB7)   // TIM4_2 / BluePill.PIN35
#define ZoomDIR (PB3)  // GPIO / BluePill.PIN31

#define CommandUartRx (PB11) // UART3_RX / BluePill.PIN16
#define CommandUartTx (PB10) // UART3_TX / BluePill.PIN15

#define LancUartRx (PA3) // UART2_RX / BluePill.PIN08
#define LancUartTx (PA2) // UART2_TX / BluePill.PIN07

int ledStatus;
uint32_t oldSeconds;
uint32_t oldMillis;
uint32_t oldMicros;
uint32_t oldQuarter;
uint32_t oldFrame;

bool powerPresent;

HardwareSerial commandSerial(CommandUartRx, CommandUartTx);
HardwareSerial lancSerial(LancUartRx, LancUartTx);
// USART_TypeDef *lancUart = NULL;

HardwareTimer *lancTimer = NULL;

#define LANC_START_BIT_INTERVAL_US (1500) // in µs
#define LANC_INTER_TELEGRAM_DELAY (3)     // in LANC_START_BIT_INTERVAL

static unsigned char lancTxData[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};
static unsigned char lancRxData[8];

static int lancIndex = 0;

static void lancInterrupt(void){
  // Receive the previous byte (we receive byte N when transmitting byte N+1)
  if((0 < lancIndex) && (lancIndex <= sizeof(lancTxData))){
    if(lancSerial.available()){
      int rxIndex = lancIndex - 1; 
      lancRxData[rxIndex] = lancSerial.read() ^ 0xFF; // Inverted logic
    }
  }
  // Transmit next byte if we are inside a telegram
  if((0 <= lancIndex) && (lancIndex < sizeof(lancTxData))){
    lancSerial.write(lancTxData[lancIndex]);
  }
  lancIndex++;
  if(lancIndex > (int)sizeof(lancTxData)){
    lancIndex = -LANC_INTER_TELEGRAM_DELAY;
  }
}

void setup() {
  Serial.begin(115200);
  commandSerial.begin(9600);

  lancSerial.begin(9600, SERIAL_8N1);
  // lancUart = lancSerial.getHandle()->Instance;

  pinMode(LED_BUILTIN, INPUT_PULLUP);
//  pinMode(LED_BUILTIN, OUTPUT);

  ledStatus = HIGH;
  digitalWrite(LED_BUILTIN, ledStatus);
  
  analogReference(AR_DEFAULT);
  analogReadResolution(12);

  oldMillis = millis();
  oldMicros = micros();
  oldQuarter = oldMillis / 250;
  oldSeconds = oldMillis / 1000;
  oldFrame = oldMillis / 16; // around 60 fps

  analyzerUSB.addCallback('I', handleATI);
  analyzerUSB.addCallback('+', handlePlus);
  analyzerUSB.addCallback('&', handleAmpersAnd);
  analyzerUSB.addCallback('Z', handleATZ);

  analyzerSerial.addCallback('I', handleATI);
  analyzerSerial.addCallback('+', handlePlus);
  analyzerSerial.addCallback('&', handleAmpersAnd);
  analyzerSerial.addCallback('Z', handleATZ);

  focusServo.setPins(FocusADC, FocusPWM, FocusDIR);
  zoomServo.setPins(ZoomADC, ZoomPWM, ZoomDIR);
  irisServo.setPins(IrisADC, IrisPWM, IrisDIR);

  focusServo.setMode(Servo::MODE_DURATION);
  zoomServo.setMode(Servo::MODE_DURATION);
  irisServo.setMode(Servo::MODE_DURATION);
 
  powerPresent = (zoomServo.getAdcValue() > 1100);

  lancTimer = new HardwareTimer(TIM2);
  lancTimer->setOverflow(LANC_START_BIT_INTERVAL_US, MICROSEC_FORMAT);
  lancTimer->attachInterrupt(lancInterrupt);
  lancTimer->resume();

}

static int previousLancIndex = -1;
static bool record = false, review = false;


static bool focusChange = false;
static int focusSteps = 0;

static bool irisChange = false;
static int irisSteps;

static int zoomSpeedFromLanc(int lancSpeed){
  if(lancSpeed < 0x10){
    return(+(48 + lancSpeed * 10)); // tighter angle
  }else{
    return(-(32 + (lancSpeed - 0x10) * 10)); // wider angle
  }
}

void loop() {
  focusServo.readAdc();
  zoomServo.readAdc();
  irisServo.readAdc();
  uint32_t newMillis = millis();
  if(newMillis != oldMillis){
    focusServo.run();
    zoomServo.run();
    irisServo.run();
    oldMillis = newMillis;
#ifndef __NO_LANC__
    int currentLancIndex = lancIndex;
    if(previousLancIndex != currentLancIndex){
      currentLancIndex = previousLancIndex;
      if(5 == lancIndex){
        // We have received the 4 bytes from the remote, look at what we have
        // We have 500ms to put a correct answer in the last 4 bytes before they start being transmitted
        int zoomSpeed = 0;
        unsigned char subCommand = lancRxData[0];
        unsigned char command = lancRxData[1];
        switch(subCommand){
          default:
          break;
          case 0x00:
            if(record){
              record = false;
              Serial.printf("+RECORD" "\n");
            }
            if(review){
              review = false;
              Serial.printf("+REVIEW" "\n");
            }
            if(focusChange){
              focusChange = false;
              Serial.printf("+FOCUS:%d" "\n", focusSteps);
              if(0 != focusSteps){
                focusServo.setDirection(focusSteps > 0);
                if(focusSteps < 0){
                  focusSteps = -focusSteps;
                }
                focusServo.setTimeMs(1 * focusSteps);
              }
            }
            if(irisChange){
              irisChange = false;
              if(0 != irisSteps){
                Serial.printf("+IRIS:%d([index]=%d)" "\n", irisSteps, index);
#if 0
                int index = irisServo.getClosestSettingIndexFromAdcValue(irisServo.getAdcValue());
                // Serial.printf("+IRIS:%d(index=%d)" "\n", irisSteps, index);
                int setPointCount = 0;
                SetPoint *setPoints = irisServo.getSetPoints(&setPointCount);
                index += irisSteps;
                if(index < 0){
                  index = 0;
                }else if(index >= setPointCount){
                  index = (setPointCount - 1);
                }
                irisServo.setTargetAdcValue(setPoints[index].adcValue);
#else
                irisServo.setDirection(irisSteps > 0);
                if(irisSteps < 0){
                  irisSteps = -irisSteps;
                }
                irisServo.setTimeMs(10 * irisSteps);
#endif
              }else{
                Serial.printf("+IRIS:Auto" "\n");
              }
            }
          break;
          case 0x18:
          switch(command){
            default:
              Serial.printf("0x18 0x%02X" "\n", command);
            break;
            case 0x33:
              record = true;
              break;
            case 0x69:
              review = true;
              break;
          }
          break;
          case 0x28:
            switch(command){
            default:
              Serial.printf("0x28 0x%02X" "\n", command);
            break;
            case 0x00:
            case 0x02:
            case 0x04:
            case 0x06:
            case 0x08:
            case 0x0A:
            case 0x0C:
            case 0x0E:
            case 0x10:
            case 0x12:
            case 0x14:
            case 0x16:
            case 0x18:
            case 0x1A:
            case 0x1C:
            case 0x1E:
              zoomSpeed = zoomSpeedFromLanc(command);
            break;
            case 0xAD:
              irisChange = true;
              irisSteps = 0;
              break;
            case 0x53:
              irisChange = true;
              irisSteps = +1;
              break;
            case 0x55:
              irisChange = true;
              irisSteps = -1;
              break;
            case 0xC5:
              irisChange = true;
              irisSteps = +4;
              break;
            case 0xC7:
              irisChange = true;
              irisSteps = -4;
              break;
            case 0x41:
              focusChange = true;
              focusSteps = 0;
              break;
            case 0xE1:
            case 0xE3:
            case 0xE5:
              focusChange = true;
              focusSteps = +(command - 0xE0);
              break;
            case 0xF1:
            case 0xF3:
            case 0xF5:
              focusChange = true;
              focusSteps = -(command - 0xF0);
              break;
          }
          break;
        }
        if(0 != zoomSpeed){
          if(0 < zoomSpeed){
            zoomServo.setDirection(true);
          }else{
            zoomServo.setDirection(false);
            zoomSpeed = -zoomSpeed;
          }
          zoomServo.setPwmRatioMax(zoomSpeed);
          zoomServo.setTimeMs(20); // at least up-to next LANC cycle
        }
      } // lancIndex == 5
    }
#endif // __LANC__
  }
  {
    // USB channel
    int available = Serial.available();
    if(available > 0){
#define BUFFER_SIZE (64)
      char buffer[BUFFER_SIZE];
      if((unsigned int)available > sizeof(buffer)){
        available = sizeof(buffer);
      }
      int lus = Serial.readBytes(buffer, available);
      if(lus > 0){
        // Serial.write(buffer, lus);
        for(int i = 0 ; i < lus ; i++){
          analyzerUSB.addChar(buffer[i]);
        }
      }
    }
  }
  {
    // UART channel
    int available = commandSerial.available();
    if(available > 0){
#define BUFFER_SIZE (64)
      char buffer[BUFFER_SIZE];
      if((unsigned int)available > sizeof(buffer)){
        available = sizeof(buffer);
      }
      int lus = commandSerial.readBytes(buffer, available);
      if(lus > 0){
        for(int i = 0 ; i < lus ; i++){
          analyzerSerial.addChar(buffer[i]);
        }
      }
    }
  }
  uint32_t newSeconds = newMillis / 1000;
  if(newSeconds != oldSeconds){
    oldSeconds = newSeconds;
#if 0
    Serial.printf("ZoomADC=%5d" "\n", zoomServo.getAdcValue());
    Serial.printf("IrisADC= %5d" "\n", irisServo.getAdcValue());
    Serial.printf("FocusADC=%5d" "\n", focusServo.getAdcValue());
#endif
  }
  uint32_t newQuarter = newMillis / 250;
  if(newQuarter != oldQuarter){
    oldQuarter = newQuarter;
    ledStatus = (HIGH == ledStatus) ? LOW : HIGH;
    digitalWrite(LED_BUILTIN, ledStatus);
  }
}

