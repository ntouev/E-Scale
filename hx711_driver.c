/* Function Prototypes -------------------------------------------------------*/
void hx711_GPIO_Init(void);
uint32_t HX711_raw_data(void);
uint32_t HX711_cooked_data(void);
void HX711_Calibration(void);
__STATIC_INLINE void delay_us(uint32_t microseconds);

/* Global Vars ---------------------------------------------------------------*/
#define AVERAGE_SAMPLES                 2
// FL_RCVD_STRING is a string used by calibration process for synchronization purposes
// during I-O with the user. It is set in an USART interrupt handler whenever a string is received.
// This code is not added here.


uint32_t sum_zero_raw_value             // The sum of AVERAGE_SAMPLES (2) raw sensor values with zero load
uint32_t sum_reference_raw_value        // The sum of AVERAGE_SAMPLES (2) raw sensor values with reference load
uint32_t ref_actual_weight              // The actual weight of the reference load in gramms.

uint8_t FL_IS_CALIBRATED = 0;           // Flag to indicate that calibration process is over. Needed for application purposes.

/**
  * @brief hx711_GPIO_Init Initialization Function
  * @param None
  * @retval None
  */
static void hx711_GPIO_Init(void)
{
	LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);

	  /**LPUART1 GPIO Configuration
	  pc10   ------> hx711_dt
	  pc12   ------> hx711_sck
	  */
	  GPIO_InitStruct.Pin = HX711_SCK_PIN;
	  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
	  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	  LL_GPIO_Init(HX711_PORT, &GPIO_InitStruct);

	  GPIO_InitStruct.Pin = HX711_DT_PIN;
	  GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
	  LL_GPIO_Init(HX711_PORT, &GPIO_InitStruct);

	  LL_GPIO_ResetOutputPin(HX711_PORT, HX711_SCK_PIN);
}

/**
  * @brief HX711_Calibration  Function
  * @param None
  * @retval None
  */
void HX711_Calibration(void)
{
	for(uint8_t i=0; i<AVERAGE_SAMPLES; i++)
	{
		sum_zero_raw_value += HX711_raw_data();
	}

	LPUART_SendString("Place a reference weight and press any key to start calibration process ... \r\n\0");

    while (FL_RCVD_STRING == 0)
    	;
    FL_RCVD_STRING = 0;

	for(uint8_t i=0; i<AVERAGE_SAMPLES; i++)
	{
		sum_reference_raw_value += HX711_raw_data();
	}

    LPUART_SendString("Give the reference actual weight in g.\r\n\0");

    while (FL_RCVD_STRING == 0)
    	;
    FL_RCVD_STRING = 0;

    // need to parse the input here
    // I put it hardcoded for now ...
    ref_actual_weight = 330;

    FL_IS_CALIBRATED = 1;
}

/**
  * @brief HX711_raw_data  Function
  * @param None
  * @retval Cooked sensor value
  */
uint32_t HX711_cooked_data(void)
{
	uint32_t sum_raw_value = 0;
	uint32_t cooked_value = 0;

	for(uint8_t i=0; i<AVERAGE_SAMPLES; i++)
	{
		sum_raw_value += HX711_raw_data();
	}

	if (sum_raw_value >= sum_zero_raw_value)
	{
		cooked_value = ((sum_raw_value - sum_zero_raw_value) * ref_actual_weight) / \
											(sum_reference_raw_value - sum_zero_raw_value);
	}
	else
	{
		cooked_value = 0;
	}

	return cooked_value;
}


/**
  * @brief HX711_raw_data  Function
  * @param None
  * @retval Raw sensor value
  */
uint32_t HX711_raw_data(void)
{
    uint32_t count = 0;

    // csk should be low here
    LL_GPIO_ResetOutputPin(HX711_PORT, HX711_SCK_PIN);

    // wait till DT goes to low --> data ready to be retrieved
    // XI = 0 --> data rate is 10Hz
    while(LL_GPIO_IsInputPinSet(HX711_PORT, HX711_DT_PIN))
	    ;

    for(uint8_t i=0; i<24; i++)
    {
	    LL_GPIO_SetOutputPin(HX711_PORT, HX711_SCK_PIN);
        delay_us(1);

        count = count << 1;

        LL_GPIO_ResetOutputPin(HX711_PORT, HX711_SCK_PIN);
        delay_us(1);

        if(LL_GPIO_IsInputPinSet(HX711_PORT, HX711_DT_PIN))
        	count++;
    }

    // one more pulse is needed to pull DT back to high and
    // be ready for the next Conversion Complete event
    // Channel A, Gain = 128
    LL_GPIO_SetOutputPin(HX711_PORT, HX711_SCK_PIN);
    delay_us(1);

    count = count ^ 0x800000;	//;

    LL_GPIO_ResetOutputPin(HX711_PORT, HX711_SCK_PIN);
    delay_us(1);

    return count;
}


/**
  * @brief delay_us  Function
  * @param None
  * @retval None
  */
__STATIC_INLINE void delay_us(uint32_t microseconds)
{
  uint32_t dwt_cycles_start = DWT->CYCCNT;
  uint32_t microseconds_ticks = microseconds * (RCC_Clocks.HCLK_Frequency / 1000000);

  while ((DWT->CYCCNT - dwt_cycles_start) < microseconds_ticks)
	  ;
}
