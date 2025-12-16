/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fdcan.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

COM_InitTypeDef BspCOMInit;
__IO uint32_t BspButtonState = BUTTON_RELEASED;

/* USER CODE BEGIN PV */
FDCAN_TxHeaderTypeDef TxHeader;
FDCAN_TxHeaderTypeDef TxHeader_Echo;

FDCAN_RxHeaderTypeDef RxHeader;
FDCAN_RxHeaderTypeDef RxHeader_Echo;
uint8_t TxData[16] = { 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55,
    0xAA, 0x55, 0xAA };
uint8_t RxData[16] = { 0 };
uint8_t RxData_Echo[16] = { 0 };

__IO uint8_t can1_TxCompleteFlag = 0;
__IO uint8_t can1_RxCompleteFlag = 0;

__IO uint8_t can2_TxCompleteFlag = 0;
__IO uint8_t can2_RxCompleteFlag = 0;

uint32_t tmpCount = 0;

FDCAN_ErrorCountersTypeDef FDCAN_ErrorCount_master;
FDCAN_ErrorCountersTypeDef FDCAN_ErrorCount_slave;
FDCAN_ProtocolStatusTypeDef FDCAN_ProtocolStatus_master;
FDCAN_ProtocolStatusTypeDef FDCAN_ProtocolStatus_slave;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void CAN1_SetFilter(void);
void CAN2_SetFilter(void);

void CAN1_SetTxHeader(void);
void CAN2_SetTxHeader(void);

void CAN_Diagnose_Status(FDCAN_HandleTypeDef *hfdcan);
HAL_StatusTypeDef CAN_Setup_Debug_Notifications(FDCAN_HandleTypeDef *hfdcan);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_FDCAN1_Init();
  MX_FDCAN2_Init();
  MX_TIM2_Init();
  MX_FDCAN3_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Initialize led */
  BSP_LED_Init(LED_GREEN);

  /* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

  /* Initialize COM1 port (115200, 8 bits (7-bit data + 1 stop bit), no parity */
  BspCOMInit.BaudRate   = 115200;
  BspCOMInit.WordLength = COM_WORDLENGTH_8B;
  BspCOMInit.StopBits   = COM_STOPBITS_1;
  BspCOMInit.Parity     = COM_PARITY_NONE;
  BspCOMInit.HwFlowCtl  = COM_HWCONTROL_NONE;
  if (BSP_COM_Init(COM1, &BspCOMInit) != BSP_ERROR_NONE)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN BSP */

  /* -- Sample board code to send message over COM1 port ---- */
  printf("Welcome to STM32 world !\r\n");

  /* -- Sample board code to switch on led ---- */
  BSP_LED_On(LED_GREEN);

  /* USER CODE END BSP */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint32_t tmpSystemClock = HAL_RCC_GetSysClockFreq();
  uint32_t tmpPeripheralClock = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_FDCAN);
  printf("System Clock frequency = %lu MHz\r\n", tmpSystemClock / 1000000);
  printf("FDCAN Peripheral Clock frequency = %lu MHz\r\n", tmpPeripheralClock / 1000000);
  HAL_Delay(10);

  /* --- Configure FDCAN1 --- */
  /* Configure FDCAN1 filters */
  CAN1_SetFilter();
  /* Activate FDCAN1 RX FIFO 0 new message notification */
  if(HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK) {
    Error_Handler();
  }
  /* Setup FDCAN1 debug notifications */
  if(CAN_Setup_Debug_Notifications(&hfdcan1) != HAL_OK) {
    Error_Handler();
  }

  /* --- Configure FDCAN2 --- */
  /* Configure FDCAN2 filters */
  CAN2_SetFilter();
  /* Activate FDCAN2 RX FIFO 1 new message notification */
  if(HAL_FDCAN_ActivateNotification(&hfdcan2, FDCAN_IT_RX_FIFO1_NEW_MESSAGE, 0) != HAL_OK) {
    Error_Handler();
  }
  /* Setup FDCAN2 debug notifications */
  if(CAN_Setup_Debug_Notifications(&hfdcan2) != HAL_OK) {
    Error_Handler();
  }

  /* Configure Tx Headers */
  CAN1_SetTxHeader();
  CAN2_SetTxHeader();

  /* Start FDCAN1 */
  if(HAL_FDCAN_Start(&hfdcan1) != HAL_OK) {
    Error_Handler();
  }
  /* Start FDCAN2 */
  if (HAL_FDCAN_Start(&hfdcan2) != HAL_OK) {
    Error_Handler();
  }

  while (1)
  {

    /* -- Sample board code for User push-button in interrupt mode ---- */
    if (BspButtonState == BUTTON_PRESSED)
    {
      /* Update button state */
      BspButtonState = BUTTON_RELEASED;
      /* -- Sample board code to toggle led ---- */
      BSP_LED_Toggle(LED_GREEN);

      /* ..... Perform your action ..... */
//      HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader, TxData);
    }
    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader, TxData);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if (can2_RxCompleteFlag == 1) {
      can2_RxCompleteFlag = 0;
      printf("FDCAN2 Received ID: 0x%03lX\r\n", RxHeader_Echo.Identifier);
      HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2, &TxHeader_Echo, RxData_Echo);
    }

    if (can1_RxCompleteFlag == 1) {
      tmpCount++;
      can1_RxCompleteFlag = 0;
      printf("[%lu] FDCAN1 Received ID: 0x%03lX\r\n",tmpCount, RxHeader.Identifier);
    }

    CAN_Diagnose_Status(&hfdcan1);
    CAN_Diagnose_Status(&hfdcan2);

    HAL_Delay(100);;
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV3;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV4;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void CAN1_SetFilter(void) {
  FDCAN_FilterTypeDef sFilterConfig;

  sFilterConfig.IdType = FDCAN_STANDARD_ID;
  sFilterConfig.FilterIndex = 0;
  sFilterConfig.FilterType = FDCAN_FILTER_MASK;
  sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
  sFilterConfig.FilterID1 = 0x200;
  sFilterConfig.FilterID2 = 0x7FF;
  if(HAL_FDCAN_ConfigFilter(&hfdcan1, &sFilterConfig) != HAL_OK) {
    Error_Handler();
  }

  if(HAL_FDCAN_ConfigGlobalFilter(&hfdcan1, FDCAN_REJECT, FDCAN_REJECT, FDCAN_REJECT_REMOTE,
  FDCAN_REJECT_REMOTE) != HAL_OK) {
    Error_Handler();
  }
}

void CAN2_SetFilter(void) {
  FDCAN_FilterTypeDef sFilterConfig;

  sFilterConfig.IdType = FDCAN_STANDARD_ID;
  sFilterConfig.FilterIndex = 0;
  sFilterConfig.FilterType = FDCAN_FILTER_MASK;
  sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO1;
  sFilterConfig.FilterID1 = 0x100;
  sFilterConfig.FilterID2 = 0x7FF;
  if(HAL_FDCAN_ConfigFilter(&hfdcan2, &sFilterConfig) != HAL_OK) {
    Error_Handler();
  }

  if(HAL_FDCAN_ConfigGlobalFilter(&hfdcan2, FDCAN_REJECT, FDCAN_REJECT, FDCAN_REJECT_REMOTE,
  FDCAN_REJECT_REMOTE) != HAL_OK) {
    Error_Handler();
  }
}

void CAN1_SetTxHeader(void) {
  TxHeader.Identifier = 0x100;
  TxHeader.IdType = FDCAN_STANDARD_ID;
  TxHeader.TxFrameType = FDCAN_DATA_FRAME;
  TxHeader.DataLength = FDCAN_DLC_BYTES_16;
  TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  TxHeader.BitRateSwitch = FDCAN_BRS_ON;
  TxHeader.FDFormat = FDCAN_FD_CAN;
  TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
  TxHeader.MessageMarker = 0;
}

void CAN2_SetTxHeader(void) {
  TxHeader_Echo.Identifier = 0x200;
  TxHeader_Echo.IdType = FDCAN_STANDARD_ID;
  TxHeader_Echo.TxFrameType = FDCAN_DATA_FRAME;
  TxHeader_Echo.DataLength = FDCAN_DLC_BYTES_16;
  TxHeader_Echo.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  TxHeader_Echo.BitRateSwitch = FDCAN_BRS_ON;
  TxHeader_Echo.FDFormat = FDCAN_FD_CAN;
  TxHeader_Echo.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
  TxHeader_Echo.MessageMarker = 0;
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs) {
  if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET) {
    HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &RxHeader, RxData);
    can1_RxCompleteFlag = 1;
  }
}

void HAL_FDCAN_RxFifo1Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo1ITs) {
  if ((RxFifo1ITs & FDCAN_IT_RX_FIFO1_NEW_MESSAGE) != RESET) {
    HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO1, &RxHeader_Echo, RxData_Echo);
    can2_RxCompleteFlag = 1;
  }
}

void CAN_Diagnose_Status(FDCAN_HandleTypeDef *hfdcan) {
  FDCAN_ProtocolStatusTypeDef FDCAN_Status = { 0 };
  FDCAN_ErrorCountersTypeDef FDCAN_Errors = { 0 };
  char instance_char[10] = { 0 };
  uint8_t tmpTxErrCnt = 0;
  uint8_t tmpRxErrCnt = 0;

  // 1. 상태 읽기
  HAL_FDCAN_GetProtocolStatus(hfdcan, &FDCAN_Status);
  HAL_FDCAN_GetErrorCounters(hfdcan, &FDCAN_Errors);
  tmpTxErrCnt = (uint8_t)FDCAN_Errors.TxErrorCnt;
  tmpRxErrCnt = (uint8_t)FDCAN_Errors.RxErrorCnt;

  // 에러가 없으면 조기 리턴 (불필요한 로그 방지, 필요 시 주석 처리)
  if (FDCAN_Status.LastErrorCode == FDCAN_PROTOCOL_ERROR_NONE &&
      FDCAN_Status.Activity != 0 &&
      FDCAN_Errors.TxErrorCnt == 0 && FDCAN_Errors.RxErrorCnt == 0) {
    return;
  }

  // 인스턴스 구분 출력
  if (hfdcan->Instance == FDCAN1) strcpy(instance_char, "FDCAN1");
  else if (hfdcan->Instance == FDCAN2) strcpy(instance_char, "FDCAN2");

  // 2. 심각도 순서로 버스 상태 체크 (Bus-Off -> Passive -> Warning)
  if (FDCAN_Status.BusOff) {
    printf("[%s][CRITICAL] Bus-Off (Comm. Disabled)\r\n", instance_char);
    // Bus-Off 복구 로직 필요 시 여기에 추가 (ex. HAL_FDCAN_Init)
  }
  else if (FDCAN_Status.ErrorPassive) {
    printf("[%s][STATE] Error Passive (Only Listen or Passive Flag)\r\n", instance_char);
  }
  else if (FDCAN_Status.Warning) {
    printf("[%s][STATE] Error Warning (TEC or REC >= 96)\r\n", instance_char);
  }
  else {
    // printf("[STATE] Error Active (Normal)\r\n"); // 필요 시 주석 해제
  }

  // 3. 에러 카운터 상세 출력 (상태 플래그와 무관하게 값 확인)
  if (FDCAN_Errors.TxErrorCnt > 0 || FDCAN_Errors.RxErrorCnt > 0) {
    printf("[%s]Counters - TEC: %d, REC: %d\r\n", instance_char, tmpTxErrCnt, tmpRxErrCnt);
  }

  // 4. Last Error Code (LEC) 분석
  // LEC는 읽으면 클리어되거나 바뀔 수 있으므로 0(No Error)이 아닐 때만 출력
  if (FDCAN_Status.LastErrorCode != FDCAN_PROTOCOL_ERROR_NO_CHANGE &&
      FDCAN_Status.LastErrorCode != FDCAN_PROTOCOL_ERROR_NONE) {

    printf("LEC Reason: ");
    switch (FDCAN_Status.LastErrorCode) {
      case FDCAN_PROTOCOL_ERROR_STUFF: printf("Stuff Error\r\n"); break;
      case FDCAN_PROTOCOL_ERROR_FORM:  printf("Form Error\r\n"); break;
      case FDCAN_PROTOCOL_ERROR_ACK:   printf("Ack Error\r\n"); break;
      case FDCAN_PROTOCOL_ERROR_BIT1:  printf("Bit1 (Recessive sent, Dominant read)\r\n"); break;
      case FDCAN_PROTOCOL_ERROR_BIT0:  printf("Bit0 (Dominant sent, Recessive read)\r\n"); break;
      case FDCAN_PROTOCOL_ERROR_CRC:   printf("CRC Error\r\n"); break;
      default: printf("[%s]Unknown(0x%lx)\r\n", instance_char, FDCAN_Status.LastErrorCode); break;
    }
  }
}

/* 인터럽트 활성화 설정 (Init 단계에 추가 필요) */
HAL_StatusTypeDef CAN_Setup_Debug_Notifications(FDCAN_HandleTypeDef *hfdcan) {
  // Bus-Off, Error Passive, Protocol Error 발생 시 인터럽트 발생
  HAL_FDCAN_ActivateNotification(hfdcan, FDCAN_IT_BUS_OFF |
                                         FDCAN_IT_ERROR_PASSIVE |
                                         FDCAN_IT_ERROR_WARNING, 0);
  return HAL_OK;
}

void HAL_FDCAN_ErrorStatusCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t ErrorStatusITs) {
  // 실제 구현 시: printf 대신 에러 플래그(global variable)를 set 하고 메인에서 처리 권장
  if (ErrorStatusITs & FDCAN_IT_BUS_OFF) {
    // 긴급 정지 로직
  }
  else if (ErrorStatusITs & FDCAN_IT_ERROR_PASSIVE) {
    // 패시브 상태 로직
  }
  else if (ErrorStatusITs & FDCAN_IT_ERROR_WARNING) {
    // 경고 상태 로직
  }
  // 디버깅 단계에서는 breakpoint를 걸어 상태 확인
}
/* USER CODE END 4 */

/**
  * @brief BSP Push Button callback
  * @param Button Specifies the pressed button
  * @retval None
  */
void BSP_PB_Callback(Button_TypeDef Button)
{
  if (Button == BUTTON_USER)
  {
    BspButtonState = BUTTON_PRESSED;
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();

  while (1)
  {
    BSP_LED_Toggle(LED_GREEN);
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
    printf("Wrong parameters value: file %s on line %d\r\n", file, line);*/
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
