#define CW 1
#define CCW 2


uint8_t spi_send_read(uint8_t data)
{
	PORTD &= 0xFB;                      //port D bit 2, assert active low
	PORTE |= 0x40;						//port E bit 6, assert active high
	SPDR = data;                        //send data byte
	while (bit_is_clear(SPSR,SPIF)) {}  //wait till data is sent out
	PORTD |= 0x04;                      //port D bit 2, deassert to logic high
	PORTE &= 0xBF;						//port E bit 6, deassert to logic low

	return SPDR;						//return the data read from the encoders
}

uint8_t find_direct (uint8_t encoder_state, uint8_t i)
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
