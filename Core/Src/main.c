/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Author : Alexandr Olejnik <urgorka@gmail.com>
  * Hydrocontrol Ver.1.0
  * Simple controller hydroponic system (light and water) by timer.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "rtc.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "LCD.h"
#include "string.h"
#include "stdio.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
enum menuItemType {MIT_INTERVAL, MIT_TIME, MIT_TRIPLE};
enum menuMode {MENU_MODE_LISTING, MENU_MODE_TUNING};
enum menuIndex {MENU_IDX_WATER_DAY, MENU_IDX_LIGHT_DAY, MENU_IDX_TIME, MENU_IDX_WATER_MODE, MENU_IDX_LIGHT_MODE};
enum type {TYPE_HOUR, TYPE_MINUTE, TYPE_BOOL, TYPE_ONEOFMANY};
struct menuItemStruct {
	char firstLine[17];
	char secondLine[17];
	char template[20];
	enum menuItemType type;
	uint8_t settingsMaxIdx;
	uint8_t values[4];
	uint8_t valuePosition[4];
	uint8_t valueType[4];
} menuItems[5];
struct MenuStruct{
	uint8_t currentIdx;
    uint8_t maxIdx;
    uint8_t settingIdx;
    enum menuMode menuMode;
} menu;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint32_t debounceA = 0;
uint32_t bedounceInterval = 300;
uint32_t encoderLastActivityTick = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void displayOn(void) {
	HAL_GPIO_WritePin(SCREEN_BACKLIGHT_GPIO_Port, SCREEN_BACKLIGHT_Pin, 1);
}
void displayOff(void) {
	HAL_GPIO_WritePin(SCREEN_BACKLIGHT_GPIO_Port, SCREEN_BACKLIGHT_Pin, 0);
}
_Bool isDisplayOn(void) {
	return HAL_GPIO_ReadPin(SCREEN_BACKLIGHT_GPIO_Port, SCREEN_BACKLIGHT_Pin) == 1;
}
HAL_StatusTypeDef RTC_Set(uint8_t hour, uint8_t min) {
    HAL_StatusTypeDef res;
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;

//    memset(&time, 0, sizeof(time));
//    memset(&date, 0, sizeof(date));

    date.WeekDay = 7;
    date.Year = 20;
    date.Month = 11;
    date.Date = 15;

    res = HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN);
    if(res != HAL_OK) {
        return res;
    }

    time.Hours = hour;
    time.Minutes = min;
    time.Seconds = 0;

    return HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN);
}
// Invoke before reformat second line and menu redraw to set their some proprietary variables.
void beforeMenuRedraw(void) {
	RTC_TimeTypeDef time;
	HAL_StatusTypeDef res;
	if (MIT_TIME == menuItems[menu.currentIdx].type) {
		res = HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
		if (res == HAL_OK) {
			menuItems[menu.currentIdx].values[0] = time.Hours;
			menuItems[menu.currentIdx].values[1] = time.Minutes;
		} else {
			menuItems[menu.currentIdx].values[0] = 0;
			menuItems[menu.currentIdx].values[1] = 0;
		}
	}
}
// Make some code when switch from Tuning state back to Listing.
void applyTunedState(void) {
	switch (menuItems[menu.currentIdx].type) {
	case MIT_TIME:
		RTC_Set(menuItems[menu.currentIdx].values[0], menuItems[menu.currentIdx].values[1]);
		break;
	case MIT_TRIPLE:
		if (menu.currentIdx == MENU_IDX_LIGHT_MODE) {
			if (1 == menuItems[menu.currentIdx].values[0]) {
				HAL_GPIO_WritePin(RELAY_LIGHT_GPIO_Port, RELAY_LIGHT_Pin, 1);
				HAL_TIM_Base_Stop_IT(&htim4);
			} else if (1 == menuItems[menu.currentIdx].values[1]) {
				HAL_GPIO_WritePin(RELAY_LIGHT_GPIO_Port, RELAY_LIGHT_Pin, 0);
				HAL_TIM_Base_Stop_IT(&htim4);
			} else if (1 == menuItems[menu.currentIdx].values[2]) {
				HAL_TIM_Base_Start_IT(&htim4);
				HAL_TIM_PeriodElapsedCallback(&htim4);
			}
		} else if (menu.currentIdx == MENU_IDX_WATER_MODE) {
			if (1 == menuItems[menu.currentIdx].values[0]) {
				HAL_GPIO_WritePin(RELAY_WATER_GPIO_Port, RELAY_WATER_Pin, 1);
				HAL_TIM_Base_Stop_IT(&htim4);
			} else if (1 == menuItems[menu.currentIdx].values[1]) {
				HAL_GPIO_WritePin(RELAY_WATER_GPIO_Port, RELAY_WATER_Pin, 0);
				HAL_TIM_Base_Stop_IT(&htim4);
			} else if (1 == menuItems[menu.currentIdx].values[2]) {
				HAL_TIM_Base_Start_IT(&htim4);
				HAL_TIM_PeriodElapsedCallback(&htim4);
			}
		}
		break;
	case MIT_INTERVAL:
		break;
	}
}
// Redraw both lines of menu
void menuRedraw(void) {
	LCD_Clear();
    LCD_CursorMove(1, 1); LCD_SendString(menuItems[menu.currentIdx].firstLine);
    LCD_CursorMove(2, 1); LCD_SendString(menuItems[menu.currentIdx].secondLine);
}
// Shows system time when screen backlight is off.
void showStandbyScreen(void) {
	HAL_StatusTypeDef res;
	RTC_TimeTypeDef time;
	char str[25];
	res = HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
	if (res != HAL_OK) {
		Error_Handler();
	}
	LCD_Clear();
	LCD_SendString("Hydrocontrol");
	LCD_CursorMove(2, 1);
	sprintf(str, "%02d:%02d       v1.0", time.Hours, time.Minutes);
	LCD_SendString(str);
}
// Format second line data according to second line format
void menuSecondLineReformat(void) {
	char text [16];

	switch (menuItems[menu.currentIdx].type) {
	case MIT_INTERVAL:
		sprintf(text, menuItems[menu.currentIdx].template, menuItems[menu.currentIdx].values[0], menuItems[menu.currentIdx].values[1], menuItems[menu.currentIdx].values[2], menuItems[menu.currentIdx].values[3]);
		strcpy(menuItems[menu.currentIdx].secondLine, text);
		break;
	case MIT_TIME:
		sprintf(text, menuItems[menu.currentIdx].template, menuItems[menu.currentIdx].values[0], menuItems[menu.currentIdx].values[1]);
				strcpy(menuItems[menu.currentIdx].secondLine, text);
		break;
	case MIT_TRIPLE:
		sprintf(text, menuItems[menu.currentIdx].template,
				menuItems[menu.currentIdx].values[0] == 1 ? 0b01111110 : 32,
				menuItems[menu.currentIdx].values[1] == 1 ? 0b01111110 : 32,
				menuItems[menu.currentIdx].values[2] == 1 ? 0b01111110 : 32);
		strcpy(menuItems[menu.currentIdx].secondLine, text);
		break;
	}
}
// move cursor to current tuning position. Points to which parameter should be changed.
void tuningClickCursorMove(void) {
	LCD_CursorMove(2, menuItems[menu.currentIdx].valuePosition[menu.settingIdx]);
}
// Interruption callback for encoder button click.
// Changes state of menu and switch between parameters.
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	uint32_t bedounceB = HAL_GetTick();
	if (bedounceB - debounceA < bedounceInterval) return;
	debounceA = bedounceB;
	if (!isDisplayOn()) {
		displayOn();
		return;
	}
	switch (menu.menuMode) {
	case MENU_MODE_LISTING:
		menu.menuMode = MENU_MODE_TUNING;
		menu.settingIdx = 0;
		LCD_CursorState(LCD_CursorState_On);
		menuRedraw();
		tuningClickCursorMove();
		break;
	case MENU_MODE_TUNING:
		if (menu.settingIdx < menuItems[menu.currentIdx].settingsMaxIdx) {
			menu.settingIdx++;
			tuningClickCursorMove();
		} else {
			menu.menuMode = MENU_MODE_LISTING;
			applyTunedState();
			LCD_CursorState(LCD_CursorState_Off);
		}
		break;
	}
}
/**
  * Interruption handler of every-minute-timer.
  * @brief  Period elapsed callback in non-blocking mode
  * @param  htim TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	 if (TIM4 == htim->Instance) {
		RTC_TimeTypeDef time;
		HAL_StatusTypeDef res;
		uint32_t currentTimeLinear;
		uint32_t tStart, tStop;
		res = HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
		if (res == HAL_OK) {
			currentTimeLinear = time.Hours * 60 + time.Minutes;
			if (menuItems[MENU_IDX_WATER_MODE].values[2] == 1) {
				tStart = menuItems[MENU_IDX_WATER_DAY].values[0] * 60 + menuItems[MENU_IDX_WATER_DAY].values[1];
				tStop = menuItems[MENU_IDX_WATER_DAY].values[2] * 60 + menuItems[MENU_IDX_WATER_DAY].values[3];
				if (currentTimeLinear >= tStart && currentTimeLinear < tStop) {
					HAL_GPIO_WritePin(RELAY_WATER_GPIO_Port, RELAY_WATER_Pin, 1);
				} else {
					HAL_GPIO_WritePin(RELAY_WATER_GPIO_Port, RELAY_WATER_Pin, 0);
				}
			}
			if (menuItems[MENU_IDX_LIGHT_MODE].values[2] == 1) {
				tStart = menuItems[MENU_IDX_LIGHT_DAY].values[0] * 60 + menuItems[MENU_IDX_LIGHT_DAY].values[1];
				tStop = menuItems[MENU_IDX_LIGHT_DAY].values[2] * 60 + menuItems[MENU_IDX_LIGHT_DAY].values[3];
				if (currentTimeLinear >= tStart && currentTimeLinear < tStop) {
					HAL_GPIO_WritePin(RELAY_LIGHT_GPIO_Port, RELAY_LIGHT_Pin, 1);
				} else {
					HAL_GPIO_WritePin(RELAY_LIGHT_GPIO_Port, RELAY_LIGHT_Pin, 0);
				}
			}
		} else {
			Error_Handler();
		}
		if (HAL_GetTick() - encoderLastActivityTick > 5000) {
			displayOff();
			showStandbyScreen();
		}
		encoderLastActivityTick = HAL_GetTick();
	}
}
// Handles signals from encoder according menu state (mode) and selected parameter.
void encoderSignal(int8_t signal) {
	encoderLastActivityTick = HAL_GetTick();
	if (!isDisplayOn()) {
		displayOn();
		return;
	}
	switch (menu.menuMode) {
	case MENU_MODE_LISTING:
		if (1 == signal) {
			if (menu.currentIdx < menu.maxIdx) {
				menu.currentIdx++;
				beforeMenuRedraw();
				menuSecondLineReformat();
				menuRedraw();
			}
		} else {
			if (menu.currentIdx > 0) {
				menu.currentIdx--;
				beforeMenuRedraw();
				menuSecondLineReformat();
				menuRedraw();
			}
		}
		break;
	case MENU_MODE_TUNING: // increment/decrement value
		switch (menuItems[menu.currentIdx].valueType[menu.settingIdx]) {
		case TYPE_HOUR:
			if (1 == signal && menuItems[menu.currentIdx].values[menu.settingIdx] < 23)
				menuItems[menu.currentIdx].values[menu.settingIdx]++;
			else if (-1 == signal && menuItems[menu.currentIdx].values[menu.settingIdx] > 0)
				menuItems[menu.currentIdx].values[menu.settingIdx]--;
			break;
		case TYPE_MINUTE:
			if (1 == signal && menuItems[menu.currentIdx].values[menu.settingIdx] < 59)
				menuItems[menu.currentIdx].values[menu.settingIdx]++;
			else if (-1 == signal && menuItems[menu.currentIdx].values[menu.settingIdx] > 0)
				menuItems[menu.currentIdx].values[menu.settingIdx]--;
			break;
//		case TYPE_BOOL:
//			menuItems[menu.currentIdx].values[menu.settingIdx] ^= 1;
//			break;
		case TYPE_ONEOFMANY:
			for (uint8_t i = 0; i < sizeof(menuItems[menu.currentIdx].values); i++)
				menuItems[menu.currentIdx].values[i] = 0;
			menuItems[menu.currentIdx].values[menu.settingIdx] = 1;
			break;
		}
		menuSecondLineReformat();
		LCD_CursorMove(2, 1); LCD_SendString(menuItems[menu.currentIdx].secondLine);
		tuningClickCursorMove();
		break;
	}
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
  MX_RTC_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
	////////////////////////////////////////////////////////////////////////////
	// Menu definition with all them types
	strcpy(menuItems[0].firstLine, "Water day    [1]");
	strcpy(menuItems[0].secondLine, "");
	strcpy(menuItems[0].template, "%02d:%02d-%02d:%02d");
	menuItems[0].type = MIT_INTERVAL;
	menuItems[0].settingsMaxIdx = 3;
	menuItems[0].valuePosition[0] = 1;
	menuItems[0].valuePosition[1] = 4;
	menuItems[0].valuePosition[2] = 7;
	menuItems[0].valuePosition[3] = 10;
	menuItems[0].values[0] = 12;
	menuItems[0].values[1] = 31;
	menuItems[0].values[2] = 12;
	menuItems[0].values[3] = 33;
	menuItems[0].valueType[0] = TYPE_HOUR;
	menuItems[0].valueType[1] = TYPE_MINUTE;
	menuItems[0].valueType[2] = TYPE_HOUR;
	menuItems[0].valueType[3] = TYPE_MINUTE;

	strcpy(menuItems[1].firstLine, "Light day    [2]");
	strcpy(menuItems[1].secondLine, "");
	strcpy(menuItems[1].template, "%02d:%02d-%02d:%02d");
	menuItems[1].type = MIT_INTERVAL;
	menuItems[1].settingsMaxIdx = 3;
	menuItems[1].valuePosition[0] = 1;
	menuItems[1].valuePosition[1] = 4;
	menuItems[1].valuePosition[2] = 7;
	menuItems[1].valuePosition[3] = 10;
	menuItems[1].values[0] = 12;
	menuItems[1].values[1] = 32;
	menuItems[1].values[2] = 12;
	menuItems[1].values[3] = 34;
	menuItems[1].valueType[0] = TYPE_HOUR;
	menuItems[1].valueType[1] = TYPE_MINUTE;
	menuItems[1].valueType[2] = TYPE_HOUR;
	menuItems[1].valueType[3] = TYPE_MINUTE;

	strcpy(menuItems[2].firstLine, "Current time [3]");
	strcpy(menuItems[2].secondLine, "");
	strcpy(menuItems[2].template, "%02d:%02d");
	menuItems[2].type = MIT_TIME;
	menuItems[2].settingsMaxIdx = 1;
	menuItems[2].valuePosition[0] = 1;
	menuItems[2].valuePosition[1] = 4;
	menuItems[2].values[0] = 12;
	menuItems[2].values[1] = 30;
	menuItems[2].valueType[0] = TYPE_HOUR;
	menuItems[2].valueType[1] = TYPE_MINUTE;

	strcpy(menuItems[MENU_IDX_WATER_MODE].firstLine, "Water mode   [4]");
	strcpy(menuItems[MENU_IDX_WATER_MODE].secondLine, "");
	strcpy(menuItems[MENU_IDX_WATER_MODE].template, "%con %coff %ctimer");
	menuItems[MENU_IDX_WATER_MODE].type = MIT_TRIPLE;
	menuItems[MENU_IDX_WATER_MODE].settingsMaxIdx = 2;
	menuItems[MENU_IDX_WATER_MODE].valuePosition[0] = 1;
	menuItems[MENU_IDX_WATER_MODE].valuePosition[1] = 5;
	menuItems[MENU_IDX_WATER_MODE].valuePosition[2] = 10;
	menuItems[MENU_IDX_WATER_MODE].values[0] = 0;
	menuItems[MENU_IDX_WATER_MODE].values[1] = 0;
	menuItems[MENU_IDX_WATER_MODE].values[2] = 1;
	menuItems[MENU_IDX_WATER_MODE].valueType[0] = TYPE_ONEOFMANY;
	menuItems[MENU_IDX_WATER_MODE].valueType[1] = TYPE_ONEOFMANY;
	menuItems[MENU_IDX_WATER_MODE].valueType[2] = TYPE_ONEOFMANY;


	strcpy(menuItems[MENU_IDX_LIGHT_MODE].firstLine, "Light mode   [5]");
	strcpy(menuItems[MENU_IDX_LIGHT_MODE].secondLine, "");
	strcpy(menuItems[MENU_IDX_LIGHT_MODE].template, "%con %coff %ctimer");
	menuItems[MENU_IDX_LIGHT_MODE].type = MIT_TRIPLE;
	menuItems[MENU_IDX_LIGHT_MODE].settingsMaxIdx = 2;
	menuItems[MENU_IDX_LIGHT_MODE].valuePosition[0] = 1;
	menuItems[MENU_IDX_LIGHT_MODE].valuePosition[1] = 5;
	menuItems[MENU_IDX_LIGHT_MODE].valuePosition[2] = 10;
	menuItems[MENU_IDX_LIGHT_MODE].values[0] = 0;
	menuItems[MENU_IDX_LIGHT_MODE].values[1] = 0;
	menuItems[MENU_IDX_LIGHT_MODE].values[2] = 1;
	menuItems[MENU_IDX_LIGHT_MODE].valueType[0] = TYPE_ONEOFMANY;
	menuItems[MENU_IDX_LIGHT_MODE].valueType[1] = TYPE_ONEOFMANY;
	menuItems[MENU_IDX_LIGHT_MODE].valueType[2] = TYPE_ONEOFMANY;

	menu.currentIdx = 0;
	menu.maxIdx = 4;

	HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
	LCD_Init();
	RTC_Set(12, 30);

    __HAL_TIM_SET_COUNTER(&htim3, 1);// prevent fake increment at start
    menuSecondLineReformat();
    menuRedraw();
    if (1 == menuItems[MENU_IDX_WATER_MODE].values[2] || 1 == menuItems[MENU_IDX_LIGHT_MODE].values[2]) {
    	HAL_TIM_Base_Start_IT(&htim4);
    	HAL_TIM_PeriodElapsedCallback(&htim4);
   	}
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    uint16_t encValue = 1;
	uint16_t encValuePrev = 1;
	uint8_t  encDirection;
	displayOn();
	while (1) {
		encValue = __HAL_TIM_GET_COUNTER(&htim3);
		encDirection = READ_BIT(htim3.Instance->CR1, TIM_CR1_DIR);
		if (encValuePrev != encValue && (encValue % 2 == 0)){
			encoderSignal(0 == encDirection ? 1 : -1);
		}
		encValuePrev = encValue;
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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
