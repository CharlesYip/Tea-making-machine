// EEE4099F Flywheel Project Code
// INCLUDE FILES
//====================================================================
#include "lcd_stm32f0.h"
#include "stm32f0xx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//====================================================================
// GLOBAL CONSTANTS
//====================================================================
#define SW0 (GPIOA -> IDR & GPIO_IDR_0)
#define SW1 (GPIOA -> IDR & GPIO_IDR_1)
#define SW2 (GPIOA -> IDR & GPIO_IDR_2)
#define GPIO_AFRH_PIN10_AF2 0b00000000000000000000001000000000
#define GPIO_AFRH_PIN11_AF2 0b00000000000000000010000000000000
//====================================================================
// FUNCTION DECLARATIONS
//====================================================================
void init_ports();
void init_TIM2();
void init_ADC();
//void EXT4();
//void EXTI4_15_IRQHandler();
void init_timer(void);

void drop_ind();
void check_adc5();
void check_adc6();
void check_button();
void screen_home();
void screen_tea();
void screen_sugar();
void screen_wait();
void check_screen();

void delay_loop();
void conv_sugar();
void conv_tea();
void display_confirm();
void display_tea();
void display_sugar();
void temperature_control();
//====================================================================
// DEFINE VARIABLES
//====================================================================
uint16_t pre_adc = 0;
int sw0;
int sw1;
int sw2;
uint16_t sugar_level;
char* level;
char* tea;
int ready;
int sel_tea;
int sel_sugar = 0;
int screen_number = 0;
int dummy = 5;
int teafeed = 0;
int sugarfeed = 0;
int time = 180000;
int temp_adc;
int temp;
//====================================================================
// INITILIZATION
//====================================================================
void init_timer(void){
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

	GPIOB->MODER |= GPIO_MODER_MODER10_1; // PB10 = AF
	GPIOB->MODER |= GPIO_MODER_MODER11_1; // PB11 = AF
	GPIOB->AFR[1] |= GPIO_AFRH_PIN10_AF2; // PB10_AF = AF2 (ie: map to TIM2_CH3)
	GPIOB->AFR[1] |= GPIO_AFRH_PIN11_AF2; // PB11_AF = AF2 (ie: map to TIM2_CH4)

	TIM2->CCMR2 &= ~TIM_CCMR1_CC1S;		// Configure TIM2 for OC

	TIM2->ARR = 8000;  // f = 1 KHz

	// specify PWM mode: OCxM bits in CCMRx. We want mode 1
	TIM2->CCMR2 |= (TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1); // PWM Mode 1
	TIM2->CCMR2 |= (TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1); // PWM Mode 1

	// set PWM percantages;
	TIM2->CCR4 = teafeed * 80;
	TIM2->CCR3 = sugarfeed * 80;

	// enable the OC channels
	TIM2->CCER |= TIM_CCER_CC3E;
	TIM2->CCER |= TIM_CCER_CC4E;
	TIM2->CR1 |= TIM_CR1_CEN; // counter enable*/
}

void init_ports()
{
	RCC->AHBENR |= (RCC_AHBENR_GPIOAEN |			// Sending power to GPIOA & GPIOB
					RCC_AHBENR_GPIOBEN);

	GPIOB->MODER   |= (GPIO_MODER_MODER0_0|			// Using PB0 to enable heating element
					   GPIO_MODER_MODER1_0|			// Using PB1 to enable water pump
					   GPIO_MODER_MODER2_0|
					   GPIO_MODER_MODER3_0);

	//GPIOB->AFR[1]  |= (GPIO_AFRH_PIN10_AF2|	    // Setting PB10 & PB11 to AF2
				       //GPIO_AFRH_PIN11_AF2);
	GPIOA -> MODER |=  (GPIO_MODER_MODER5|		// Set PA5 analog mode
					    GPIO_MODER_MODER6);		// Set PA6 to analog mode

	GPIOA -> PUPDR |= (GPIO_PUPDR_PUPDR0_0		// Pull up resistors to SW0
					  |GPIO_PUPDR_PUPDR1_0
					  |GPIO_PUPDR_PUPDR2_0);	// Pull up resistors to SW1


}

void init_ADC()
{
	RCC -> APB2ENR |= RCC_APB2ENR_ADCEN;		// Sending power to the ADC
	ADC1 -> CFGR1 &= ~ADC_CFGR1_RES;			// Setting the resolution to 12-bits
	//ADC1->CFGR1 |= ADC_CFGR1_RES_1;
	ADC1 -> CFGR1  &= ~ADC_CFGR1_ALIGN;			// Setting the ADC as right aligned
	ADC1 -> CFGR1  &= ~ADC_CFGR1_CONT;			// Setting the ADC to single-shot mode
	ADC1 -> CHSELR |= ADC_CHSELR_CHSEL6;		// Selecting Channel 6
	ADC1 -> CR |= ADC_CR_ADEN;					// Starting the ADC
	while ((ADC1 -> ISR & ADC_ISR_ADRDY) == 0);	// Wait until the ADC is ready
}

//====================================================================
// FUNCTIONS
//====================================================================
void check_adc5(void)							// This is a function where we can check the ADC value from PA5
{
	ADC1 -> CHSELR |= ADC_CHSELR_CHSEL5;		// Selecting Channel 5
	ADC1 -> CR |= ADC_CR_ADSTART;				// Start the conversion
	while ((ADC1 -> ISR & ADC_ISR_EOC) == 0);	// Wait for the conversion to end
}

void check_adc6(void)							// This is a function where we can check the ADC value from PA6
{
	ADC1 -> CHSELR |= ADC_CHSELR_CHSEL6;		// Selecting Channel 6
	ADC1 -> CR |= ADC_CR_ADSTART;				// Start the conversion
	while ((ADC1 -> ISR & ADC_ISR_EOC) == 0);	// Wait for the conversion to end
}

void check_button()								// Check button state
{
	if(screen_number >= 0 && screen_number <4)	// Make sure that the screen number is in between the valid value
	{
		if(SW0 == 0)							// If confirm button is pressed
		{
			screen_number = screen_number + 1;	// Increase the screen number -> move to the next screen interface
		}
		if(SW1 == 0)							// If the cancel button is pressed
		{
			screen_number = screen_number - 1;	// Decrease the screen number -> move to the previous screen interface
		}
		for (int i = 0; i <100000; i++);		// Delay, button debounce
	}
}

void display_tea()								// Update tea type when user is selecting
{
		check_adc5();
		conv_tea();
		lcd_command(LINE_TWO);
		lcd_putstring(tea);
}

void conv_tea(void)								// Do logic check what type of tea is the user picking
{
	pre_adc = ADC1->DR;							// Getting ADC value from Data register
	if(pre_adc < 1365)
	{
		tea = "White Tea       ";
		sel_tea = 1;
	}

	if(pre_adc < 2730 && pre_adc >1365)
	{
		tea = "Green Tea       ";
		sel_tea = 2;
	}
	if(pre_adc > 2730)
	{
		tea = "Oolong Tea      ";
		sel_tea = 3;
	}
}

void display_sugar()							// Update sugar level when user is doing the selection
{
		check_adc5();
		conv_sugar();
		lcd_command(LINE_TWO);
		lcd_putstring(level);
}

void conv_sugar()								// Sugar level logic, only given the user an option from 0 tsp to 4 tsp
{
	sugar_level = ADC1->DR;

	if(sugar_level < 807)
	{
		level = "0 tsp           ";
		sel_sugar = 0;
	}
	if(sugar_level > 807 && sugar_level < 1614)
	{
		level = "1 tsp           ";
		sel_sugar = 1;
	}
	if(sugar_level > 1614 && sugar_level < 2421)
	{
		level = "2 tsp           ";
		sel_sugar = 2;
	}
	if(sugar_level > 2421 && sugar_level < 3228)
	{
		level = "3 tsp           ";
		sel_sugar = 3;
	}
	if(sugar_level > 3228)
	{
		level = "4 tsp max       ";
		sel_sugar = 4;
	}
}

void display_confirm()							// Update screen and display the order to user -> confirmation
{
	lcd_command(LINE_TWO);
	char confirm[16];
	char* a;
	char* b;
	if(sel_tea == 1)
	{
		a = "   White Tea ";
	}
	if(sel_tea == 2)
	{
		a = "   Green Tea ";
	}
	if(sel_tea == 3)
	{
		a = "  Oolong Tea ";
	}
	if(sel_sugar == 0)
	{
		b = "0";
	}
	if(sel_sugar == 1)
	{
		b = "1";
	}
	if(sel_sugar == 2)
	{
		b = "2";
	}
	if(sel_sugar == 3)
	{
		b = "3";
	}
	if(sel_sugar == 4)
	{
		b = "4";
	}

	strcpy(confirm, a);
	strcat(confirm, b);
	//confirm = [a,b];
	lcd_putstring(confirm);
}

void screen_home()								// Home page screen
{
	lcd_command(CLEAR);
	lcd_putstring("   Welcome to   ");
	lcd_command(LINE_TWO);
	lcd_putstring("   UniversiTea  ");
}

void screen_tea()								// Selection of tea screen page
{
	lcd_command(CLEAR);
	lcd_putstring("Select your tea ");
}

void screen_sugar()								// Sugar level screen page
{
	lcd_command(CLEAR);
	lcd_putstring("Sugar Level    ");
}

void screen_confirm()							// Confirmation screen page
{
	lcd_command(CLEAR);
	lcd_putstring(" Confirm order ");
}

void screen_wait()								// Loading screen while tea is been make
{
	lcd_command(CLEAR);
	lcd_putstring(" Making Tea Now  ");
	lcd_command(LINE_TWO);
	lcd_putstring("     Relax       ");
}

void check_screen()								// Running logic on screen number to see which screen page should be displayed
{												// Make sure if the current screen number is same as previous screen number, do not refresh screen -> avoid LCD blinking
	if (screen_number == 1)
	{
		if (dummy == screen_number)
		{
			check_button();
			display_tea();
			//temperature_control();
		}
		else
		{
			//temperature_control();
			screen_tea();
			display_tea();
			dummy = 1;
		}
	}
	if (screen_number == 2)
	{
		if (dummy == screen_number)
		{
			check_button();
			display_sugar();
		}
		else
		{
			screen_sugar();
			display_sugar();
			dummy = 2;
		}
	}
	if (screen_number == 3)
	{
		if (dummy == screen_number)
		{
			check_button();
			display_confirm();
		}
		else
		{
			screen_confirm();
			dummy = 3;
		}
	}

	if (screen_number == 4)
	{
		if (dummy == screen_number)
		{
			drop_ind();
			//make_tea();
			screen_number = 0;
			screen_home();
		}
		else
		{
			screen_wait();
			dummy = 4;
		}
	}
}

void drop_ind()									// Function where we control the PWM to drop ingredients
{
	if (sel_tea == 1)
	{
		teafeed = 50;
		TIM2->CCR4 = teafeed * 80;
		delay(time);
		teafeed = 0;
		TIM2->CCR4 = teafeed * 80;
	}
	if(sel_tea == 2)
	{
		teafeed = 50;
		TIM2->CCR4 = teafeed * 80;
		delay(time*2);
		teafeed = 0;
		TIM2->CCR4 = teafeed * 80;
	}
	if(sel_tea == 3)
	{
		teafeed = 50;
		TIM2->CCR4 = teafeed * 80;
		delay(time*3);
		teafeed = 0;
		TIM2->CCR4 = teafeed * 80;
	}



	if (sel_sugar == 0)
	{
		sugarfeed = 50;
		TIM2->CCR3 = sugarfeed * 80;
		delay(0);
		sugarfeed = 0;
		TIM2->CCR3 = sugarfeed * 80;
	}
	if (sel_sugar == 1)
	{
		sugarfeed = 50;
		TIM2->CCR3 = sugarfeed * 80;
		delay(time*2);
		sugarfeed = 0;
		TIM2->CCR3 = sugarfeed * 80;
	}
	if (sel_sugar == 2)
	{
		sugarfeed = 50;
		TIM2->CCR3 = sugarfeed * 80;
		delay(time*3);
		sugarfeed = 0;
		TIM2->CCR3 = sugarfeed * 80;
	}
	if (sel_sugar == 3)
	{
		sugarfeed = 50;
		TIM2->CCR3 = sugarfeed * 80;
		delay(time*4);
		sugarfeed = 0;
		TIM2->CCR3 = sugarfeed * 80;
	}
	if (sel_sugar == 4)
	{
		sugarfeed= 50;
		TIM2->CCR3 = sugarfeed * 80;
		delay(time*5);
		sugarfeed = 0;
		TIM2->CCR3 = sugarfeed * 80;
	}
}

void make_tea()									// Function where we control the temperature
{
	if (sel_tea == 1)							// Check if the Red tea is selected
	{
		temperature_control();					// Get temperature reading from thermocouple
		GPIOB->ODR = 1;							// Turn on the heating element
		while(temp < 80)						// Stay in while loop while temp is below the desired temperature
		{
			temperature_control();				// Update temperature
		}
		GPIOB->ODR = 0;							// Once temperature reached desired temperature, heating element turn off
	}
	if (sel_tea == 2)							// Check if the Green tea is selected
	{
		temperature_control();					// Get temperature reading from thermocouple
		GPIOB->ODR = 1;							// Turn on the heating element
		while (temp < 70)						// Stay in while loop while temperature is below the desired temperature
		{
			temperature_control();				// Update temperature
		}
		GPIOB->ODR = 0;							// Once temperature reached desired temperature, heating element turn off
	}
	if (sel_tea == 3)							// Check if the Oolong tea is selected
	{
		temperature_control();					// Get temperature reading from thermocouple
		GPIOB->ODR = 1;							// Turn on the heating element
		while (temp < 85)						// Stay in while loop while temperature is below the desired temperature
		{
			temperature_control();				// Update temperature
		}
		GPIOB->ODR = 0;							// ONce temperature reached desired temeprature, heating element turn off
	}
}

void temperature_control()						// Update ADC PA6 from the thermocouple amplifier
{
	check_adc6();
	//char dis_temp[16];
	temp_adc = ADC1->DR;
	temp = ((temp_adc - 1.25)/0.005);			// Calculate the actual temperature in Celusis
}

//====================================================================
// Main code
//====================================================================
int main(void)
{
	init_ports();
	init_ADC();
	init_LCD();
	init_timer();
	screen_home();

	while(1)
	{
		check_button();
		check_screen();
	}
}


