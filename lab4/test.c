#include "kellen_music.h"
#include <avr/interrupt.h>

#define TOGGLE_VALUE (0x0fff/2)
#define TEST_PIN 4

// PORTC bit 7 -> alarm signal.
// PORTE bit 5 -> volume.
extern volatile uint16_t beat;

void init_tcnt3(){
	TCCR3A = (1<<COM3C1) | (0<<COM3C0) | (1<<WGM31) | (0<<WGM30);
	TCCR3B = (1<<WGM33) | (1<<WGM32) | (1<<CS30);
	ICR3   = 0x0fff;
	OCR3C  = TOGGLE_VALUE;
}

void init_tcnt2(){
	TIMSK |= (1<<TOIE2);							 //enable timer overflow interrupt.
	TCCR2 |= (1<<WGM20)|(1<<WGM21)|(1<<COM21)|(1<<COM20)|(1<<CS22)|(0<<CS21)|(0<<CS20); //Fast PWM, (non-inverting), 256 prescale 
}

// 16M/(256^2) = 244.14 per second
ISR(TIMER2_OVF_vect){
	static uint8_t counter  = 0;
	static uint8_t ms   = 0;

	if( !(counter%128) ){
		ms++;
		if(ms % 8 == 0) {
			//for note duration (64th notes) 
			beat++;
		}
	}
}

int main()
{
	uint8_t i = 0;
	DDRB	|= 0xFF;
	DDRC	|= 0xFF;
	DDRD	|= 0x84; //set port D bit 2, 7 as output
	DDRE	|= 0xff;

	PORTB   |= (1<<PB7);

	init_tcnt3();
	init_tcnt2();

	sei();

	music_init();
	music_on();
	while(1){

		if (TCNT3 < TOGGLE_VALUE)
			PORTE |= 1<<TEST_PIN;
		else
			PORTE &= ~(1<<TEST_PIN);
	  } //while
}  //main
