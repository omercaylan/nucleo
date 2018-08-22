#include "FreeRTOS.h"
#include "logger.h"
#include "main.h"
#include "task.h"

#define ledSTACK_SIZE	configMINIMAL_STACK_SIZE
#define ledPRIORITY		( tskIDLE_PRIORITY + 1UL )

static void Error_Handler(void);
static void SystemClock_Config(void);
static void Timer_Config(void);
static void vLEDFlashTask(void *pvParameters);

static TIM_HandleTypeDef TIM_HandleStruct;

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
	Timer_Config();

	GPIO_InitTypeDef GPIO_InitStruct;

	__GPIOA_CLK_ENABLE();

	/* Configure PA05 IO in output push-pull mode to drive external LED */
	GPIO_InitStruct.Pin = GPIO_PIN_5;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FAST;

	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	xTaskCreate(vLEDFlashTask, "LED1", ledSTACK_SIZE, (void *)NULL,
		    ledPRIORITY, (TaskHandle_t *) NULL);

	loggerPrintf("cnt a %d", __HAL_TIM_GetCounter(&TIM_HandleStruct));

	vTaskStartScheduler();
	//Should never pass this point.

	while (1) {
		HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
		HAL_Delay(100);
	}
}

static void vLEDFlashTask(void *pvParameters)
{
	int i = 0;
	for (;;) {
		i++;
		taskENTER_CRITICAL();
		loggerPrintf("cnt x %d", __HAL_TIM_GetCounter(&TIM_HandleStruct));
		loggerPrintf("L%d: a!", i);
		taskEXIT_CRITICAL();
		vTaskDelay(100);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
		vTaskDelay(400);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
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
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = 0x10;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 16;
	RCC_OscInitStruct.PLL.PLLN = 336;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
	RCC_OscInitStruct.PLL.PLLQ = 7;
	if ((status = HAL_RCC_OscConfig(&RCC_OscInitStruct)) != HAL_OK) {
		Error_Handler();
	}

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

}

static void Timer_Config(void)
{
	__TIM5_CLK_ENABLE();
	TIM_HandleStruct.Instance = TIM5;
	TIM_HandleStruct.Init.Prescaler = 4;
	TIM_HandleStruct.Init.CounterMode = TIM_COUNTERMODE_UP;
	TIM_HandleStruct.Init.Period = 1000;
	TIM_HandleStruct.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	HAL_TIM_Base_Init(&TIM_HandleStruct);
	HAL_TIM_Base_Start(&TIM_HandleStruct);
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
