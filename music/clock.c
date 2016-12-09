#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>

#include "music.h"

#define LED_DELAY 500
#define TEMPO 8 // 1/64th note = (256*128*TEMPO)/16Mhz

ISR(TIMER2_OVF_vect) {
	static uint8_t int_count = 0;

	if (++int_count < 128)
		return;
	int_count = 0;
uint8_t tempo_count =0;
	if(tempo_count >= TEMPO){
		//for note duration (64th notes) 
		beat++;
		tempo_count = 0;
	}
	tempo_count++;
}

int main()
{
	DDRE = 0xFF;
	DDRB = 0xff;
	DDRC = 0xFF;

	//TIMER 2 SETUP
	TIMSK |= (1<<TOIE2);					//enable overflow interrupt
	TCCR2 |= (1<WGM21) | (1<<WGM20) | (1<<CS20) | (1<<COM21) | (1<<COM20);	//Fast pwm, no prescale!!, inverting OC2
	OCR2 = 255;

	//TIMER 3 SETUP
	TCCR3A = (1<<COM3C1) | (0<<COM3C0) | (1<<WGM31) | (0<<WGM30);
	TCCR3B = (1<<WGM33) | (1<<WGM32) | (1<<CS30);
	ICR3   = 0x1000;
	OCR3C  = 0x0A00;

	music_init();
	sei();
music_on();
	while(1){     //do forever

//		_delay_ms(1);
//		PORTC ^= 0x80;
	} //while 
} //main
