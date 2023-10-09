/*
 * main.c
 *
 *  Created on: Aug 18, 2023
 *      Author: user
 */

#define F_CPU     						8000000UL

#include "LBIT_math.h"
#include "LSTD_types.h"
#include <util/delay.h>

#include "MGIE_Interface.h"
#include "MDIO_interface.h"
#include "MTIM_Interface.h"
#include "MEXTI_interface.h"

#include "HLCD_interface.h"
#include "HKEYPAD_interface.h"

/*
 *	need to read from Keypad number that number represents Minutes
 *	we can display it on LCD --> XX:YY where XX is minutes and YY is seconds
 *	also we we will display the delay on LCD every 1-sec
 *	lets say we need to delay 5 min
 *	LCD -->  05:00 first then after 1sec it must be 04:59
 *	then 04:58	if the delay is 00:00 it must stop and display finish and stop timer
 * 	if min =5:00 --> then 5*60 == 300 --> 299 sec , 299/60 = 4.9 --> 04 :
 * 	299%60 == 59 sec  -- > 04:59 ; prescaler --> 32
 * 	SWC -->
 *
 * 	Abstraction layers
 * 	APP  --> App.c
 * 	HAL  --> HLCD , HKPD.
 * 	MCAL --> MDIO, MTIM, MGIE.
 * 	Lib  --> LBIT_math, LSTD_types.
 *
 * 	we will use only one timer that generate interrupt every 1sec
 *
 */

/* 1st Variables used to save Minutes and Seconds */
volatile u8 G_u8MinutesValue=0,G_u8SecondsValue=0;
u8 G_u8Flag=0;
#define OK		 	0
#define WARNING 	1
#define ERROR 		2
#define FINISHED	3

/* will be used for MTIM_ISR */
void ChangeTimeCallBack(void){
	// 1st
	//special condition to stop timer
	if(0==G_u8MinutesValue  && 0==G_u8SecondsValue)
	{
		// stop timer
		MTIM_stderrStopTimer(MTIM_TIMER0);

	}
	else if( 0!=G_u8MinutesValue && 0==G_u8SecondsValue ){
		G_u8SecondsValue=59;
		G_u8MinutesValue--;
	}
	else{
		G_u8SecondsValue--;
	}

}

/* will used to read from keypad and save this in minutes only read minutes */
void ReadEnteredTime(void){
	// read max two digits if user enter special char that mean finish & start timer
	u8 L_u8i;
	G_u8MinutesValue=0;
	G_u8SecondsValue=0;
	for(L_u8i=0;L_u8i<2;L_u8i++)
	{
		if('*'==HKPD_charGetKey() )
		{
			/* save data input 1 digit */
			break ;
		}
		else
		{
			G_u8MinutesValue *=10; // store
			G_u8MinutesValue += (HKPD_charGetKey() - '0');
		}
		_delay_ms(300);
	}
}

/* will be used to display Time on LCD Like XX:YY */
void DisplayTimeOnLCD(void){
	/* display minutes then : then Num */
	HLCD_voidSetCur(1,3);
	if(10>G_u8MinutesValue)
	{
		HLCD_voidSendChar('0');
		HLCD_voidSendNum((u16) G_u8MinutesValue);
	}
	else
	{
		HLCD_voidSendNum((u16) G_u8MinutesValue);
	}
	HLCD_voidSendChar(':');
	if(10>G_u8SecondsValue)
	{
		HLCD_voidSendChar('0');
		HLCD_voidSendNum((u16) G_u8SecondsValue);
	}
	else
	{
		HLCD_voidSendNum((u16) G_u8SecondsValue);
	}

}

void StartButtonFunc(void)
{
	u8 L_u8WeightSensor= 0;
	u8 L_u8DoorState= 0 ;
	MDIO_stderrGetPinValue(&L_u8WeightSensor, MDIO_PORTB, MDIO_PIN0);
	MDIO_stderrGetPinValue(&L_u8DoorState, MDIO_PORTB, MDIO_PIN2);
	/*
	 *  Minutes entered not 0 , weight sensor is 1 indicating there
	 *  is food Door state is 1 indicate door is close
	 */
	if( 0!=G_u8MinutesValue && 1 == L_u8WeightSensor && 1==L_u8DoorState  )
	{
		/* start timer, Led On, Heating on  */
		MTIM_stderrStartTimer(MTIM_TIMER0);
		MDIO_stderrSetPinValue(MDIO_PORTB, MDIO_PIN3, MDIO_HIGH); /* LED */
		MDIO_stderrSetPinValue(MDIO_PORTB, MDIO_PIN4, MDIO_HIGH); /* Heating */
	}
	else
	{
		G_u8Flag= WARNING; /* Warning message */
	}
}

/* have 2 options  to stop Microwave or cancel the entered time before start */
void StopButtonFunc(void)
{
	u8 L_u8State=0;
	G_u8MinutesValue=0;
	G_u8SecondsValue=0;
	MDIO_stderrGetPinValue(&L_u8State,MDIO_PORTB,MDIO_PIN3); /* Read Led pin */
	if(1==L_u8State)
	{
		/* Microwave is on then stop */
		MTIM_stderrStopTimer(MTIM_TIMER0);
		MDIO_stderrSetPinValue(MDIO_PORTB, MDIO_PIN3, MDIO_LOW); /* LED */
		MDIO_stderrSetPinValue(MDIO_PORTB, MDIO_PIN4, MDIO_LOW); /* Heating */
		G_u8Flag=FINISHED; /* display finished */
	}
	else
	{
//		HLCD_voidClear();
		G_u8Flag=OK;

	}

}

/* have 2 options  to stop Microwave or cancel the entered time before start */
void DoorOpenFunc(void)
{
	u8 L_u8State=0;
	G_u8MinutesValue=0;
	G_u8SecondsValue=0;
	MDIO_stderrGetPinValue(&L_u8State,MDIO_PORTB,MDIO_PIN3); /* Read Led pin */
	if(1==L_u8State)
	{
		/* Microwave is on then stop */
		MTIM_stderrStopTimer(MTIM_TIMER0);
		MDIO_stderrSetPinValue(MDIO_PORTB, MDIO_PIN3, MDIO_LOW); /* LED */
		MDIO_stderrSetPinValue(MDIO_PORTB, MDIO_PIN4, MDIO_LOW); /* Heating */
		G_u8Flag=FINISHED; /* display finished */
	}
	else
	{
		/* Do nothing */
	}

}


int main (void)
{
	/* Initialization  (Run Once)  */

	/* DIO INIT */
	MDIO_stderrSetPinDirection(MDIO_PORTD, MDIO_PIN2, MDIO_INPUT_PULLUP);	/* for start SW INT0 */
	MDIO_stderrSetPinDirection(MDIO_PORTD, MDIO_PIN3, MDIO_INPUT_PULLUP);	/* for stop SW INT1 */
	MDIO_stderrSetPinDirection(MDIO_PORTB, MDIO_PIN2, MDIO_INPUT_PULLUP);   /* for Door sensor INT2 */
	MDIO_stderrSetPinDirection(MDIO_PORTB, MDIO_PIN0, MDIO_INPUT_PULLUP);	/* for Weight sensor */
	MDIO_stderrSetPinDirection(MDIO_PORTB, MDIO_PIN3, MDIO_OUTPUT);   		/* for Led indicate On state */
	MDIO_stderrSetPinDirection(MDIO_PORTB, MDIO_PIN4, MDIO_OUTPUT);			/* for Heating */
	/* EXTI INIT */
	MEXTI_stderrInit(MEXTI_INT0 , MEXTI_MODE_FALLING);  /* configure the INTX    */
	MEXTI_stderrInit(MEXTI_INT1 , MEXTI_MODE_FALLING);
	MEXTI_stderrInit(MEXTI_INT2 , MEXTI_MODE_FALLING);
	MEXTI_stderrEnable(MEXTI_INT0);
	MEXTI_stderrEnable(MEXTI_INT1);
	MEXTI_stderrEnable(MEXTI_INT2);

	MEXTI_stderrCallBack(&StartButtonFunc,MEXTI_INT0);
	MEXTI_stderrCallBack(&StopButtonFunc,MEXTI_INT1);
	MEXTI_stderrCallBack(&DoorOpenFunc,MEXTI_INT2);
	/* TIM0 INIT */
	MTIM_stderrInit(MTIM_TIMER0, MTIM_CS_PRESCALER_1024,MTIM_MODE_NORMAL,MTIM_HWPIN_DISCONNECTED);
	MTIM_stderrSetCallBack(&ChangeTimeCallBack, MTIM_INTERRUPT_T0_OVF);
	MTIM_stderrEnableInterrupt(MTIM_INTERRUPT_T0_OVF);/* Enable Interrupt for T0 Overflow */

	MGIE_voidEnable();
	G_u8Flag=0;
	MTIM_stderrTimerDelay(MTIM_TIMER0, 1000); /* OVF every 1 Second */
	/* LCD INIT */
	HLCD_voidInit();

	HLCD_voidSendCmd(HLCD_CURSOR_OFF);

	while(1)
	{


		if(0==G_u8SecondsValue && 0==G_u8MinutesValue  && OK==G_u8Flag)
		{
			HLCD_voidSetCur(0,1);
			HLCD_voidSendString("New Delay");
			DisplayTimeOnLCD();
			ReadEnteredTime();
		}
		else if (WARNING==G_u8Flag || FINISHED==G_u8Flag  )
		{
			HLCD_voidClear();
			HLCD_voidSetCur(0,3);
			if(WARNING==G_u8Flag)
			{
				HLCD_voidSendString(" WARNING ...");
			}
			else
			{
				HLCD_voidSendString(" FINISHED ..");
			}
			_delay_ms(1000);
			HLCD_voidClear();
			G_u8Flag=OK;
		}
		else
		{
			DisplayTimeOnLCD();

		}

	}
	return 0 ;
}


