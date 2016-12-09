#include "radio.h"
#include "functions.h"
#include "LCDDriver.h"
#include "si4734.h"

extern char first_line[16];
extern uint16_t current_fm_freq;
extern uint16_t station_preset[8];
extern uint8_t si4734_tune_status_buf[8]; //buffer for holding tune_status data  



void radio_reset(){
    //hardware reset of Si4734
    PORTE &= ~(1<<PE5); //int2 initially low to sense TWI mode
    DDRE  |=  (1<<PE5); //turn on Port E bit 5 to drive it low
    PORTE |=  (1<<PE2); //hardware reset Si4734 
    _delay_us(200);     //hold for 200us, 100us by spec         
    PORTE &= ~(1<<PE2); //release reset 
    _delay_us(30);      //5us required because of my slow I2C translators I suspect
                        //Si code in "low" has 30us delay...no explaination
    DDRE  &= ~(1<<PE5); //now Port E bit 5 becomes input from the radio interrupt
}
void init_radio(){

    EICRB |= (1<<ISC70) | (1<<ISC71);
    EIMSK |= (1<<INT7);

    radio_reset();
}
void radio_signal(){
    fm_rsq_status();
    uint8_t rssi;
    rssi =  si4734_tune_status_buf[4];
#define SIGNAL_SIGN 0x04
    if(rssi < 14)
        sprintf(first_line, "radio       %c   ", SIGNAL_SIGN);
    else if(rssi < 24)
        sprintf(first_line, "radio       %c%c  ", SIGNAL_SIGN,0x03);
    else if(rssi < 32)
        sprintf(first_line, "radio       %c%c%c ", SIGNAL_SIGN,0x03,0x02);
    else
        sprintf(first_line, "radio       %c%c%c%c", SIGNAL_SIGN,0x03,0x02,0x01);
}
void station_presetting(){
    if(check_long_presses(0)){
        sprintf(first_line, "%3u.%d set for 0 ", current_fm_freq/100, (current_fm_freq%100)/10);
        LCD_MovCursorLn1();
        LCD_PutStr(first_line);
        station_preset[0] = current_fm_freq;
    }
    if(check_long_presses(1)){
        sprintf(first_line, "%3u.%d set for 1 ", current_fm_freq/100, (current_fm_freq%100)/10);
        LCD_MovCursorLn1();
        LCD_PutStr(first_line);
        station_preset[1] = current_fm_freq;
    }
    if(check_long_presses(2)){
        sprintf(first_line, "%3u.%d set for 2 ", current_fm_freq/100, (current_fm_freq%100)/10);
        LCD_MovCursorLn1();
        LCD_PutStr(first_line);
        station_preset[2] = current_fm_freq;
    }
    if(check_long_presses(4)){
        sprintf(first_line, "%3u.%d set for 4 ", current_fm_freq/100, (current_fm_freq%100)/10);
        LCD_MovCursorLn1();
        LCD_PutStr(first_line);
        station_preset[4] = current_fm_freq;
    }
    if(check_long_presses(5)){
        sprintf(first_line, "%3u.%d set for 5 ", current_fm_freq/100, (current_fm_freq%100)/10);
        LCD_MovCursorLn1();
        LCD_PutStr(first_line);
        station_preset[5] = current_fm_freq;
    }
    if(check_long_presses(6)){
        sprintf(first_line, "%3u.%d set for 6 ", current_fm_freq/100, (current_fm_freq%100)/10);
        LCD_MovCursorLn1();
        LCD_PutStr(first_line);
        station_preset[6] = current_fm_freq;
    }
    if(check_long_presses(7)){
        sprintf(first_line, "%3u.%d set for 7 ", current_fm_freq/100, (current_fm_freq%100)/10);
        LCD_MovCursorLn1();
        LCD_PutStr(first_line);
        station_preset[7] = current_fm_freq;
    }
}
