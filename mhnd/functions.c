#include "functions.h"
#include "si4734.h"
/*
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
*/

/***********************************************************************/
//                              tcnt0_init                             
//Initalizes timer/counter0 (TCNT0). TCNT0 is running in async mode
//with external 32khz crystal.  Runs in normal mode with no prescaling.
//Interrupt occurs at overflow 0xFF.
//
//
/*********************************************************************/
void init_tcnt0(){
    ASSR    |= (1<<AS0);                            //external 32khz osc (TOSC)
    TIMSK   |= (1<<TOIE0);                          //enable interrupts
    TCCR0   |= (1<<CS02) |(0<<CS01) | (1<<CS00);    //normal mode, 128 prescale
}


//*******************************************************************************
//                            debounce_switch                                  
// Adapted from Ganssel's "Guide to Debouncing"            
// Checks the state of pushbutton S0 It shifts in ones till the button is pushed. 
// Function returns a 1 only once per debounced button push so a debounce and toggle 
// function can be implemented at the same time.  Expects active low pushbutton on 
// Port D bit zero.  Debounce time is determined by external loop delay times 12. 
//*******************************************************************************
uint16_t state[8] = {0};
uint8_t debounce_switch(uint8_t button) {
//    static uint16_t state[8] = {0};
    state[button] = (state[button] << 1) | (! bit_is_clear(PINA, button) ) | 0xE000;
    if (state[button] == 0xF000) return 1;
    return 0;
}

/*********************************************************************/
//                            spi_init                               
//Initializes the SPI port on the mega128. Does not do any further   
//external device specific initializations.                          
/*********************************************************************/
void spi_init(){
    DDRB  = DDRB | 0x07;            //Turn on SS, MOSI, SCLK pins
    SPCR |= (1<<SPE) | (1<<MSTR);   // enable spi, master mode, low polarity, MSB 1st, fosc/2
    SPSR |= (1<<SPI2X);             //run at double speed 
}

uint8_t spi_read_send(uint8_t data){
    PORTE |= (1<<PE6);                  //bit 6 high: enable encoders serial shift   
    SPDR = data;                        //send data byte
    while (bit_is_clear(SPSR,SPIF)) {}  //wait till data is sent out
    PORTE &= ~(1<<PE6);                  //bit 6 low: disable encoders serial shift   
    PORTB |= (1<<PD0);                  //shift storage to BarGraph
    PORTB &= ~(1<<PD0);
    return SPDR;
}

/*********************************************************************/
//                  check long presses
//
/*********************************************************************/
#define LEFT_DIGITS     MINUTES
#define RIGHT_DIGITS    HOURS
uint8_t pressed[8] = {0};
uint16_t station_preset[8] = {9090, 9090,9090,9090,9090,9090,9090,9090};
uint8_t stop_seconds[8] = {100};
extern uint8_t mode;
extern uint8_t CLOCK[3];
extern uint8_t ALARM[3];
extern uint8_t LED[5];
extern uint16_t current_fm_freq;
uint8_t check_long_presses(uint8_t pin){
        if(debounce_switch(pin)){ 
            state[pin] = 0xffff;
            pressed[pin] += 1;
            if(pressed[pin] == 1)
                stop_seconds[pin] = CLOCK[SECONDS];
            if(pressed[pin] == 0x1f/3){
                pressed[pin] = 0;
                stop_seconds[pin] = 100;
                return 1;
            }
        }  //if switch true for 12 passes, increment port B
        
        else if( ((stop_seconds[pin]+1)==CLOCK[SECONDS]) && pressed[pin])
        {
            current_fm_freq = station_preset[pin];
            LED[0] = LED_seg(mode, CLOCK[LEFT_DIGITS], ALARM[MINUTES], current_fm_freq, 0);
            LED[1] = LED_seg(mode, CLOCK[LEFT_DIGITS], ALARM[MINUTES], current_fm_freq, 1);
            LED[3] = LED_seg(mode, CLOCK[RIGHT_DIGITS], ALARM[HOURS], current_fm_freq, 3);
            LED[4] = LED_seg(mode, CLOCK[RIGHT_DIGITS], ALARM[HOURS], current_fm_freq, 4);
            pressed[pin] = 0;
        }
        
        return 0;
}

/*********************************************************************/
//                  check switch buttons                               
//
/*********************************************************************/
uint8_t check_buttons(uint8_t mode, uint8_t pin){
        uint8_t local_mode = mode;
        if(debounce_switch(pin)){ 
            local_mode ^= (1<<pin);
        }  //if switch true for 12 passes, increment port B
        return local_mode;
}


/*********************************************************************/
//                  integer to segments
//
/*********************************************************************/
uint8_t int_to_digit(uint8_t num){
    switch(num){
        case 0:
            return 0xc0; //0x11000000 
        case 1:
            return 0xf9; //0x11111001;
        case 2:
            return 0xa4; //0x10100100;
        case 3:
            return 0xb0; //0x10110000;
        case 4:
            return 0x99; //0x10011001;
        case 5:
            return 0x92; //0x10010010;
        case 6:
            return 0x82; //0x10000010;
        case 7:
            return 0xf8; //0x11111000;
        case 8:
            return 0x80; //0x10000000;
        case 9:
            return 0x90; //0x10010000;
        default:
            return 0xcf; //0x10111111;
    }
}


void init_adc(){
    ADMUX   = (0<<REFS1) | (1<<REFS0) | (1<<ADLAR) |(1<<MUX2)|(1<<MUX1)|(0<<MUX0); //AVCC refrence, Left justified. input: PF6
    ADCSRA  = (1<<ADEN) | (0<<ADSC) | (0<<ADFR) | (0<<ADIE) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0); //enable ADC, prescale by 128
}
uint8_t read_adc(){
    ADCSRA |= (1<<ADSC); //start conversion.
    while(bit_is_clear(ADCSRA, ADIF)){}
    ADCSRA |= (1<<ADIF);
    return ADCH;
}

void init_tcnt3(){
    TCCR3A |= (1<<COM3A1) | (0<<COM3A0)  | (1<<WGM31) | (0<<WGM30);
    TCCR3B |= (1<<WGM33) | (1<<WGM32) | (1<<CS30);
//    ETIMSK |= (TOIE3);
    ICR3   = 0x0fff;
    OCR3A  = 0x0fff/6; //Volume duty cycle.
}

