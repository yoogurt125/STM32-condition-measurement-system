/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "tm_stm32_ds18b20.h"
#include "tm_stm32_onewire.h"
#include "tm_stm32_delay.h"
#include "tm_stm32_rcc.h"
#include "defines.h"
#include "string.h"
#include <stdbool.h>
#include <stdio.h>


/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
TIM_HandleTypeDef htim7;

UART_HandleTypeDef huart2;
UART_HandleTypeDef huart1;


osThreadId defaultTaskHandle;
osThreadId ds18b20_taskHandle;
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM7_Init(void);
static void MX_USART1_UART_Init(void);
void StartDefaultTask(void const * argument);
void Start_ds18b20_task(void const * argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
char buffer[1];
float temp;
uint8_t numbers[10] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F,
		0x67 };
uint8_t temp_int_units;
uint8_t temp_int_decimals;
uint8_t humidity_int_units;
uint8_t humidity_int_decimals;
uint8_t humidityTab[30];
uint8_t temperatureTab[30];
int i = 0;
bool display_temp = true;
TM_OneWire_t OW;
uint8_t DS_ROM[8];

/*DHT11 CODE*/
uint8_t Rh_byte1, Rh_byte2, Temp_byte1, Temp_byte2;
uint16_t sum, RH, TEMP;
uint8_t presence = 0;
GPIO_InitTypeDef GPIO_InitStruct;


void set_gpio_output(void) {
	/*Configure GPIO pin output: PA2 */
	GPIO_InitStruct.Pin = DHT11_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(DHT11_GPIO_Port, &GPIO_InitStruct);
}

void set_gpio_input(void) {
	/*Configure GPIO pin input: PA2 */
	GPIO_InitStruct.Pin = DHT11_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(DHT11_GPIO_Port, &GPIO_InitStruct);
}

void DHT11_start(void) {
	set_gpio_output();  // set the pin as output
	HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin, 0);   // pull the pin low
	Delay(18000);   // wait for 18ms
	HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin, 1);   // pull the pin low
	Delay(20);
	set_gpio_input();   // set as input
}

uint8_t check_response(void) {
	uint8_t response = 0;
	Delay(40);
	if (!(HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin))) {
		Delay(80);
		if ((HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin)))
			response = 1;
		else
			response = -1;
	}
	while ((HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin)))
		;   // wait for the pin to go low

	return response;
}

uint8_t read_data(void) {
	uint8_t i, j;
	for (j = 0; j < 8; j++) {
		while (!(HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin)))
			;   // wait for the pin to go high
		Delay(40);   // wait for 40 us
		if ((HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin)) == 0) // if the pin is low
				{
			i &= ~(1 << (7 - j));   // write 0
		} else
			i |= (1 << (7 - j));  // if the pin is high, write 1
		while ((HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin)))
			;
		// wait for the pin to go low
	}
	return i;
}



/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	TM_RCC_InitSystem();

	GPIOC->ODR = 0xff;
	GPIOB->ODR = 0xff;

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();


  /* USER CODE BEGIN Init */
	TM_OneWire_Init(&OW, GPIOC, GPIO_PIN_9);


	if (TM_OneWire_First(&OW)) {
		/* Read ROM number */
		TM_OneWire_GetFullROM(&OW, DS_ROM);
	}
	if (TM_DS18B20_Is(DS_ROM)) {
		/* Set resolution */
		TM_DS18B20_SetResolution(&OW, DS_ROM, TM_DS18B20_Resolution_9bits);

		/* Set high and low alarms */
		TM_DS18B20_SetAlarmHighTemperature(&OW, DS_ROM, 30);
		TM_DS18B20_SetAlarmLowTemperature(&OW, DS_ROM, 10);

		/* Start conversion on all sensors */
		TM_DS18B20_Start(&OW, DS_ROM);
	}

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();

	MX_TIM7_Init();
	MX_USART1_UART_Init();
//	MX_USART2_UART_Init();


	/* USER CODE BEGIN 2 */
	HAL_TIM_Base_Start_IT(&htim7);
  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
	/* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
	/* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
	/* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
	/* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityIdle, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of ds18b20_task */
//  osThreadDef(ds18b20_task, Start_ds18b20_task, osPriorityNormal, 0, 128);
//  ds18b20_taskHandle = osThreadCreate(osThread(ds18b20_task), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
	/* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 180;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM7 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM7_Init(void)
{

  /* USER CODE BEGIN TIM7_Init 0 */

  /* USER CODE END TIM7_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM7_Init 1 */

	/* USER CODE END TIM7_Init 1 */
	htim7.Instance = TIM7;
	htim7.Init.Prescaler = 179;
	htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim7.Init.Period = 89;
	htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim7) != HAL_OK) {
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim7, &sMasterConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN TIM7_Init 2 */

  /* USER CODE END TIM7_Init 2 */

}


/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, A_Pin|B_Pin|C_Pin|D_Pin
                          |E_Pin|F_Pin|G_Pin|H_Pin
                          |DS18B20_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, A1_Pin|B1_Pin|C1_Pin|D1_Pin
                          |E1_Pin|F1_Pin|G1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : BlueB_Pin */
  GPIO_InitStruct.Pin = BlueB_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BlueB_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : A_Pin B_Pin C_Pin D_Pin
                           E_Pin F_Pin G_Pin H_Pin
                           DS18B20_Pin */
  GPIO_InitStruct.Pin = A_Pin|B_Pin|C_Pin|D_Pin
                          |E_Pin|F_Pin|G_Pin|H_Pin
                          |DS18B20_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : A1_Pin B1_Pin C1_Pin D1_Pin
                           E1_Pin F1_Pin G1_Pin */
  GPIO_InitStruct.Pin = A1_Pin|B1_Pin|C1_Pin|D1_Pin
                          |E1_Pin|F1_Pin|G1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
void debugPrintln(UART_HandleTypeDef *huart, char _out[]){
	HAL_UART_Transmit(huart, (uint8_t *) _out, strlen(_out), 1000);
	char newline[2] = "\r\n";
	HAL_UART_Transmit(huart, (uint8_t *) newline, 2, 1000);
}

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{

  /* USER CODE BEGIN 5 */
	/* Infinite loop */
	for (;;) {
		for(i=0;i<30;i++){
		DHT11_start();
		presence = check_response();
		Rh_byte1 = read_data();
		Rh_byte2 = read_data();
		Temp_byte1 = read_data();
		Temp_byte2 = read_data();
		sum = read_data();

		humidity_int_units = (Rh_byte1) % 10;
		humidity_int_decimals = (Rh_byte1) / 10;

		humidityTab[i] = (uint8_t) Rh_byte1;


//		GPIOB->ODR = numbers[humidity_int_decimals];
//		GPIOC->ODR = numbers[humidity_int_units];

//		if (HAL_GPIO_ReadPin(BlueB_GPIO_Port, BlueB_Pin)
//						== GPIO_PIN_RESET) {
//

		vTaskDelay(3000);

		if (HAL_GPIO_ReadPin(BlueB_GPIO_Port, BlueB_Pin)) {
			if (TM_DS18B20_Is(DS_ROM)) {
				/* Everything is done */
				if (TM_DS18B20_AllDone(&OW)) {
					/* Read temperature from device */
					if (TM_DS18B20_Read(&OW, DS_ROM, &temp)) {
						/* Temp read OK, CRC is OK */

						/* Start again on all sensors */
						TM_DS18B20_StartAll(&OW);

						temp_int_units = ((uint8_t) temp) % 10;
						temp_int_decimals = ((uint8_t) temp) / 10;

						temperatureTab[i] = (uint8_t) temp;

//							GPIOB->ODR = numbers[temp_int_decimals];
//							GPIOC->ODR = numbers[temp_int_units];

						/* Check temperature */

					} else {
						/* CRC failed, hardware problems on data line */
					}
				}
			}
		} else if (HAL_GPIO_ReadPin(BlueB_GPIO_Port, BlueB_Pin)
				== GPIO_PIN_RESET) {

			display_temp = !display_temp;
		}

		if (display_temp) {
			GPIOB->ODR = numbers[temp_int_decimals];
			GPIOC->ODR = numbers[temp_int_units];
		} else {
			GPIOB->ODR = numbers[humidity_int_decimals];
			GPIOC->ODR = numbers[humidity_int_units];
		}
		LD2_TOGGLE();
//		debugPrintln(&huart1, (char*) temp_int_decimals
		HAL_UART_Receive(&huart1, (uint8_t*) &buffer, 1, 1000);
		if(*buffer=='1'){
			temperatureTab[29]='\n';
			HAL_UART_Transmit(&huart1, temperatureTab, 30, 1000);
			*buffer='0';
		}
		if(i==30)
			i=0;
		}
	}

}
  /* USER CODE END 5 */


/* USER CODE BEGIN Header_Start_ds18b20_task */
/**
 * @brief Function implementing the ds18b20_task thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_Start_ds18b20_task */
//void Start_ds18b20_task(void const * argument)
//{
//  /* USER CODE BEGIN Start_ds18b20_task */
//	/* Infinite loop */
//	for (;;) {
////		vTaskDelay(500);
////		if (HAL_GPIO_ReadPin(BlueB_GPIO_Port, BlueB_Pin)) {
////			if (TM_DS18B20_Is(DS_ROM)) {
////				/* Everything is done */
////				if (TM_DS18B20_AllDone(&OW)) {
////					/* Read temperature from device */
////					if (TM_DS18B20_Read(&OW, DS_ROM, &temp)) {
////						/* Temp read OK, CRC is OK */
////
////						/* Start again on all sensors */
////						TM_DS18B20_StartAll(&OW);
////
////						temp_int_units = ((uint8_t) temp) % 10;
////						temp_int_decimals = ((uint8_t) temp) / 10;
////
////						GPIOB->ODR = numbers[temp_int_decimals];
////						GPIOC->ODR = numbers[temp_int_units];
////
////
////						/* Check temperature */
////
////						vTaskDelay(500);
////
////					} else {
////						/* CRC failed, hardware problems on data line */
////					}
////				}
////			}
////		} else if (HAL_GPIO_ReadPin(BlueB_GPIO_Port, BlueB_Pin)
////				== GPIO_PIN_RESET) {
////
//////			GPIOC->ODR = numbers[temp_int_decimals];
//////			GPIOB->ODR = numbers[temp_int_units];
////
////			}
//	}
//}

  /* USER CODE END Start_ds18b20_task */


/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

	/* USER CODE END Callback 0 */
	if (htim->Instance == TIM7) {
		HAL_IncTick();
	}
	/* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */

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
	 tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
