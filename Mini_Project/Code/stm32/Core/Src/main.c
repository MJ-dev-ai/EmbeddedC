/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body for IoT Smart Parking System
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "clcd.h"
#include "MFRC522.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
    uint8_t id[3];
    uint8_t status; // 0: 외부, 1: 주차장 내부
} RFID_Card;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

#define ARR_CNT 5
#define CMD_SIZE 50
#define DIST_THRESHOLD 10.0 // 거리 감지 임계값 (cm)
#define HOLD_TIME 1500      // 차단기 열림 유지 시간 (ms)
#define RESET_DELAY 1500    // 카드 재인식 방지 딜레이 (ms)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c3;
SPI_HandleTypeDef hspi2;
TIM_HandleTypeDef htim3;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart6;

/* USER CODE BEGIN PV */
// 함수 프로토타입 선언
float HCSR04_Read(GPIO_TypeDef* TrigPort, uint16_t TrigPin, GPIO_TypeDef* EchoPort, uint16_t EchoPin);
void Set_Servo_Angle(int angle);
void bluetooth_Event(void);

// 전역 변수
uint32_t last_sensor_tick = 0;
uint32_t object_gone_tick = 0;
uint8_t is_servo_open = 0;
float Distance1 = 0;
float Distance2 = 0;

uint8_t mfrc522_id[5];
uint8_t is_card_present = 0;
uint32_t absent_start_tick = 0;

// UART 수신 버퍼
uint8_t rx2char;
volatile unsigned char rx2Flag = 0;
volatile char rx2Data[CMD_SIZE];

uint8_t btchar;
volatile unsigned char btFlag = 0;
char btData[CMD_SIZE];
char buff[30];

// 등록된 카드 목록
RFID_Card myCards[] = {
    {{0xB3, 0x16, 0x32}, 0}, // 카드 1
    {{0x1D, 0x5F, 0x4A}, 0}, // 카드 2
    {{0x67, 0x17, 0x4B}, 0}  // 카드 3
};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C3_Init(void);
static void MX_SPI2_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART6_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// printf 리다이렉션
PUTCHAR_PROTOTYPE
{
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);
  return ch;
}

// UART 수신 콜백
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART2) // PC 디버깅용
    {
        static int i2 = 0;
        rx2Data[i2] = rx2char;
        if((rx2Data[i2] == '\r') || (rx2Data[i2] == '\n'))
        {
            rx2Data[i2] = '\0';
            rx2Flag = 1;
            i2 = 0;
        }
        else {
            i2++;
            if (i2 >= CMD_SIZE) i2 = 0; // 버퍼 오버플로우 방지
        }
        HAL_UART_Receive_IT(&huart2, &rx2char, 1);
    }
    else if(huart->Instance == USART6) // 블루투스/서버 통신용
    {
        static int i6 = 0;
        btData[i6] = btchar;
        if((btData[i6] == '\n') || (btData[i6] == '\r'))
        {
            btData[i6] = '\0';
            btFlag = 1;
            i6 = 0;
        }
        else {
            i6++;
            if (i6 >= CMD_SIZE) i6 = 0; // 버퍼 오버플로우 방지
        }
        HAL_UART_Receive_IT(&huart6, &btchar, 1);
    }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C3_Init();
  MX_SPI2_Init();
  MX_TIM3_Init();
  MX_USART2_UART_Init();
  MX_USART6_UART_Init();

  /* USER CODE BEGIN 2 */
  // 인터럽트 및 PWM 시작
  HAL_UART_Receive_IT(&huart2, &rx2char, 1);
  HAL_UART_Receive_IT(&huart6, &btchar, 1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);

  // 장치 초기화
  LCD_init(&hi2c3);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET); // RFID CS Pin (NSS) - High로 초기화
  HAL_Delay(50);
  MFRC522_Init();

  // 초기 화면
  LCD_writeStringXY(0, 0, "IoT Smart Park  ");
  LCD_writeStringXY(1, 0, "Waiting...      ");
  printf("System Started\r\n");


  sprintf(buff, "[SQL]GETDB\n");
  HAL_UART_Transmit(&huart6, (uint8_t *)buff, strlen(buff), 100);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      uint32_t current_tick = HAL_GetTick();

      // 1. 초음파 센서 거리 측정 (500ms 주기)
      if (current_tick - last_sensor_tick >= 500) {
    	  Distance1 = HCSR04_Read(Trig_GPIO_Port, Trig_Pin, Echo_GPIO_Port, Echo_Pin);
    	  Distance2 = HCSR04_Read(Trig2_GPIO_Port, Trig2_Pin, Echo2_GPIO_Port, Echo2_Pin);
          last_sensor_tick = current_tick;

          // 차량 출차 감지 (거리가 가까워지면)
          if ((Distance1 > 0 && Distance1 < DIST_THRESHOLD) || (Distance2 > 0 && Distance2 < DIST_THRESHOLD)) {
              if (!is_servo_open) {
                  Set_Servo_Angle(90); // 차단기 열기
                  is_servo_open = 1;
                  LCD_writeStringXY(0, 0, "GoodBye~!       ");
                  LCD_writeStringXY(1, 0, "Drive Safely    ");
              }
              object_gone_tick = current_tick; // 감지되는 동안 시간 갱신
          }
      }

      // 차단기 닫기 로직 (물체가 사라지고 HOLD_TIME 경과 후)
      if (is_servo_open && (current_tick - object_gone_tick >= HOLD_TIME)) {
          Set_Servo_Angle(0); // 차단기 닫기
          is_servo_open = 0;
          LCD_writeStringXY(0, 0, "IoT Smart Park  ");
          LCD_writeStringXY(1, 0, "Waiting...      ");
      }

      // 2. RFID 카드 인식 (200ms 주기 - 반응속도 개선)
      static uint32_t rfid_poll_tick = 0;
      if (current_tick - rfid_poll_tick >= 200) {
          rfid_poll_tick = current_tick;

          uint8_t str[16]; // MFRC522용 임시 버퍼

          // 카드 존재 확인 및 충돌 방지
          if (MFRC522_Request(PICC_REQIDL, str) == MI_OK && MFRC522_Anticoll(mfrc522_id) == MI_OK) {

              if (is_card_present == 0) { // 새로운 카드 태깅 시
                  char* current_action = NULL;

                  // 등록된 카드인지 확인
                  for (int i = 0; i < 3; i++) {
                      if (memcmp(mfrc522_id, myCards[i].id, 3) == 0) {
                          myCards[i].status = !myCards[i].status; // 상태 토글 (입차<->출차)
                          current_action = myCards[i].status ? "ENTER" : "LEAVE";
                          break;
                      }
                  }

                  if (current_action != NULL) {
                      char sql_msg[80];
                      // 프로토콜 포맷: [SQL]ACTION@UID...
                      sprintf(sql_msg, "[SQL]%s@%02X%02X%02X%02X%02X\n",
                              current_action, mfrc522_id[0], mfrc522_id[1], mfrc522_id[2], mfrc522_id[3], mfrc522_id[4]);

                      HAL_UART_Transmit(&huart6, (uint8_t *)sql_msg, strlen(sql_msg), 100);
                      printf("Action Sent: %s\r", sql_msg);

//                      sprintf(buff, "ID: %02X%02X%02X Found", mfrc522_id[0], mfrc522_id[1], mfrc522_id[2]);
//                      LCD_writeStringXY(1, 0, buff);
                  }
                  else {
                      printf("Unknown Card: %02X%02X%02X\n", mfrc522_id[0], mfrc522_id[1], mfrc522_id[2]);
                      LCD_writeStringXY(1, 0, "Unknown Card    ");
                  }
                  is_card_present = 1; // 카드 인식 상태 플래그 설정
                  absent_start_tick = current_tick; // 인식 시점 기록
              }
              else {
                  // 카드가 계속 놓여있는 경우 타이머 갱신
                  absent_start_tick = current_tick;
              }
          }
          else {
              // 카드가 인식되지 않을 때 (떼었을 때)
              if (is_card_present == 1) {
                  // 일정 시간(RESET_DELAY) 동안 카드가 없으면 다시 인식 가능하도록 초기화
                  if (current_tick - absent_start_tick >= RESET_DELAY) {
                      is_card_present = 0;
                      // LCD 원복은 차단기 로직에서 처리하거나 여기서 처리 가능
                  }
              }
          }
      }

      // 3. UART 명령어 처리
      if(rx2Flag) {
          printf("Recv(PC): %s\r\n", rx2Data);
          rx2Flag = 0;
      }

      if(btFlag) {
          bluetooth_Event(); // 블루투스 데이터 파싱
          btFlag = 0;
      }
  }
  /* USER CODE END WHILE */
}

/* USER CODE BEGIN 4 */
// 블루투스 데이터 파싱 함수
void bluetooth_Event()
{
  int i = 0;
  char *pToken;
  char *pArray[ARR_CNT] = {0};
  char recvBuf[CMD_SIZE] = {0};

  strcpy(recvBuf, (char*)btData); // 원본 보존을 위해 복사

  // 구분자 [@] 로 파싱
  pToken = strtok(recvBuf, "[@]");
  while(pToken != NULL)
  {
    pArray[i] = pToken;
    if(++i >= ARR_CNT) break;
    pToken = strtok(NULL, "[@]");
  }

  // 로직: 주차비 정산 완료 메시지 ([CMD]@EXIT@CAR_NUM@FEE)
  if(pArray[1] != NULL && strcmp(pArray[1], "EXIT") == 0)
  {
      printf("Parsing Exit: %s\r\n", btData);
      int fee = atoi(pArray[2]);

      LCD_writeStringXY(0, 0, "Parking Fee     ");
      sprintf(buff, "%5d Won       ", fee);
      LCD_writeStringXY(1, 0, buff);
  }
  // 로직: 서보 모터 강제 제어 ([CMD]@SERVO@ANGLE)
  else if(pArray[1] != NULL && strcmp(pArray[1], "SERVO") == 0)
  {
      int ang = atoi(pArray[2]);
      Set_Servo_Angle(ang);
      printf("Force Servo: %d\r\n", ang);
  }
  if(pArray[1] != NULL && strcmp(pArray[1], "GETDB") == 0) {
	  if (i == 5) {
		  myCards[0].status = atoi(pArray[2]);
		  myCards[1].status = atoi(pArray[3]);
		  myCards[2].status = atoi(pArray[4]);
	  }
  }
}

// 초음파 센서 거리 측정
float HCSR04_Read(GPIO_TypeDef* TrigPort, uint16_t TrigPin, GPIO_TypeDef* EchoPort, uint16_t EchoPin) {
    uint32_t count = 0;

    // Trigger Signal: 10us High Pulse
    HAL_GPIO_WritePin(TrigPort, TrigPin, GPIO_PIN_SET);
    for(int i=0; i<100; i++) __NOP();
    HAL_GPIO_WritePin(TrigPort, TrigPin, GPIO_PIN_RESET);

    // Wait for Echo High
    uint32_t timeout = 10000;
    while (HAL_GPIO_ReadPin(EchoPort, EchoPin) == GPIO_PIN_RESET) {
        if(timeout-- == 0) return 0.0;
    }

    // Measure Pulse Width
    while (HAL_GPIO_ReadPin(EchoPort, EchoPin) == GPIO_PIN_SET) {
        count++;
        if(count > 50000) break;
    }

    return (float)count * 0.017;
}

// 서보 모터 각도 제어 (TIM3 CH1)
void Set_Servo_Angle(int angle) {
    if(angle < 0) angle = 0;
    if(angle > 180) angle = 180;

    uint16_t compare_value = 500 + (angle * 2000 / 180);
//    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, compare_value);
    TIM3->CCR1 = compare_value - 1;
}
/* USER CODE END 4 */

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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C3_Init(void)
{
  hi2c3.Instance = I2C3;
  hi2c3.Init.ClockSpeed = 10000;
  hi2c3.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 84-1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 20000-1;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 500-1;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_TIM_MspPostInit(&htim3);
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 9600;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, SPI2_SDA_Pin|RC522_RST_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(Trig_GPIO_Port, Trig_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SPI2_SDA_Pin RC522_RST_Pin */
  GPIO_InitStruct.Pin = SPI2_SDA_Pin|RC522_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : Trig_Pin */
  GPIO_InitStruct.Pin = Trig_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Trig_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Echo_Pin */
  GPIO_InitStruct.Pin = Echo_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(Echo_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Trig2_Pin */
  GPIO_InitStruct.Pin = Trig2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Trig2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Echo2_Pin */
  GPIO_InitStruct.Pin = Echo2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(Echo2_GPIO_Port, &GPIO_InitStruct);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
