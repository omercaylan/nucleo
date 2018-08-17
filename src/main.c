#include "FreeRTOS.h"
#include "logger.h"
#include "main.h"
#include "stm32f4xx_hal_rtc.h"
#include "task.h"

#define ledSTACK_SIZE	configMINIMAL_STACK_SIZE
#define ledPRIORITY		( tskIDLE_PRIORITY + 1UL )

#define RTC_ASYNCH_PREDIV  0x7F   /* LSE as RTC clock */
#define RTC_SYNCH_PREDIV 0x00FF /* LSE as RTC clock */

static void Error_Handler(void);
static void SystemClock_Config(void);
static void vTriggerMeasurementTask(void *pvParameters);
static void vMeasureTask(void *pvParameters);
static void RTC_Config(void);

void configureGpioLed();
void configureGpioDistance();

int main(void)
{
	loggerInit();

	/* STM32F4xx HAL library initialization:
	   - Configure the Flash prefetch, Flash preread and Buffer caches
	   - Systick timer is configured by default as source of time base, but user
	   can eventually implement his proper time base source (a general purpose
	   timer for example or other time source), keeping in mind that Time base
	   duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
	   handled in milliseconds basis.
	   - Low Level Initialization
	 */
	HAL_Init();

	/* Configure the System clock to 84 MHz */
	SystemClock_Config();
	loggerPrintf("test logger");
	RTC_Config(); 
	__GPIOA_CLK_ENABLE();

	configureGpioLed();
	configureGpioDistance();

	xTaskCreate(vTriggerMeasurementTask, "MEAS1", ledSTACK_SIZE,
		    (void *)NULL, ledPRIORITY, (TaskHandle_t *) NULL);
	xTaskCreate(vMeasureTask, "MEAS2", ledSTACK_SIZE, (void *)NULL,
		    ledPRIORITY, (TaskHandle_t *) NULL);

	vTaskStartScheduler();

	while (1) {
		HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
		HAL_Delay(100);
	}
}

void configureGpioLed()
{
	/* Configure PA05 IO in output push-pull mode to drive external LED */
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.Pin = GPIO_PIN_5;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

#define GPIO_PIN_DIST_TRIGGER	GPIO_PIN_6
#define GPIO_PIN_DIST_ECHO 	GPIO_PIN_7
void configureGpioDistance()
{
	{
		GPIO_InitTypeDef GPIO_InitStruct;
		GPIO_InitStruct.Pin = GPIO_PIN_DIST_TRIGGER;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull = GPIO_PULLUP;
		GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	}
	{
		GPIO_InitTypeDef GPIO_InitStruct;
		GPIO_InitStruct.Pin = GPIO_PIN_DIST_ECHO;
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
		GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	}
}

static void vTriggerMeasurementTask(void *pvParameters)
{
	for (;;) {
		HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_DIST_TRIGGER, GPIO_PIN_RESET);
		vTaskDelay(1);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_DIST_TRIGGER, GPIO_PIN_SET);
		vTaskDelay(1);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_DIST_TRIGGER, GPIO_PIN_RESET);
//		taskENTER_CRITICAL();
//		loggerPrintf("%d: trig meas", xTaskGetTickCount());
//		taskEXIT_CRITICAL();
		vTaskDelay(500);
//              vTaskDelay(100);
//              HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
//              vTaskDelay(400);
//              HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
	}
}

static void vMeasureTask(void *pvParameters)
{
	for (;;) {
		TickType_t tickAtRaising, tickAtFalling;
		GPIO_PinState s1 = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_DIST_ECHO);
		vTaskDelay(1);
		GPIO_PinState s2 = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_DIST_ECHO);
		if (s1 == GPIO_PIN_RESET && s2 == GPIO_PIN_SET) {
			tickAtRaising = xTaskGetTickCount();
			tickAtFalling = 0;
//                      taskENTER_CRITICAL();
//                      loggerPrintf("%d: slope raising", xTaskGetTickCount());
//                      taskEXIT_CRITICAL();
		} else if (s1 == GPIO_PIN_SET && s2 == GPIO_PIN_RESET) {
			tickAtFalling = xTaskGetTickCount();
			taskENTER_CRITICAL();
			loggerPrintf("%d: slope falling, (%d - %d) * C = %dm",
				     xTaskGetTickCount(), tickAtFalling,
				     tickAtRaising,
				     343 * (tickAtFalling -
					    tickAtRaising) / 1000);
			taskEXIT_CRITICAL();
			tickAtRaising = 0;
			tickAtFalling = 0;
		}
	}
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (HSI)
  *            SYSCLK(Hz)                     = 84000000
  *            HCLK(Hz)                       = 84000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 2
  *            APB2 Prescaler                 = 1
  *            HSI Frequency(Hz)              = 16000000
  *            PLL_M                          = 16
  *            PLL_N                          = 336
  *            PLL_P                          = 4
  *            PLL_Q                          = 7
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale2 mode
  *            Flash Latency(WS)              = 2
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_OscInitTypeDef RCC_OscInitStruct;
	HAL_StatusTypeDef status;

	/* Enable Power Control clock */
	__PWR_CLK_ENABLE();

	/* The voltage scaling allows optimizing the power consumption when the device is
	   clocked below the maximum system frequency, to update the voltage scaling value
	   regarding system frequency refer to product datasheet.  */
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

	/* Enable HSI Oscillator and activate PLL with HSI as source */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.LSIState = RCC_LSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = 0x10;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 16;
	RCC_OscInitStruct.PLL.PLLN = 336;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
	RCC_OscInitStruct.PLL.PLLQ = 7;
	loggerPrintf("SystemClock_Config 0");
	if ((status = HAL_RCC_OscConfig(&RCC_OscInitStruct)) != HAL_OK) {
		loggerPrintf("SystemClock_Config error");
		Error_Handler();
	}
	loggerPrintf("SystemClock_Config 0 pass");

	/* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
	   clocks dividers */
	RCC_ClkInitStruct.ClockType =
	    (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 |
	     RCC_CLOCKTYPE_PCLK2);
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
		Error_Handler();
	}
	loggerPrintf("SystemClock_Config bye");
}

static void RTC_Config(void)
{
	loggerPrintf("rtc config 0");
	RTC_HandleTypeDef RtcHandle;
	RtcHandle.Instance = RTC; 
	RtcHandle.Init.HourFormat = RTC_HOURFORMAT_24;
	RtcHandle.Init.AsynchPrediv = RTC_ASYNCH_PREDIV;
	RtcHandle.Init.SynchPrediv = RTC_SYNCH_PREDIV;
	RtcHandle.Init.OutPut = RTC_OUTPUT_DISABLE;
	RtcHandle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
	RtcHandle.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

	loggerPrintf("rtc config 1");
  	HAL_StatusTypeDef ret = HAL_RTC_Init(&RtcHandle);
	loggerPrintf("rtc config 2");
	switch (ret)
	{
		case HAL_OK:
			loggerPrintf("HAL_OK");
			break;
		case HAL_ERROR:
			loggerPrintf("HAL_ERROR");
			break;
		case HAL_BUSY:
			loggerPrintf("HAL_BUSY");
			break;
		case HAL_TIMEOUT:
			loggerPrintf("HAL_TIMEOUT");
			break;
		default:
			loggerPrintf("HAL_???");
			break;
	}

  	if(ret != HAL_OK)
	{
		Error_Handler(); 
	}

//
//	// 0000000000000000000000000000000000]]]
//    RTC_DateTypeDef sdatestructureget;
//    RTC_TimeTypeDef stimestructureget;
//    
//    /* Get the RTC current Time */
//    HAL_RTC_GetTime(&RtcHandle, &stimestructureget, FORMAT_BIN);
//    /* Get the RTC current Date */
//    HAL_RTC_GetDate(&RtcHandle, &sdatestructureget, FORMAT_BIN);

}

static void Error_Handler(void)
{
	while (1) {
	}
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t * file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
	   ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while (1) {
	}
}
#endif
