/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "logo.h"
#include "ruFonts.h"
#include "ssd1306.h"
#include "stdio.h"
#include <math.h>

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
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

osThreadId readKeyTaskHandle;
osThreadId guiTaskHandle;
osThreadId driverTaskHandle;
osMessageQId buttonPressedQueueHandle;
osTimerId cancelButtonHandle;
osTimerId screenTimeoutTimerHandle;
/* USER CODE BEGIN PV */
uint16_t adcBatteryVoltage = 0;

uint8_t guiState = 0; //motor settings
uint8_t speed = 5;
uint8_t sleep = 60;
uint8_t corner = 45;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_ADC1_Init(void);
void StartReadKeyTask(void const * argument);
void StartGuiTask(void const * argument);
void StartDriverTask(void const * argument);
void cancelButtonCallback(void const * argument);
void screenTimeoutCallback(void const * argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int _write(int file, char *ptr, int len)
{
  (void)file;
  int DataIdx;

  for (DataIdx = 0; DataIdx < len; DataIdx++)
  {
    ITM_SendChar(*ptr++);
  }
  return len;
}
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
  MX_I2C1_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */

  ssd1306_Init();
  ssd1306_Fill(Black);
  ssd1306_DrawBitmap(43, 2, logo, 42, 42, White);
  ssd1306_SetCursor(11, 46);
  ssd1306_WriteString("АвтоШашлык V0.1", RuFont_7x13, White);
  ssd1306_UpdateScreen();
  HAL_ADCEx_Calibration_Start(&hadc1);
  //HAL_Delay(2000);

  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* Create the timer(s) */
  /* definition and creation of cancelButton */
  osTimerDef(cancelButton, cancelButtonCallback);
  cancelButtonHandle = osTimerCreate(osTimer(cancelButton), osTimerOnce, NULL);

  /* definition and creation of screenTimeoutTimer */
  osTimerDef(screenTimeoutTimer, screenTimeoutCallback);
  screenTimeoutTimerHandle = osTimerCreate(osTimer(screenTimeoutTimer), osTimerOnce, NULL);

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* definition and creation of buttonPressedQueue */
  osMessageQDef(buttonPressedQueue, 8, uint8_t);
  buttonPressedQueueHandle = osMessageCreate(osMessageQ(buttonPressedQueue), NULL);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of readKeyTask */
  osThreadDef(readKeyTask, StartReadKeyTask, osPriorityAboveNormal, 0, 128);
  readKeyTaskHandle = osThreadCreate(osThread(readKeyTask), NULL);

  /* definition and creation of guiTask */
  osThreadDef(guiTask, StartGuiTask, osPriorityNormal, 0, 128);
  guiTaskHandle = osThreadCreate(osThread(guiTask), NULL);

  /* definition and creation of driverTask */
  osThreadDef(driverTask, StartDriverTask, osPriorityHigh, 0, 128);
  driverTaskHandle = osThreadCreate(osThread(driverTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  osThreadSuspend(driverTaskHandle);
  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, buzzer_Pin|drv_step_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(drv_en_GPIO_Port, drv_en_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : buzzer_Pin */
  GPIO_InitStruct.Pin = buzzer_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(buzzer_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : drv_en_Pin */
  GPIO_InitStruct.Pin = drv_en_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(drv_en_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : drv_step_Pin */
  GPIO_InitStruct.Pin = drv_step_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(drv_step_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : btn_bottom_Pin */
  GPIO_InitStruct.Pin = btn_bottom_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(btn_bottom_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : btn_left_Pin btn_ok_Pin btn_right_Pin btn_top_Pin */
  GPIO_InitStruct.Pin = btn_left_Pin|btn_ok_Pin|btn_right_Pin|btn_top_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartReadKeyTask */
/**
  * @brief  Function implementing the readKeyTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartReadKeyTask */
void StartReadKeyTask(void const * argument)
{
  /* USER CODE BEGIN 5 */
  #define SCREEN_TIMEOUT 30000
  osTimerStart(screenTimeoutTimerHandle, SCREEN_TIMEOUT);
  /* Infinite loop */
  for(;;)
  {

	if (HAL_GPIO_ReadPin(btn_bottom_GPIO_Port, btn_bottom_Pin) == 0) {
	    if (ssd1306_GetDisplayOn()==0) osMessagePut(buttonPressedQueueHandle, 6, 100); else osMessagePut(buttonPressedQueueHandle, 0, 100);
	    while(HAL_GPIO_ReadPin(btn_bottom_GPIO_Port, btn_bottom_Pin) == 0)
	    {
		    osDelay(1);
	    }
	    osTimerStart(screenTimeoutTimerHandle, SCREEN_TIMEOUT);
	}
	else if (HAL_GPIO_ReadPin(btn_top_GPIO_Port, btn_top_Pin) == 0) {
	    if (ssd1306_GetDisplayOn()==0) osMessagePut(buttonPressedQueueHandle, 6, 100); else osMessagePut(buttonPressedQueueHandle, 1, 100);
	    while(HAL_GPIO_ReadPin(btn_top_GPIO_Port, btn_top_Pin) == 0)
	    {
		    osDelay(1);
	    }
	    osTimerStart(screenTimeoutTimerHandle, SCREEN_TIMEOUT);
	}
	else if (HAL_GPIO_ReadPin(btn_left_GPIO_Port, btn_left_Pin) == 0) {
	    if (ssd1306_GetDisplayOn()==0) osMessagePut(buttonPressedQueueHandle, 6, 100); else osMessagePut(buttonPressedQueueHandle, 2, 100);
	    while(HAL_GPIO_ReadPin(btn_left_GPIO_Port, btn_left_Pin) == 0)
	    {
		    osDelay(1);
	    }
	    osTimerStart(screenTimeoutTimerHandle, SCREEN_TIMEOUT);
	}
	else if (HAL_GPIO_ReadPin(btn_right_GPIO_Port, btn_right_Pin) == 0) {
	    if (ssd1306_GetDisplayOn()==0) osMessagePut(buttonPressedQueueHandle, 6, 100); else osMessagePut(buttonPressedQueueHandle, 3, 100);
	    while(HAL_GPIO_ReadPin(btn_right_GPIO_Port, btn_right_Pin) == 0)
	    {
		    osDelay(1);
	    }
	    osTimerStart(screenTimeoutTimerHandle, SCREEN_TIMEOUT);
	}
	else if (HAL_GPIO_ReadPin(btn_ok_GPIO_Port, btn_ok_Pin) == 0) {
	    osTimerStart(cancelButtonHandle, 1000);
	    while(HAL_GPIO_ReadPin(btn_ok_GPIO_Port, btn_ok_Pin) == 0)
	    {
		    osDelay(1);
	    }
	    osTimerStart(screenTimeoutTimerHandle, SCREEN_TIMEOUT);
	    if (xTimerIsTimerActive(cancelButtonHandle) != pdFALSE) {
		if (ssd1306_GetDisplayOn()==0) osMessagePut(buttonPressedQueueHandle, 6, 100); else osMessagePut(buttonPressedQueueHandle, 4, 100);
		//short press
	    }
	    osTimerStop(cancelButtonHandle);
	}

    osDelay(10);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartGuiTask */
/**
* @brief Function implementing the guiTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartGuiTask */
void StartGuiTask(void const * argument)
{
  /* USER CODE BEGIN StartGuiTask */
  osEvent btnEvent;
  uint8_t selectedMode = 0;
  uint8_t intermittentModeMenuPointer = 0;
  /* Infinite loop */

  for(;;)
  {
      HAL_ADC_Start(&hadc1); // запускаем преобразование сигнала АЦП
      HAL_ADC_PollForConversion(&hadc1, 100); // ожидаем окончания преобразования
      adcBatteryVoltage = HAL_ADC_GetValue(&hadc1); // читаем полученное значение в переменную adc
      HAL_ADC_Stop(&hadc1); // останавливаем АЦП (не обязательно)
      printf("ADC %d\n", adcBatteryVoltage);

      ssd1306_Fill(Black);
      ssd1306_DrawRectangle(0, 0, 127, 63, White);

      if(adcBatteryVoltage > 3523)
	{
	  ssd1306_DrawRectangle(102, 3, 122, 12, White);
	  ssd1306_FillRectangle(104, 5, 108, 10, White);
	  ssd1306_FillRectangle(110, 5, 114, 10, White);
	  ssd1306_FillRectangle(116, 5, 120, 10, White);
	  ssd1306_DrawRectangle(123, 6, 124, 9, White);
	}
      else if(adcBatteryVoltage > 3237 && adcBatteryVoltage < 3522)
	{
	  ssd1306_DrawRectangle(102, 3, 122, 12, White);
	  ssd1306_FillRectangle(104, 5, 108, 10, White);
	  ssd1306_FillRectangle(110, 5, 114, 10, White);
	  ssd1306_DrawRectangle(123, 6, 124, 9, White);
	}
      else if(adcBatteryVoltage > 3047 && adcBatteryVoltage < 3236)
	{
	  ssd1306_DrawRectangle(102, 3, 122, 12, White);
	  ssd1306_FillRectangle(104, 5, 108, 10, White);
	  ssd1306_DrawRectangle(123, 6, 124, 9, White);
	}
      else if(adcBatteryVoltage < 3046)
	{
	  ssd1306_DrawRectangle(102, 3, 122, 12, White);
	  ssd1306_DrawRectangle(123, 6, 124, 9, White);
	}

      btnEvent = osMessageGet(buttonPressedQueueHandle, 100);

      switch (guiState) {
	case 0: // mode select
	  ssd1306_SetCursor(1,1);
	  ssd1306_WriteString("Выбор режима", RuFont_7x13, Black);
	  if (selectedMode == 0) {
	    ssd1306_SetCursor(4,16);
	    ssd1306_WriteString("> Постоянный", RuFont_7x13, White);
	    ssd1306_SetCursor(4,30);
	    ssd1306_WriteString("  Прерывистый", RuFont_7x13, White);
	  } else if (selectedMode == 1) {
	    ssd1306_SetCursor(4,16);
	    ssd1306_WriteString("  Постоянный", RuFont_7x13, White);
	    ssd1306_SetCursor(4,30);
	    ssd1306_WriteString("> Прерывистый", RuFont_7x13, White);
	  }
	  if (btnEvent.status == osEventMessage)
	  {
	    if (btnEvent.value.v == 0 || btnEvent.value.v == 1)
	    { //bottom or top
	      selectedMode = !selectedMode;
	    } else if (btnEvent.value.v == 4)
	    { //ok
	      if (selectedMode == 0)
	      {
		  guiState = 1;
	      } else if (selectedMode == 1)
	      {
		  guiState = 2;
	      }
	    }
	  }
	  break;
	case 1:
	  osThreadResume(driverTaskHandle);
	  ssd1306_SetCursor(1,1);
	  ssd1306_WriteString("Пост. реж.", RuFont_7x13, Black);
	  ssd1306_SetCursor(4,16);
	  ssd1306_WriteString("> Скорость: ", RuFont_7x13, White);
	  ssd1306_SetCursor(88,16);
	  char speedStr[3];
	  sprintf(speedStr, "%d", speed);
	  ssd1306_WriteString(speedStr, RuFont_7x13, White);
	  if (btnEvent.status == osEventMessage)
	  {
	    if (btnEvent.value.v == 2)
	    { //left
		if(speed == 1)
		{
		  speed = 10;
		}
		else
		{
		  speed--;
		}

	    } else if ( btnEvent.value.v == 3)
	      {//right
		if(speed == 10)
		{
		  speed = 1;
		}
		else
		{
		  speed++;
		}
	      }
	      else if (btnEvent.value.v == 5)
	    { //cancel
		guiState = 0;
		osThreadSuspend(driverTaskHandle);
		HAL_GPIO_WritePin(drv_en_GPIO_Port, drv_en_Pin, 1);
	    }
	  }
	  break;
	case 2:
	  osThreadResume(driverTaskHandle);
	  ssd1306_SetCursor(1,1);
	  ssd1306_WriteString("Прерыв. реж.", RuFont_7x13, Black);
	  if(intermittentModeMenuPointer == 0)
	   {
	      ssd1306_SetCursor(4,16);
	      ssd1306_WriteString("> Задержка: ", RuFont_7x13, White);
	      ssd1306_SetCursor(88,16);

	      uint8_t min = sleep / 60;
	      uint8_t sec = sleep % 60;

	      char minStr[2];
	      char secStr[2];

	      sprintf(minStr, "%d", min);
	      sprintf(secStr, "%d", sec);

	      char timeStr[5];

	      if(min <= 9)
	      {
		timeStr[0] = '0';
		timeStr[1] = minStr[0];
	      }
	      else
	      {
		timeStr[0] = minStr[0];
		timeStr[1] = timeStr[1];
	      }

	      timeStr[2] = ':';

	      if(sec <= 9)
	      {
		timeStr[3] = '0';
		timeStr[4] = secStr[0];
	      }
	      else
	      {
		timeStr[3] = secStr[0];
		timeStr[4] = secStr[1];
	      }

	      ssd1306_WriteString(timeStr, RuFont_7x13, White);
	      ssd1306_SetCursor(4,30);
	      ssd1306_WriteString("  Угол: ", RuFont_7x13, White);
	      ssd1306_SetCursor(56, 30);
	      char cornerStr[5];
	      sprintf(cornerStr, "%d", corner);
	      ssd1306_WriteString(cornerStr, RuFont_7x13, White);
	   }
	  else if(intermittentModeMenuPointer == 1)
	  {
	      ssd1306_SetCursor(4,16);
	      ssd1306_WriteString("  Задержка: ", RuFont_7x13, White);
	      ssd1306_SetCursor(88,16);

	      uint8_t min = sleep / 60;
	      uint8_t sec = sleep % 60;

	      char minStr[2];
	      char secStr[2];

	      sprintf(minStr, "%d", min);
	      sprintf(secStr, "%d", sec);

	      char timeStr[5];

	      if(min <= 9)
	      {
		timeStr[0] = '0';
		timeStr[1] = minStr[0];
	      }
	      else
	      {
		timeStr[0] = minStr[0];
		timeStr[1] = timeStr[1];
	      }

	      timeStr[2] = ':';

	      if(sec <= 9)
	      {
		timeStr[3] = '0';
		timeStr[4] = secStr[0];
	      }
	      else
	      {
		timeStr[3] = secStr[0];
		timeStr[4] = secStr[1];
	      }

	      ssd1306_WriteString(timeStr, RuFont_7x13, White);
	      ssd1306_SetCursor(4,30);
	      ssd1306_WriteString("> Угол: ", RuFont_7x13, White);
	      ssd1306_SetCursor(56, 30);
	      char cornerStr[3];
	      sprintf(cornerStr, "%d", corner);
	      ssd1306_WriteString(cornerStr, RuFont_7x13, White);
	  }
	  if (btnEvent.status == osEventMessage)
	  {
	    if (btnEvent.value.v == 0 || btnEvent.value.v == 1)
	    { //bottom or top
		intermittentModeMenuPointer = !intermittentModeMenuPointer;
	    } else if (btnEvent.value.v == 3)
	    {//right
		xTaskAbortDelay(driverTaskHandle);
		if(intermittentModeMenuPointer == 0)
		  {
		    if(sleep == 250)
		      sleep = 10;
		    else
		      sleep+=10;
		  }
		else if(intermittentModeMenuPointer == 1)
		  {
		    if(corner == 45)
			corner = 90;
		    else if(corner == 90)
		      corner = 45;
		  }
	    } else if (btnEvent.value.v == 2)
	      { //left
		xTaskAbortDelay(driverTaskHandle);
		if(intermittentModeMenuPointer == 0)
		  {
		    if(sleep == 10)
		      sleep = 250;
		    else
		      sleep-=10;
		  }
		else if(intermittentModeMenuPointer == 1)
		  {
		    if(corner == 45)
			corner = 90;
		    else if(corner == 90)
		      corner = 45;
		  }
	      }
	    else if (btnEvent.value.v == 5)
	    { //cancel
		guiState = 0;
		HAL_GPIO_WritePin(drv_en_GPIO_Port, drv_en_Pin, 1);
		osThreadSuspend(driverTaskHandle);

	    }
	  }
	  break;

	}

	if (btnEvent.status == osEventMessage)
	{
	  if (btnEvent.value.v == 0) { //bottom
		  HAL_GPIO_WritePin(buzzer_GPIO_Port, buzzer_Pin, 1);
		  osDelay(30);
		  HAL_GPIO_WritePin(buzzer_GPIO_Port, buzzer_Pin, 0);
	  } else if (btnEvent.value.v == 1) { //top
		  HAL_GPIO_WritePin(buzzer_GPIO_Port, buzzer_Pin, 1);
		  osDelay(30);
		  HAL_GPIO_WritePin(buzzer_GPIO_Port, buzzer_Pin, 0);
	  } else if (btnEvent.value.v == 2) { //left
		  HAL_GPIO_WritePin(buzzer_GPIO_Port, buzzer_Pin, 1);
		  osDelay(30);
		  HAL_GPIO_WritePin(buzzer_GPIO_Port, buzzer_Pin, 0);
	  } else if (btnEvent.value.v == 3) { //right
		  HAL_GPIO_WritePin(buzzer_GPIO_Port, buzzer_Pin, 1);
		  osDelay(30);
		  HAL_GPIO_WritePin(buzzer_GPIO_Port, buzzer_Pin, 0);
	  } else if (btnEvent.value.v == 4) { //ok
		  HAL_GPIO_WritePin(buzzer_GPIO_Port, buzzer_Pin, 1);
		  osDelay(50);
		  HAL_GPIO_WritePin(buzzer_GPIO_Port, buzzer_Pin, 0);
		  osDelay(50);
		  HAL_GPIO_WritePin(buzzer_GPIO_Port, buzzer_Pin, 1);
		  osDelay(30);
		  HAL_GPIO_WritePin(buzzer_GPIO_Port, buzzer_Pin, 0);
	  } else if (btnEvent.value.v == 5) { //cancel
		  HAL_GPIO_WritePin(buzzer_GPIO_Port, buzzer_Pin, 1);
		  osDelay(250);
		  HAL_GPIO_WritePin(buzzer_GPIO_Port, buzzer_Pin, 0);
	  } else if (btnEvent.value.v == 6) { //displayUnlock
		  ssd1306_SetDisplayOn(1);
	  }

	}

	ssd1306_UpdateScreen();
	osDelay(10);//
  }
  /* USER CODE END StartGuiTask */
}

/* USER CODE BEGIN Header_StartDriverTask */
/**
* @brief Function implementing the driverTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartDriverTask */
void StartDriverTask(void const * argument)
{
  /* USER CODE BEGIN StartDriverTask */
  /* Infinite loop */
  for(;;)
  {
    HAL_GPIO_WritePin(drv_en_GPIO_Port, drv_en_Pin, 0);
    if (guiState == 1)  { //Постоянный
	HAL_GPIO_TogglePin(drv_step_GPIO_Port, drv_step_Pin);
	osDelay(fabs(speed-11));
    } else

    if (guiState == 2)  { //Прерывистый
      switch (corner) {
	case 45:
	  for (int var = 0; var < 800; var++) {
	    HAL_GPIO_TogglePin(drv_step_GPIO_Port, drv_step_Pin);
	    osDelay(1);
	  }
	  break;
	case 90:
	  for (int var = 0; var < 1600; var++) {
	    HAL_GPIO_TogglePin(drv_step_GPIO_Port, drv_step_Pin);
	    osDelay(1);
	  }
	  break;
      }
      osDelay(sleep*1000);
    }
  }
  /* USER CODE END StartDriverTask */
}

/* cancelButtonCallback function */
void cancelButtonCallback(void const * argument)
{
  /* USER CODE BEGIN cancelButtonCallback */
  if (ssd1306_GetDisplayOn()==0) osMessagePut(buttonPressedQueueHandle, 6, 100); else osMessagePut(buttonPressedQueueHandle, 5, 100);
  /* USER CODE END cancelButtonCallback */
}

/* screenTimeoutCallback function */
void screenTimeoutCallback(void const * argument)
{
  /* USER CODE BEGIN screenTimeoutCallback */
  ssd1306_SetDisplayOn(0);
  /* USER CODE END screenTimeoutCallback */
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
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
