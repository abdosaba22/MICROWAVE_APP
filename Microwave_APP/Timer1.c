/*
 * Timer0.c
 *
 * Created: 10/4/2022 12:23:34 PM
 *  Author: DELL
 */ 
#include "Timer1.h"

/***************
FCPU = 16 Mhz ---> c.c = 1/16 usec
CTC_v = [time delay / (1/16 usec)] -1 

the implement of interrupt in ISR(TIMER1_COMPA_vect);
***************/

void TIMER1A_init_compare(uint16_t CTC_value){
	TCNT1=0;
	OCR1A= CTC_value;
	SETBIT(TIMSK,OCIE1A); /* enable compare interrupt */
	SETBIT(TCCR1A,FOC1A);
	SETBIT(TCCR1B,WGM12); /* compare mode enabled */
	SETBIT(TCCR1B,CS10); 
	SETBIT(TCCR1B,CS12); /*  prescaler  1024 and start timer*/
	//sei();
}

void Timer1A_stop(){
	TCCR1B=0;
	TIMSK=0;
}