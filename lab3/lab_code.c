#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>

#include "_functions.c"

uint16_t COUNT = 0;
uint8_t SEGS[5] = {0,0,0xFF,0,0};

//*******************************************************************************
//                            debounce_switch                                  
// Adapted from Ganssel's "Guide to Debouncing"            
// Checks the state of pushbutton S0 It shifts in ones till the button is pushed. 
// Function returns a 1 only once per debounced button push so a debounce and toggle 
// function can be implemented at the same time.  Expects active low pushbutton on 
// Port D bit zero.  Debounce time is determined by external loop delay times 12. 
//*******************************************************************************
int8_t debounce_switch(uint8_t pin) {
	static uint16_t state[8] = {0,0,0,0,0,0,0,0}; //holds present states
	state[pin] = (state[pin] << 1) | (! bit_is_clear(PINA, 7-pin)) | 0xE000;	//count 12 "presses"
	if (state[pin] == 0xF000) return 1;
	return 0;
}

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

int main()
{
	uint8_t i;
	uint8_t mode = 0x00;
	uint8_t data, dir, tempmode;	

	DDRB = 0xF7;  	//set port B to all outputs (except PB3 - MISO)
	PORTB= 0x78;	//set select bits off, PWM low, pull up resistor on MISO

	DDRD = 0xFF;
	DDRE = 0xFF;

	SPI_init();

	while(1){     //do forever

		split_count();			//populate SEGS array for 7seg output
		DDRA = 0xFF;			//set PORTA to all outputs
		for (i = 0; i < 5; i++) {	//Loop through each 7seg digit
			PORTA = SEGS[i];
			PORTB &= (i << PB4) & 0x70;
			_delay_ms(1);
			PORTB |= (1<<PB6) | (1<<PB5) | (1<<PB4);
		}

		DDRA = 0x00;			//set PORTA to all inputs
		PORTA= 0xFF;			//set all pull up resistors on PORTA
		PORTB &= (5 << 4) & 0x70;	//set select bits to take input from pushbuttons
		asm("nop");

		for (i = 0; i < 8; i++) {	//take input with debouncing
			if (debounce_switch(i))
				mode ^= 1 << i;
		}

		PORTB |= (1<<PB6) | (1<<PB5) | (1<<PB4);	//turn off select bits

		data = spi_send_read(mode);
		dir = find_direction(data, 0);

		tempmode = mode & 0x03;

		if (dir == CW) {
			if (tempmode == 0x00)
				COUNT++;
			else if (tempmode == 0x01)
				COUNT += 2;
			else if (tempmode == 0x02)
				COUNT += 4;
		}
		if (dir == CCW) {
			if (tempmode == 0x00)
				COUNT--;
			else if (tempmode == 0x01)
				COUNT -= 2;
			else if (tempmode == 0x02)
				COUNT -= 4;
		}
		if (COUNT > 1023)				//check for overflow
			COUNT -= 1023;
		else if (COUNT < 0)				//check for underflow
			COUNT += 1023;

	} //while 
} //main
