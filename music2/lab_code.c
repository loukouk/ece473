#include <avr/io.h>
#include <util/delay.h>

int main()
{
	DDRA = 0xff;
	DDRB = 0xff;
	DDRC = 0xff;
	DDRE |= 1<<5;

	//TIMER 3 SETUP
	TCCR3A = (1<<COM3C1) | (0<<COM3C0) | (1<<WGM31) | (0<<WGM30);
	TCCR3B = (1<<WGM33) | (1<<WGM32) | (1<<CS30);
	ICR3   = 0x1000;
	OCR3C  = 0x0000;

	uint8_t i;
	
	while(1) {
		for (i=0; i<5; i++) {
			PORTB &= 0x0F;
			PORTB |= i << 4;
			_delay_ms(1);
		}
	}
}
