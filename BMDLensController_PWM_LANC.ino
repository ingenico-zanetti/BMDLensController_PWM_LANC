#include "AtCommand.hpp"
#include "AtCommandI.hpp"
#include "AtCommandZ.hpp"
#include "AtCommandPlus.hpp"
#include "AtCommandAmpersAnd.hpp"
#include "Servo.hpp"
#include "Lens.hpp"
#include "GlobalConfiguration.hpp"

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

/*
 * PIN configuration
 */
#define ZoomADC (PA4)  // ADC12_IN4 / BluePill.PIN09
#define IrisADC (PA5)  // ADC12_IN5 / BluePill.PIN10
#define FocusADC (PA6) // ADC12_IN6 / BluePill.PIN11
#define SpeedADC (PA7) // ADC12_IN7 / BluePill.PIN12

#define FocusPWM (PB9)  // TIM4_4 / BluePill.PIN37
#define FocusDIR (PB14) // GPIO / BluePill.PIN21

#define IrisPWM (PB8)   // TIM4_3 / BluePill.PIN36
#define IrisDIR (PB13)  // GPIO / BluePill.PIN22

#define ZoomPWM (PB7)   // TIM4_2 / BluePill.PIN35
#define ZoomDIR (PB12)  // GPIO / BluePill.PIN23

#define CommandUartRx (PB11) // UART3_RX / BluePill.PIN16
#define CommandUartTx (PB10) // UART3_TX / BluePill.PIN16

#define LancUartRx (PA3) // UART2_RX / BluePill.PIN08
#define LancUartTx (PA2) // UART2_TX / BluePill.PIN07
#define LancGPIO   (PA1) // GPIO / BluePill.PIN06


int ledStatus;
uint32_t oldSeconds;
uint32_t oldMillis;
uint32_t oldMicros;
uint32_t oldQuarter;

bool powerPresent;

HardwareSerial commandSerial(CommandUartRx, CommandUartTx);
HardwareSerial lancSerial(LancUartRx, LancUartTx);

void setup() {
  Serial.begin(115200);
  commandSerial.begin(9600);

  pinMode(LancGPIO, INPUT_PULLUP);
  lancSerial.begin(9600, SERIAL_8N2);

  pinMode(LED_BUILTIN, INPUT_PULLUP);

  ledStatus = HIGH;
  digitalWrite(LED_BUILTIN, ledStatus);
  
  analogReference(AR_DEFAULT);
  analogReadResolution(12);

  oldMillis = millis();
  oldMicros = micros();
  oldQuarter = oldMillis / 250;
  oldSeconds = oldMillis / 1000;

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
}

void loop() {
  focusServo.readAdc();
  zoomServo.readAdc();
  irisServo.readAdc();
  uint32_t newMillis = millis();
  if(newMillis != oldMillis){
    oldMillis = newMillis;
    powerPresent = (zoomServo.getAdcValue() > 1100);
    if(powerPresent){
      focusServo.run();
      zoomServo.run();
      irisServo.run();
    }
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
    if(powerPresent){
      ledStatus = (HIGH == ledStatus) ? LOW : HIGH;
      digitalWrite(LED_BUILTIN, ledStatus);
    }
    lancSerial.write("\x55\xAA");
  }
  uint32_t newQuarter = newMillis / 250;
  if(newQuarter != oldQuarter){
    oldQuarter = newQuarter;
    if(!powerPresent){
      ledStatus = (HIGH == ledStatus) ? LOW : HIGH;
      digitalWrite(LED_BUILTIN, ledStatus);
    }
  }
}
