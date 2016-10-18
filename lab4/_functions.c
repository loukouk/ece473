#define CW 1
#define CCW 2

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
int8_t debounce_PORTC(uint8_t pin) {
	static uint16_t state[8] = {0,0,0,0,0,0,0,0}; //holds present states
	state[pin] = (state[pin] << 1) | (! bit_is_clear(PINC, pin)) | 0xE000;	//count 12 "presses"
	if (state[pin] == 0xF000) return 1;
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

void SPI_init()
{
	SPCR = (1<<SPE) | (1<<MSTR);	//SPI enabled, master, low polarity, MSB 1st
	SPSR = (1<<SPI2X);		//run at I/O clock/2
}

uint8_t spi_send_read(uint8_t data)
{
	PORTE |= 0x40;				//port E bit 6, assert active high
	SPDR = data;                    	//send data byte
	while (bit_is_clear(SPSR,SPIF)) {}	//wait till data is sent out
	PORTD &= 0xFB;                  	//port D bit 2, assert active low
	PORTD |= 0x04;				//port D bit 2, deassert to logic high
	PORTE &= 0xBF;				//port E bit 6, deassert to logic low

	return SPDR;				//return the data read from the encoders
}

uint8_t find_direction (uint8_t encoder_state, uint8_t i)
{
	static uint8_t prev_a[2] = {-1,-1};		//holds last state of b
	static uint8_t prev_b[2] = {-1,-1};		//holds last state of b
	uint8_t a = encoder_state & 1;			//current a
	uint8_t b = (encoder_state>>1) & 1;		//current b
	uint8_t direction = 0;				//holds resulting direction
	
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
	
	//set prev variables for next pass
	prev_a[i] = a;
	prev_b[i] = b;
	
	return direction;
}
