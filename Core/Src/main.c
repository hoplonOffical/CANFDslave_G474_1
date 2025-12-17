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
#define DEBUG_MODE
#define SystemCoreClock_Hz  160000000U
#define FDCAN_PCLK_Hz       80000000U
#define MegaUnit            1000000U

#define MASTER_ID          0x100

#define SLAVE_ID_BASE      0x200
#define SLAVE_ID_END       0x2FF
#define SLAVE_ID_ECHO      0x300

// CAN slave ID는 0x200 + 지정된 node 번호를 부여
// node1: 0x201, node2: 0x202, ...
// 노드 번호를 매크로로 보드마다 지정
// 자동화된 형태로 SLAVE_ID를 설정하도록 하는 방법 알려줘
#ifndef NODE_NUMBER
  #define NODE_NUMBER  1U // 기본값 1, 빌드 시 -DNODE_NUMBER=x 로 변경 가능
#endif
#define SLAVE_ID         (SLAVE_ID_BASE + NODE_NUMBER)
/* Example: NODE_NUMBER가 3으로 설정된 경우 SLAVE_ID는 0x203이 됨 */
/* Example: NODE_NUMBER가 5으로 설정된 경우 SLAVE_ID는 0x205이 됨 */
// SLAVE_ID가 SLAVE_ID_END를 초과하지 않도록 주의 필요
#if SLAVE_ID > SLAVE_ID_END
  #error "SLAVE_ID exceeds the maximum allowed value. Please check NODE_NUMBER."
#endif

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

COM_InitTypeDef BspCOMInit;
__IO uint32_t BspButtonState = BUTTON_RELEASED;

/* USER CODE BEGIN PV */
FDCAN_TxHeaderTypeDef TxHeader;
FDCAN_RxHeaderTypeDef RxHeader;

uint8_t TxData[16] = { 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55,
    0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA };
uint8_t RxData[16] = { 0 };

__IO uint8_t TxCompleteFlag = 0;
__IO uint8_t RxCompleteFlag = 0;

uint32_t tmpCount = 0;

FDCAN_ErrorCountersTypeDef FDCAN_ErrorCount_slave;
FDCAN_ProtocolStatusTypeDef FDCAN_ProtocolStatus_slave;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void canSetInitFilter(void);
void canSetInitInterrupts(void);
void canSetInitTxHeader(void);
void canSetInitTxPayload(void);
void canStart(void);

ErrorStatus canDiagnoseStatus(FDCAN_HandleTypeDef *hfdcan);
HAL_StatusTypeDef canSetupDebugNotifications(FDCAN_HandleTypeDef *hfdcan);
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
  canSetInitFilter();
  /* Configure FDCAN1 interrupts */
  canSetInitInterrupts();
  /* Configure Tx Headers */
  canSetInitTxHeader();
  /* Configure Tx Payload */  
  canSetInitTxPayload();

  /* Start FDCAN1 module */
  canStart();
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if(RxCompleteFlag) {
      RxCompleteFlag = 0;
      // 수신된 메시지 처리
      // 예: 수신된 데이터 출력
      tmpCount++;
      if(tmpCount % 500 == 0) {
        printf("[%lu] FDCAN1 Received ID: 0x%03lX\r\n",tmpCount, RxHeader.Identifier);
      }
    }

    canDiagnoseStatus(&hfdcan1);
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

void canSetInitFilter(void) {
  FDCAN_FilterTypeDef sFilterConfig;

  sFilterConfig.IdType = FDCAN_STANDARD_ID;
  sFilterConfig.FilterIndex = 0;
  sFilterConfig.FilterType = FDCAN_FILTER_MASK;
  sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
  sFilterConfig.FilterID1 = MASTER_ID;
  sFilterConfig.FilterID2 = 0x7FF; // 모든 비트 매칭
  if(HAL_FDCAN_ConfigFilter(&hfdcan1, &sFilterConfig) != HAL_OK) {
    Error_Handler();
  }

  if(HAL_FDCAN_ConfigGlobalFilter(&hfdcan1, FDCAN_REJECT, FDCAN_REJECT, FDCAN_REJECT_REMOTE,
  FDCAN_REJECT_REMOTE) != HAL_OK) {
    Error_Handler();
  }
}

void canSetInitInterrupts(void) {
    /* Activate FDCAN1 RX FIFO 0 new message notification */
    if(HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK) {
      Error_Handler();
    }
    /* Setup FDCAN1 debug notifications */
    if(canSetupDebugNotifications(&hfdcan1) != HAL_OK) {
      Error_Handler();
    }
  }

void canSetInitTxHeader(void) {
  //TxHeader.Identifier = SLAVE_ID; // 노드 번호에 따라 다른 ID 설정, ex. 0x201, 0x202, ...
  TxHeader.Identifier = 0x200; // 테스트용으로 마스터 ID로 설정
  TxHeader.IdType = FDCAN_STANDARD_ID;
  TxHeader.TxFrameType = FDCAN_DATA_FRAME;
  TxHeader.DataLength = FDCAN_DLC_BYTES_16;
  TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  TxHeader.BitRateSwitch = FDCAN_BRS_ON;
  TxHeader.FDFormat = FDCAN_FD_CAN;
  TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
  TxHeader.MessageMarker = 0;
}

void canSetInitTxPayload(void) {
  /*for (uint8_t i = 0; i < 16; i++) {
    TxData[i] = (uint8_t)(i + NODE_NUMBER); // 노드 번호에 따라 다른 데이터 전송
  }*/
}

void canStart(void) {
  if(HAL_FDCAN_Start(&hfdcan1) != HAL_OK) {
    Error_Handler();
  }
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs) {
  if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET) {
    HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &RxHeader, RxData);
    RxCompleteFlag = 1;
  }
}

ErrorStatus canDiagnoseStatus(FDCAN_HandleTypeDef *hfdcan) {
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
    return SUCCESS;
  }

  /*// 인스턴스 구분 출력
  if (hfdcan->Instance == FDCAN1) strcpy(instance_char, "FDCAN1");*/

  // 2. 심각도 순서로 버스 상태 체크 (Bus-Off -> Passive -> Warning)
  if (FDCAN_Status.BusOff) {
    return ERROR;
    //printf("[%s][CRITICAL] Bus-Off (Comm. Disabled)\r\n", instance_char);
    // Bus-Off 복구 로직 필요 시 여기에 추가 (ex. HAL_FDCAN_Init)
  }
  else if (FDCAN_Status.ErrorPassive) {
    return ERROR;
    //printf("[%s][STATE] Error Passive (Only Listen or Passive Flag)\r\n", instance_char);
  }
  else if (FDCAN_Status.Warning) {
    return ERROR;
    //printf("[%s][STATE] Error Warning (TEC or REC >= 96)\r\n", instance_char);
  }
  else {
    return SUCCESS;
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
HAL_StatusTypeDef canSetupDebugNotifications(FDCAN_HandleTypeDef *hfdcan) {
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
