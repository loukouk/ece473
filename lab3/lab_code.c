#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "_functions.c"

int16_t COUNT = 0;
uint8_t SEGS[5] = {0,0,0xFF,0,0};


void split_count ()
{
	uint16_t count = COUNT;		// local COUNT variable
	uint8_t segs[5];		// local SEGS variable

	//breaks up COUNT value into its separate digits
	segs[4] = count/1000;
	count -= segs[4] * 1000;
	segs[3] = count/100;
	count -= segs[3] * 100;
	segs[1] = count/10;
	count -= segs[1] * 10;
	segs[0] = count;

	//removes all leading zeroes for a cleaner output
	if (segs[4] == 0) {
		segs[4] = -1;
		if (segs[3] == 0) {
			segs[3] = -1;
			if (segs[1] == 0) {
				segs[1] = -1;
			}
		}
	}

	//decodes each digit into a value for the 7seg display
	SEGS[4] = decode_digit(segs[4]);
	SEGS[3] = decode_digit(segs[3]);
	SEGS[1] = decode_digit(segs[1]);
	SEGS[0] = decode_digit(segs[0]);
}

ISR(TIMER0_OVF_vect)
{
	uint8_t i;
	uint8_t mode = 0x00;
	uint8_t data, dir[2], tempmode;	
	uint8_t ports_data[2];

	ports_data[0] = PORTA;		//save PORTA data
	ports_data[1] = PORTB;		//save PORTA data
	PORTA= 0xFF;			//set all pull up resistors on PORTA
	DDRA = 0x00;			//set PORTA to all inputs
	PORTB |= 0x70;
	PORTB &= (5 << 4) & 0x70;	//set select bits to take input from pushbuttons
	asm("nop");

	for (i = 0; i < 8; i++) {	//take input with debouncing
		if (debounce_switch(i)) {
			mode |= 1 << i;
		}
		else {
			mode &= ~(1 << i);
		}
	}

	asm("nop");
	PORTB = ports_data[1];		//restore PORTB data
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
				COUNT++;
			else if (tempmode == 0x01)
				COUNT += 2;
			else if (tempmode == 0x02)
				COUNT += 4;
		}
		//decrement count if encoders are being turned counter clockwise
		if (dir[i] == CCW) {
			if (tempmode == 0x00)
				COUNT--;
			else if (tempmode == 0x01)
				COUNT -= 2;
			else if (tempmode == 0x02)
				COUNT -= 4;
		}
	}
	if (COUNT > 1023)		//check for overflow
		COUNT -= 1023;
	else if (COUNT < 0)		//check for underflow
		COUNT += 1024;
}

int main()
{
	uint8_t i;

	DDRA = 0xFF;	//set PORTA to all outputs
	PORTA= 0xff;	//set all LEDs off at init

	DDRB = 0xF7;  	//set port B to all outputs (except PB3 - MISO)
	PORTB= 0x78;	//set select bits off, PWM low, pull up resistor on MISO

	DDRD = 0xFF;	//set PORTD to all outputs
	DDRE = 0xFF;	//set PORTE to all outputs

	SPI_init();

	TIMSK |= (1<<TOIE0);				//enable interrupts
	TCCR0 |= (1<<CS02) | (1<<CS01) | (1<<CS00);	//normal mode, prescale by 1024

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
