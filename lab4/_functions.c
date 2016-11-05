#include <avr/io.h>

#include "_functions.h"

void init_globals()
{
	time[0] 	= 0x00;
	time[1] 	= 0x00;
	time[2] 	= 0x00;

	alarm[0] 	= 0x00;
	alarm[1] 	= 0x00;
	alarm[2] 	= 0x00;

	snooze[0] 	= 0x00;
	snooze[1] 	= 0x00;
	snooze[2] 	= 0x00;

	SEGS[0] 	= 0xFF;
	SEGS[1] 	= 0xFF;
	SEGS[2] 	= 0xFF;
	SEGS[3] 	= 0xFF;
	SEGS[4] 	= 0xFF;

	mode 		= 0x00;
	encoder_mode	= 0x00;
	alarm_mode	= 0x00;
	lcd_mode	= 0x00;
}

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
	state[pin] = (state[pin] << 1) | (bit_is_set(PINA, pin)) | 0xE000;	//count 12 "presses"
	if (state[pin] == 0xF000) return 1;
	return 0;
}
int8_t debounce_spi_buttons(uint8_t enc_pin, uint8_t enc_data) {
	static uint16_t enc_state[2] = {0,0}; //holds present enc_states
	enc_state[enc_pin] = (enc_state[enc_pin] << 1) | (((enc_data >> enc_pin) & 0x01)) | 0xE000;	//count 12 "presses"i
	if (enc_state[enc_pin] == 0xF000) return 1;
	return 0;
}

uint8_t decode_digit(int8_t digit)
{
	switch(digit){
		case -1:	return 0b11111111;	// turn off digit
		case 0: 	return 0b11000000;	// print 0
		case 1: 	return 0b11111001;	// print 1
		case 2: 	return 0b10100100;	// print 2
		case 3: 	return 0b10110000;	// print 3
		case 4: 	return 0b10011001;	// print 4
		case 5: 	return 0b10010010;	// print 5
		case 6: 	return 0b10000010;	// print 6
		case 7: 	return 0b11111000;	// print 7
		case 8: 	return 0b10000000;	// print 8
		case 9:		return 0b10011000;	// print 9
		case 10: 	return 0b10001000;	// print A
		case 11: 	return 0b10000011;	// print b
		case 12: 	return 0b11000110;	// print C
		case 13: 	return 0b10100001;	// print d
		case 14: 	return 0b10000110;	// print E
		case 15:	return 0b10001110;	// print F
		default:	return 0b10111111;	// print dash when there is an unexpected value
	}
}

uint8_t spi_send_read(uint8_t data)
{
	PORTE &= 0xBF;				//port E bit 6, deassert to logic low
	PORTE |= 0x40;				//port E bit 6, assert active high
	SPDR = data;                    	//send data byte
	while (bit_is_clear(SPSR,SPIF)) {}	//wait till data is sent out
	PORTB &= 0xFE;                  	//port B bit 0, assert active low
	PORTB |= 0x01;				//port B bit 0, deassert to logic high

	return SPDR;				//return the data read from the encoders
}

uint8_t find_direction (uint8_t state, uint8_t i)
{
/*
	static uint8_t sw_table[] = {0,1,2,0,2,0,0,1,1,0,0,2,0,2,1,0};
	static uint8_t prev_state[2] = {0,0};
	uint8_t index = 0;

	index = (prev_state[i] << 2) | state;
	prev_state[i] = state;
	return sw_table[index];
*/
	static uint8_t prev_a[2] = {-1,-1};	//holds last state of b
	static uint8_t prev_b[2] = {-1,-1};	//holds last state of b
	uint8_t a = state & 1;			//current a
	uint8_t b = (state>>1) & 1;		//current b
	uint8_t direction = 0;			//holds resulting direction


	if (!(a && b))
		goto FIND_DIRECTION_EXIT;

	//Determine direction using A tracking method
	if (!(a == -1 || b == -1)) {
		if (prev_a[i] == a){
			if ((a == 1) && (prev_b[i] < b)) direction = CW;
			if ((a == 1) && (prev_b[i] > b)) direction = CCW;
			if ((a == 0) && (prev_b[i] < b)) direction = CCW;
			if ((a == 0) && (prev_b[i] > b)) direction = CW;
		}
		else {
			if ((prev_a[i] < a) && ((prev_b[i] | b) == 0)) direction = CW;
			if ((prev_a[i] < a) && ((prev_b[i] | b) == 1)) direction = CCW;
			if ((prev_a[i] > a) && ((prev_b[i] | b) == 0)) direction = CCW;
			if ((prev_a[i] > a) && ((prev_b[i] | b) == 1)) direction = CW;
		}
	}
	
FIND_DIRECTION_EXIT:
	//set prev variables for next pass
	prev_a[i] = a;
	prev_b[i] = b;

	return direction;
}

void read_pushbuttons ()
{
	uint8_t i, ports_data[2];

	ports_data[0] = PORTA;		//save PORTA data
	ports_data[1] = PORTB & 0x70;	//save PORTA data
	PORTA = 0xFF;			//set all pull up resistors on PORTA
	DDRA  = 0x00;			//set PORTA to all inputs
	PORTB |= 0x70;
	PORTB &= (5 << 4) & 0x70;	//set select bits to take input from pushbuttons
	asm("nop");
	asm("nop");

	for (i = 0; i < 4; i++) {	//take input with debouncing
		if (debounce_switch(i))
			mode ^= 1 << i;
	}
	for (i = 4 ; i < 8; i++) {
		if (bit_is_clear(PINA, i))
			mode |= 1 << i;
		else
			mode &= ~(1<<i);
	}

	asm("nop");
	PORTB&= 0x8F;
	PORTB|= ports_data[1];		//restore PORTB data
	DDRA  = 0xFF;			//set PORTA back to outputs
	PORTA = ports_data[0];		//restore PORTA data
}

void print_get_up ()
{
	static uint8_t display_count;

	if (display_count < 80) {
		SEGS[0] = 0b10000111;	//t
		SEGS[1] = 0b10000110;	//E
		SEGS[2] = 0b11111111;	//off
		SEGS[3] = 0b10010000;	//g
		SEGS[4] = 0xFF;		//off
	}
	else if (display_count < 128 || display_count >= 208) {
		SEGS[0] = 0b11111111;	//all off
		SEGS[1] = 0b11111111;
		SEGS[2] = 0b11111111;
		SEGS[3] = 0b11111111;
		SEGS[4] = 0b11111111;
	}
	else {
		SEGS[0] = 0b11111111;	//off
		SEGS[1] = 0b10001100;	//P
		SEGS[2] = 0b11111111;	//off
		SEGS[3] = 0b11000001;	//U
		SEGS[4] = 0b11111111;	//off
	}
	display_count++;
}

void decode_time(uint8_t hours, uint8_t minutes)
{
	uint8_t divider[2], i;

	if (IS_AM_PM && hours > 12) {
		hours -= 12;
		SEGS[2] &= ~(0x04);
	}
	else
		SEGS[2] |= 0x04;

	for (i = 0; i < 2; i++) {
		if (((encoder_mode >> i) & 0x01)) {
			divider[i] = 16;
			SEGS[0] &= ~(1<<PA7);
		}
		else {
			divider[i] = 10;
			SEGS[0] |= 1 << PA7;
		}
	}

	//breaks up time value into its separate digits
	SEGS[1] = minutes / divider[0];
	SEGS[0] = minutes % divider[0];

	SEGS[4] = hours / divider[1];
	SEGS[3] = hours % divider[1];

	//removes all leading zeroes for a cleaner output
	if (SEGS[4] == 0)
		SEGS[4] = -1;

	//decodes each digit into a value for the 7seg display
	SEGS[4] = decode_digit(SEGS[4]);
	SEGS[3] = decode_digit(SEGS[3]);
	SEGS[1] = decode_digit(SEGS[1]);
	SEGS[0] = decode_digit(SEGS[0]);

	for (i = 0; i < 2; i++) {
		if (((encoder_mode >> i) & 0x01)) {
			SEGS[i*3] &= ~(1<<PA7);
		}
		else {
			SEGS[i*3] |= 1 << PA7;
		}
	}

	//makes colon blink on for 1sec, off for 1sec
	if ((time[0] % 2) == 0)
		SEGS[2] &= ~(0x03);
	else
		SEGS[2] |= 0x03;
}
