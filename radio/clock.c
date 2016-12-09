#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "si4734.h"
#include "twi_master.h"
#include "uart_functions.h"

volatile uint8_t STC_interrupt;  //flag bit to indicate tune or seek is done

volatile uint16_t eeprom_fm_freq;
volatile uint16_t eeprom_am_freq;
volatile uint16_t eeprom_sw_freq;
volatile uint8_t  eeprom_volume;

volatile uint16_t current_fm_freq;
volatile uint16_t current_am_freq;
volatile uint16_t current_sw_freq;
volatile uint8_t  current_volume;

//Used in debug mode for UART1
char uart1_tx_buf[40];      //holds string to send to crt
char uart1_rx_buf[40];      //holds string that recieves data from uart

ISR(INT7_vect){
	_delay_ms(1000);
	STC_interrupt = TRUE;
	PORTB ^= 0x80;
}

int main()
{
	DDRB = 0xFF;
	PORTB = 0x00;
	init_twi();
	uart_init();

	TCCR3A = (1<<COM3C1) | (0<<COM3C0) | (1<<WGM31) | (0<<WGM30);
	TCCR3B = (1<<WGM33) | (1<<WGM32) | (1<<CS30);
	ICR3   = 0x1000;
	OCR3C  = 0x0800;

	//RADIO SETUP
	DDRE  |= 0x04; //Port E bit 2 is active high reset for radio 
	PORTE |= 0x04; //radio reset is on at powerup (active high)

	PORTB |= 0x01;

	//hardware reset of Si4734
	PORTE &= ~(1<<PE7); //int2 initially low to sense TWI mode
	DDRE  |= 0x80;      //turn on Port E bit 7 to drive it low
	PORTE |=  1<<PE2; //hardware reset Si4734 
	_delay_us(200);     //hold for 200us, 100us by spec         
	PORTE &= ~(1<<PE2); //release reset 
	_delay_us(30);      //5us required because of my slow I2C translators I suspect
			//Si code in "low" has 30us delay...no explaination
	DDRE  &= ~(0x80);   //now Port E bit 7 becomes input from the radio interrupt

	PORTB |= 0x02;

	//EXT INT SETUP
	EICRB |= (1<<ISC70) | (0<<ISC71);
	EIMSK |= (1<<INT7);

	DDRE  |= 1<<5;
	PORTE |= 1<<5;
	sei();

	_delay_ms(1000);

	fm_pwr_up(); //powerup the radio as appropriate
	_delay_ms(1000);
	PORTB |= 0x04;
	current_fm_freq = 10790; //arg2, arg3: 99.9Mhz, 200khz steps
	fm_tune_freq(); //tune radio to frequency in current_fm_freq
	current_fm_freq = 10790; //arg2, arg3: 99.9Mhz, 200khz steps
	fm_tune_freq(); //tune radio to frequency in current_fm_freq
	current_fm_freq = 10790; //arg2, arg3: 99.9Mhz, 200khz steps
	fm_tune_freq(); //tune radio to frequency in current_fm_freq
	PORTB |= 0x08;

	while(1){     //do forever

	} //while 
} //main
