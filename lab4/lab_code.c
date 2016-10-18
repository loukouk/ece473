#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "_functions.c"
//#include "hd44780.h"

volatile uint8_t time[3] = {0,0,0};
volatile uint8_t SEGS[5] = {0,0,0xFF,0,0};
volatile uint8_t mode = 0x00;
volatile uint8_t encoder_mode;

void split_count()
{
	uint8_t divider;

	if (encoder_mode) {
		divider = 16;
		SEGS[2] &= ~(1<<PA2);
	}
	else {
		divider = 10;
		SEGS[2] |= 1<<PA2;
	}

	//breaks up time value into its separate digits
	SEGS[4] = time[2] / divider;
	SEGS[3] = time[2] % divider;

	SEGS[1] = time[1] / divider;
	SEGS[0] = time[1] % divider;

	SEGS[2] |= (1<<PA2);

	//removes all leading zeroes for a cleaner output
	if (SEGS[4] == 0)
		SEGS[4] = -1;
//	if (SEGS[1] == 0)
//		SEGS[1] = -1;

	//decodes each digit into a value for the 7seg display
	SEGS[4] = decode_digit(SEGS[4]);
	SEGS[3] = decode_digit(SEGS[3]);
	SEGS[1] = decode_digit(SEGS[1]);
	SEGS[0] = decode_digit(SEGS[0]);

	if (time[0] % 2)
		SEGS[2] &= ~(0x03);
	else
		SEGS[2] |= 0x03;
}

void check_time_overflow()
{
	if (time[1] >= 60) {
		time[1] -= 60;
		time[2]++;
		if (time[2] >= 24)
			time[2] -= 24;
	}
}

ISR(TIMER0_OVF_vect)
{
	time[0]++;
	if (time[0] >= 60) {
		time[0] -= 60;
		time[1]++;
		check_time_overflow();
	}
}

ISR(TIMER2_OVF_vect) {
	uint8_t i;
	uint8_t dir[2], tempmode;	
	uint8_t ports_data[2];
	static int8_t data, encoder_count = 0;

	if (debounce_PORTC(6) || debounce_PORTC(7)) {
		encoder_mode ^= 1;
	}

	ports_data[0] = PORTA;		//save PORTA data
	ports_data[1] = PORTB & 0x70;	//save PORTA data
	PORTA = 0xFF;			//set all pull up resistors on PORTA
	DDRA  = 0x00;			//set PORTA to all inputs
	PORTB |= 0x70;
	PORTB &= (5 << 4) & 0x70;	//set select bits to take input from pushbuttons
	asm("nop");

	for (i = 0; i < 8; i++) {	//take input with debouncing
		if (debounce_switch(i)) {
			mode ^= 1 << i;
		}
	}

	asm("nop");
	PORTB&= 0x8F;
	PORTB|= ports_data[1];		//restore PORTB data
	DDRA  = 0xFF;			//set PORTA back to outputs
	PORTA = ports_data[0];		//restore PORTA data

	tempmode = mode & 0x03;		//only use first 2 pushbuttons for now
	data = spi_send_read(tempmode);	//display their mode on bar graph + get input from encoders

	//for each encoder, determine which direction it is being turned
	for (i = 0; i < 2; i++) {
		dir[i] = find_direction(data >> (i*2), i);

		//increment count if encoders are being turned clockwise
		if (dir[i] == CW) {
			if (tempmode == 0x00)
				encoder_count += 1;
			else if (tempmode == 0x01)
				encoder_count += 2;
			else if (tempmode == 0x02)
				encoder_count += 4;
		}
		//decrement count if encoders are being turned counter clockwise
		if (dir[i] == CCW) {
			if (tempmode == 0x00)
				encoder_count -= 1;
			else if (tempmode == 0x01)
				encoder_count -= 2;
			else if (tempmode == 0x02)
				encoder_count -= 4;
		}
	}
	while (encoder_count >= 4) {
		encoder_count -= 4;
		time[i+1]++;
	}
	while (encoder_count <= -4) {
		encoder_count += 4;
		time[i+1]--;
	}
	check_time_overflow();
}



int main()
{
	uint8_t i;

	DDRA = 0xFF;	//set PORTA to all outputs
	PORTA= 0xff;	//set all LEDs off at init

	DDRB = 0xF7;  	//set port B to all outputs (except PB3 - MISO)
	PORTB= 0x78;	//set select bits off, PWM low, pull up resistor on MISO

	DDRC = 0x3F;	//set pin 6 and 7 to inputs
	PORTC= 0xFF;	//with pull up resistors

	DDRD = 0xfF;	//set PORTD to all outputs

	DDRE = 0xFF;	//set PORTE to all outputs

	SPI_init();

	//TIMER 0 SETUP
	ASSR  |= (1<<AS0);			//external clock 32,768Hz
	TIMSK |= (1<<TOIE0);			//enable overflow interrupt
	TCCR0 |= (1<<CS02) | (1<<CS00);		//normal mode, prescale by 128

	//TIMER 2 SETUP
	TIMSK |= (1<<TOIE2);			//enable overflow interrupt
	TCCR2 |= (1<<CS21) | (1<<CS20);		//normal mode, prescale by 256

	sei();

	while(1){     //do forever

		split_count();			//populate SEGS array for 7seg output
		for (i = 0; i < 5; i++) {	//Loop through each 7seg digit
			PORTA = SEGS[i];
			PORTB &= (i << PB4) & 0x70;
			_delay_ms(1);
			PORTB |= (1<<PB6) | (1<<PB5) | (1<<PB4);
		}
	} //while 
} //main
