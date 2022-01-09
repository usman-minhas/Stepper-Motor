/****** 



 1. both OD mode and PP mode can drive the motor! However, some pin can not output  high in OD mode!!! 
   (maybe because those pins have other alternate functions)). 

 2. the signals do not need to be inverted before feeded in H-bridge! 
*/




#include "main.h"


#define COLUMN(x) ((x) * (((sFONT *)BSP_LCD_GetFont())->Width))    //see font.h, for defining LINE(X)

TIM_HandleTypeDef    Tim3_Handle;
TIM_OC_InitTypeDef Tim3_OCInitStructure;
uint16_t Tim3_PrescalerValue;
uint8_t clockwise = 1, fullstepping = 1, state = 1;
double speedFactor = 1.0;

void ExtBtn_Config(void);
void Output_Config(void);
void TIM3_Config(void);
void newState(void);
void completeHalfStep(void);
void displayVals(void);

void LCD_DisplayString(uint16_t LineNumber, uint16_t ColumnNumber, uint8_t *ptr);
void LCD_DisplayInt(uint16_t LineNumber, uint16_t ColumnNumber, int Number);
void LCD_DisplayFloat(uint16_t LineNumber, uint16_t ColumnNumber, float Number, int DigitAfterDecimalPoint);


static void SystemClock_Config(void);
static void Error_Handler(void);

int main(void){
	
		/* STM32F4xx HAL library initialization:
       - Configure the Flash prefetch, instruction and Data caches
       - Configure the Systick to generate an interrupt each 1 msec
       - Set NVIC Group Priority to 4
       - Global MSP (MCU Support Package) initialization
     */
		HAL_Init();
		
	
		 /* Configure the system clock to 180 MHz */
		SystemClock_Config();
		
		HAL_InitTick(0x0000); // set systick's priority to the highest.
	
		TIM3_Config();
	
		BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_EXTI); 
	
		ExtBtn_Config();
	
		Output_Config();
	
		BSP_LED_Init(LED4);

		BSP_LCD_Init();
		//BSP_LCD_LayerDefaultInit(uint16_t LayerIndex, uint32_t FB_Address);
		BSP_LCD_LayerDefaultInit(0, LCD_FRAME_BUFFER);   //LCD_FRAME_BUFFER, defined as 0xD0000000 in _discovery_lcd.h
															// the LayerIndex may be 0 and 1. if is 2, then the LCD is dark.
		//BSP_LCD_SelectLayer(uint32_t LayerIndex);
		BSP_LCD_SelectLayer(0);
		//BSP_LCD_SetLayerVisible(0, ENABLE); //do not need this line.
		BSP_LCD_Clear(LCD_COLOR_WHITE);  //need this line, otherwise, the screen is dark	
		BSP_LCD_DisplayOn();
	 
		BSP_LCD_SetFont(&Font20);  //the default font,  LCD_DEFAULT_FONT, which is defined in _lcd.h, is Font24

			
			
		LCD_DisplayString(3, 0, (uint8_t*) "Step:");
		LCD_DisplayString(6, 0, (uint8_t*) "Direction:");
		LCD_DisplayString(9, 0, (uint8_t*) "Speed:");
		displayVals();
		
		while(1) {	

			
		} // end of while loop
	
}  //end of main


/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 180000000
  *            HCLK(Hz)                       = 180000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 8000000
  *            PLL_M                          = 8
  *            PLL_N                          = 360
  *            PLL_P                          = 2
  *            PLL_Q                          = 7
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 5
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();
  
  /* The voltage scaling allows optimizing the power consumption when the device is 
     clocked below the maximum system frequency, to update the voltage scaling value 
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  
  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 360;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /* Activate the Over-Drive mode */
  HAL_PWREx_EnableOverDrive();
 
  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;  
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

void  TIM3_Config(void)
{

	//Prescaler value to tick at 1ms
  Tim3_PrescalerValue = (uint32_t) (SystemCoreClock/2)/1000 - 1;

  Tim3_Handle.Instance = TIM3; //TIM3 is defined in stm32f429xx.h

  Tim3_Handle.Init.Period = 65535;
  Tim3_Handle.Init.Prescaler = Tim3_PrescalerValue;
  Tim3_Handle.Init.ClockDivision = 0;
  Tim3_Handle.Init.CounterMode = TIM_COUNTERMODE_UP;
  /*if(HAL_TIM_Base_Init(&Tim3_Handle) != HAL_OK) 
  {
    /* Initialization Error *
    Error_Handler();
  }
  
  if(HAL_TIM_Base_Start_IT(&Tim3_Handle) != HAL_OK)
  {
    /* Starting Error *
    Error_Handler();
  }*/
	
	Tim3_OCInitStructure.OCMode =  TIM_OCMODE_TIMING;
	Tim3_OCInitStructure.Pulse = 2208; //Change to 1104 for half stepping or change ccr
	Tim3_OCInitStructure.OCPolarity=TIM_OCPOLARITY_HIGH;
	
	HAL_TIM_OC_Init(&Tim3_Handle);
	
	HAL_TIM_OC_ConfigChannel(&Tim3_Handle, &Tim3_OCInitStructure, TIM_CHANNEL_1);	
	
	HAL_TIM_OC_Start_IT(&Tim3_Handle, TIM_CHANNEL_1);
}


void ExtBtn_Config(void){
	
	
 //Enable GPIO C, D, E Clock
	GPIO_InitTypeDef   GPIOE_InitStructure;
	GPIO_InitTypeDef   GPIOC_InitStructure;
	GPIO_InitTypeDef   GPIOD_InitStructure;
	
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	
	//Configure PE3 as IT Falling
	GPIOE_InitStructure.Mode =  GPIO_MODE_IT_FALLING;
  GPIOE_InitStructure.Pull = GPIO_PULLUP;
  GPIOE_InitStructure.Pin = GPIO_PIN_3;
	HAL_GPIO_Init(GPIOE, &GPIOE_InitStructure);
	
	//Configure PC1 as IT Falling
	GPIOC_InitStructure.Mode =  GPIO_MODE_IT_FALLING;
  GPIOC_InitStructure.Pull = GPIO_PULLUP;
  GPIOC_InitStructure.Pin = GPIO_PIN_1;
	HAL_GPIO_Init(GPIOC, &GPIOC_InitStructure);
	
	//Configure PD2 as IT Falling
  GPIOD_InitStructure.Mode =  GPIO_MODE_IT_FALLING;
  GPIOD_InitStructure.Pull = GPIO_PULLUP;
  GPIOD_InitStructure.Pin = GPIO_PIN_2;
	HAL_GPIO_Init(GPIOD, &GPIOD_InitStructure);
	
	__HAL_GPIO_EXTI_CLEAR_FLAG(GPIO_PIN_3);
	__HAL_GPIO_EXTI_CLEAR_FLAG(GPIO_PIN_1);
	__HAL_GPIO_EXTI_CLEAR_FLAG(GPIO_PIN_2);
	
	//Enable interrupt
	HAL_NVIC_SetPriority(EXTI3_IRQn, 4, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);
	
	HAL_NVIC_SetPriority(EXTI1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);
	
	HAL_NVIC_SetPriority(EXTI2_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);
	
	
}

void Output_Config(void){
	
	//Configure output pins PC8, PC11, PC12, PC13
	GPIO_InitTypeDef   GPIOC_InitStructure;
	
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8|GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13, GPIO_PIN_RESET);
	
	GPIOC_InitStructure.Pin = GPIO_PIN_8|GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13;
	GPIOC_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIOC_InitStructure.Pull = GPIO_NOPULL;
	GPIOC_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;
	
	HAL_GPIO_Init(GPIOC, &GPIOC_InitStructure);
	
}


void LCD_DisplayString(uint16_t LineNumber, uint16_t ColumnNumber, uint8_t *ptr)
{  
  //here the LineNumber and the ColumnNumber are NOT  pixel numbers!!!
		while (*ptr!=NULL)
    {
				BSP_LCD_DisplayChar(COLUMN(ColumnNumber),LINE(LineNumber), *ptr); //new version of this function need Xpos first. so COLUMN() is the first para.
				ColumnNumber++;
			 //to avoid wrapping on the same line and replacing chars 
				if ((ColumnNumber+1)*(((sFONT *)BSP_LCD_GetFont())->Width)>=BSP_LCD_GetXSize() ){
					ColumnNumber=0;
					LineNumber++;
				}
					
				ptr++;
		}
}

void LCD_DisplayInt(uint16_t LineNumber, uint16_t ColumnNumber, int Number)
{  
  //here the LineNumber and the ColumnNumber are NOT  pixel numbers!!!
		char lcd_buffer[15];
		sprintf(lcd_buffer,"%d",Number);
	
		LCD_DisplayString(LineNumber, ColumnNumber, (uint8_t *) lcd_buffer);
}

void LCD_DisplayFloat(uint16_t LineNumber, uint16_t ColumnNumber, float Number, int DigitAfterDecimalPoint)
{  
  //here the LineNumber and the ColumnNumber are NOT  pixel numbers!!!
		char lcd_buffer[15];
		
		sprintf(lcd_buffer,"%.*f",DigitAfterDecimalPoint, Number);  //6 digits after decimal point, this is also the default setting for Keil uVision 4.74 environment.
	
		LCD_DisplayString(LineNumber, ColumnNumber, (uint8_t *) lcd_buffer);
}



void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	
		if(GPIO_Pin == KEY_BUTTON_PIN)  //Change stepping if User Button is pressed
		{
			if(fullstepping){
				
				fullstepping = 0;
				__HAL_TIM_SET_COMPARE(&Tim3_Handle, TIM_CHANNEL_1, 1104*speedFactor);
				__HAL_TIM_SET_COUNTER(&Tim3_Handle, 0x0000); 
				//Tim3_Handle.Instance->CCR1 = 1104 * speedFactor;
			}
			else if(!fullstepping){
				
				fullstepping = 1;
				newState();
				__HAL_TIM_SET_COMPARE(&Tim3_Handle, TIM_CHANNEL_1, 2208*speedFactor);
				__HAL_TIM_SET_COUNTER(&Tim3_Handle, 0x0000); 
				//Tim3_Handle.Instance->CCR1 = 2208 * speedFactor;
			}
			
			displayVals();
		}
		
		
		if(GPIO_Pin == GPIO_PIN_1)//Change direction if Button 1 is pressed
		{
			if(clockwise){
				clockwise = 0;
			}
			else if (!clockwise){
				clockwise = 1;
			}
			
			displayVals();
		}  //end of PIN_1

		if(GPIO_Pin == GPIO_PIN_2)//Decrease Speed if Button 2 is pressed
		{
			
			if(speedFactor < 2){//Minimum speed of 0.25x
				
				speedFactor += 0.1;//New factor for speed
				
				if(fullstepping){
				__HAL_TIM_SET_COMPARE(&Tim3_Handle, TIM_CHANNEL_1, 2208*speedFactor);
				__HAL_TIM_SET_COUNTER(&Tim3_Handle, 0x0000); 
					//Tim3_Handle.Instance->CCR1 = 2208 * speedFactor;
				}
				else if(!fullstepping){
				__HAL_TIM_SET_COMPARE(&Tim3_Handle, TIM_CHANNEL_1, 1104*speedFactor);
				__HAL_TIM_SET_COUNTER(&Tim3_Handle, 0x0000); 
					//Tim3_Handle.Instance->CCR1 = 1104 * speedFactor;
				}
			}
			
			displayVals();
		} //end of if PIN_2	
		
		
		if(GPIO_Pin == GPIO_PIN_3)//Increase Speed if Button 3 is pressed
		{
			if(speedFactor > 0.6){//Max speed of 2x
				
				speedFactor -= 0.1;//New factor for speed
				
				if(fullstepping){
				__HAL_TIM_SET_COMPARE(&Tim3_Handle, TIM_CHANNEL_1, 2208*speedFactor);
				__HAL_TIM_SET_COUNTER(&Tim3_Handle, 0x0000); 
					//Tim3_Handle.Instance->CCR1 = 2208 * speedFactor;
				}
				else if(!fullstepping){
				__HAL_TIM_SET_COMPARE(&Tim3_Handle, TIM_CHANNEL_1, 1104*speedFactor);
				__HAL_TIM_SET_COUNTER(&Tim3_Handle, 0x0000); 
					//Tim3_Handle.Instance->CCR1 = 1104 * speedFactor;
				}
			}
			
			displayVals();
		} //end of if PIN_23
}



void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef * htim) //see  stm32fxx_hal_tim.c for different callback function names. 
{																																//for timer4 
			
		if ((*htim).Instance==TIM3){
			
			BSP_LED_Toggle(LED4);
			
			switch(state){//Switch to update pins
				
				case 1:
				{
					
					//Set pins
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_SET);
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
					break;
				}
				case 2:
				{
					
					//Set pins
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_SET);
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_SET);
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
					break;
				}
				case 3:
				{
					
					//Set pins
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_SET);
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
					break;
				}
				case 4:
				{
					
					//Set pins
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_SET);
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
					break;
				}
				case 5:
				{
					
					//Set pins
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
					break;
				}
				case 6:
				{
					
					//Set pins
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
					break;
				}
				case 7:
				{
					
					//Set pins
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
					break;
				}
				case 8:
				{
					
					//Set pins
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_SET);
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
					break;
				}
			}
			
			if(clockwise){//If clockwise update accordingly
				
				if(fullstepping){//Next state based on step
					if(state < 7)
						state += 2;
					else
						state = 1;
				}
				else if(!fullstepping){
					if(state < 8)
						state += 1;
					else
						state = 1;
				}
			}
			else if(!clockwise){//If not clockwise update accordingly
				
				if(fullstepping){//Next state based on step
					if(state > 1)
						state -= 2;
					else
						state = 7;
				}
				else if(!fullstepping){
					if(state > 1)
						state -= 1;
					else
						state = 8;
				}
			}
			
			//Clear Counter
			__HAL_TIM_SET_COUNTER(htim, 0x0000); 
			
		}
			
	
}

void newState (void)
{
	
	//Move to closest full step
	if(fullstepping)
	{
	 if(clockwise)//If clockwise, go up to next full step
	 {
		 if(state%2==0 && state<8)
			 {
				 completeHalfStep();
				 state += 1;
			 }
			 else if(state==8)
			 {
				 completeHalfStep();
				 state = 1;
			 }
	 }
	 else if(!clockwise)//If counter clockwise, go down to next full step
	 {
		 if(state%2==0 && state>1)
			 {
				 completeHalfStep();
				 state -= 1;
			 }
	 }
 }

}


void completeHalfStep(void)
{
	
	//Completes half step currently on if even
	switch(state){//Switch to update pins
	
	case 1:
	{
		
		//Set pins
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
		break;
	}
	case 2:
	{
		
		//Set pins
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
		break;
	}
	case 3:
	{
		
		//Set pins
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
		break;
	}
	case 4:
	{
		
		//Set pins
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
		break;
	}
	case 5:
	{
		
		//Set pins
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
		break;
	}
	case 6:
	{
		
		//Set pins
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
		break;
	}
	case 7:
	{
		
		//Set pins
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
		break;
	}
	case 8:
	{
		
		//Set pins
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
		break;
	}
}
}

void displayVals(void)
{
	if(fullstepping)
	{
		LCD_DisplayString(4, 0, (uint8_t*) "Full-Stepping");
	}
	else
	{
		LCD_DisplayString(4, 0, (uint8_t*) "Half-Stepping");
	}
	
	if(clockwise)
	{
		LCD_DisplayString(7, 0, (uint8_t*) "Clockwise        ");
	}
	else
	{
		LCD_DisplayString(7, 0, (uint8_t*) "Counter Clockwise");
	}
	
	LCD_DisplayFloat(10, 0, 1/speedFactor, 2);
}

static void Error_Handler(void)
{
  /* Turn LED4 on */
  BSP_LED_On(LED4);
  while(1)
  {
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
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif
/**
  * @}
  */

/**
  * @}
  */



