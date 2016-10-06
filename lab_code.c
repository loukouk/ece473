#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>

uint16_t COUNT = 0;
uint8_t SEGS[5];

//*******************************************************************************
//                            debounce_switch                                  
// Adapted from Ganssel's "Guide to Debouncing"            
// Checks the state of pushbutton S0 It shifts in ones till the button is pushed. 
// Function returns a 1 only once per debounced button push so a debounce and toggle 
// function can be implemented at the same time.  Expects active low pushbutton on 
// Port D bit zero.  Debounce time is determined by external loop delay times 12. 
//*******************************************************************************
int8_t debounce_switch(uint8_t pin) {
	static uint16_t state[8] = {0,0,0,0,0,0,0,0}; //holds present state
	state[pin] = (state[pin] << 1) | (! bit_is_clear(PINA, pin)) | 0xE000;
	if (state[pin] == 0xF000) return 1;
	return 0;
}

uint8_t decode_digit(int8_t digit)
{
	switch(digit){
		case -1:	return 0b11111111;
		case 0: 	return 0b11000000;
		case 1: 	return 0b11111001;
		case 2: 	return 0b10100100;
		case 3: 	return 0b10110000;
		case 4: 	return 0b10011001;
		case 5: 	return 0b10010010;
		case 6: 	return 0b10000010;
		case 7: 	return 0b11111000;
		case 8: 	return 0b10000000;
		case 9:		return 0b10011000;
		default:	return 0b10111111;
	}
}

void split_count ()
{
	uint16_t count = COUNT;
	uint8_t segs[5];

	segs[4] = count/1000;
	count -= segs[4] * 1000;
	segs[3] = count/100;
	count -= segs[3] * 100;
	segs[1] = count/10;
	count -= segs[1] * 10;
	segs[0] = count;

	SEGS[0] = decode_digit(segs[0]);
	SEGS[1] = decode_digit(segs[1]);
	SEGS[3] = decode_digit(segs[3]);
	SEGS[4] = decode_digit(segs[4]);

	if (segs[0] % 2)
		SEGS[2] = 0xFC;
	else
		SEGS[2] = 0xFF;
}

int main()
{
	uint8_t i;

	DDRB = 0xFF;  //set port B to all outputs
	PORTB= 0x00;


	while(1){     //do forever

		split_count();
		DDRA = 0xFF;	//set PORTA to all outputs
		for (i = 0; i < 5; i++) {
			PORTB &= ~(i << 4);
			PORTA = SEGS[i];
			_delay_ms(2);
			PORTB |= (1<<PB6) | (1<<PB5) | (1<<PB4);
		}

		DDRA = 0x00;	//set PORTA to all inputs
		PORTA= 0xFF;	//set all pull up resistors on PORTA
		PORTB &= ~(5 << 4);
		_delay_ms(1);

		for (i = 0; i < 8; i++) {
			if (debounce_switch(i))
				COUNT += 1 << i;
		}
		PORTB |= (1<<PB6) | (1<<PB5) | (1<<PB4);

	} //while 
} //main
