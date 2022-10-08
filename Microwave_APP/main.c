/*
 * Microwave_APP.c
 *
 * Created: 10/4/2022 12:02:45 PM
 * Author : Abdalrahman_Amin
 */ 

#include "header.h"

#define _sec1 15625
   
uint8_t M,S;

void ReadTime_from_keypad();
void WriteTime_on_LCD();
void turn_on();
void turn_off();
void interrupt_hand();

int main(void)
{
	interrupt_hand();
	M=0 ; S=0;
	LCD_init();
	KEYPAD_init();
	
	sei(); //enable global interrupt
	
	CLRBIT(DDRB,0); // ip for weight sens
	SETBIT(DDRB,5);// for heating 
	SETBIT(DDRB,6);// for lamp on 
	SETBIT(DDRB,7);// for motor
	 
	LCD_write_command(0x01);
    ReadTime_from_keypad();
	//TIMER1A_init_compare(15625);
    while (1) 
    {	
		if (M==0 && S==0){LCD_write_command(0x01);	ReadTime_from_keypad();}
		else WriteTime_on_LCD();
	}

}


ISR (TIMER1_COMPA_vect)
{	
	if(S==0 && M == 0){
		turn_off();
		return;// out 
	}
	if (S==0)
	{	M--; S=59;	}
	else  --S;	
}

//start button

ISR(INT0_vect){ 
	if( (M !=0) && READBIT(PINB,0) && (! READBIT(PINB,2)) )  turn_on(); 
	else 
	{	
		LCD_write_command(0x01);
		LCD_write_command(0x83); 
		LCD_write_string("WARNING");
		_delay_ms(100);  
		LCD_write_command(0x01);
	}
}



//cancel butt 
ISR(INT1_vect){
	M=0;S=0;
	if( READBIT(PORTB,6) ) turn_off();
	else LCD_write_command(0x01);
}

//door sens
ISR(INT2_vect){
	if(READBIT(PORTB,6)) turn_off();
}

void ReadTime_from_keypad(){
	
	for(uint8_t i=0 ;i<2;i++){
		while(! KEYPAD_read() );
		if(KEYPAD_read() != '='){
			M*=10;
			M+= (KEYPAD_read()-'0');
			while(KEYPAD_read());
			_delay_ms(50);
		}
		else  break;
	}
	WriteTime_on_LCD();
}

void WriteTime_on_LCD(){

	LCD_write_command(0x84);
	if (M<10)
	{
		LCD_write_char('0');
		LCD_write_num(M);
	}
	else LCD_write_num(M);
	LCD_write_char(':');
	if (S<10)
	{
		LCD_write_char('0');
		LCD_write_num(S);
	}
	else LCD_write_num(S);
	
}

void interrupt_hand(){
	CLRBIT(DDRD,2);// int0 sta
	CLRBIT(DDRD,3);// int1 sto
	CLRBIT(DDRB,2);// int2 door
	// make interrupt mode rising mode  1 1
	SETBIT(MCUCR,0);
	SETBIT(MCUCR,1);
	
	SETBIT(GICR,5);
	SETBIT(GICR,6);
	SETBIT(GICR,7);
	
	//sei();	
}

void turn_on(){
	TIMER1A_init_compare(15625);
	SETBIT(PORTB,5);// ONN  heating
	SETBIT(PORTB,6);// ONN  Lamp
	SETBIT(PORTB,7);// ONN  motor
	
}
void turn_off(){
	Timer1A_stop();
	LCD_write_command(0x01);
	LCD_write_command(0x82);
	LCD_write_string("FINISHED");
	CLRBIT(PORTB,5);// Off  heating
	CLRBIT(PORTB,6);// Off  Lamp
	CLRBIT(PORTB,7);// Off  motor
	_delay_ms(100);
	LCD_write_command(0x01);
	
}