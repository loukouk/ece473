#ifndef _FUNCTIONS_H_ECE473
#define _FUNCTIONS_H_ECE473

#define IS_ALARM_TRIGGER (alarm_mode&0x01)
#define SET_ALARM_TRIGGER (alarm_mode|=0x01)
#define CLEAR_ALARM_TRIGGER (alarm_mode&=0xFE)
#define IS_ALARM_SNOOZE (alarm_mode&0x02)
#define SET_ALARM_SNOOZE (alarm_mode|=0x02)
#define CLEAR_ALARM_SNOOZE (alarm_mode&=0xFD)

#define IS_SHOW_ALARM (mode&0x01)
#define IS_SETTING ((mode>>1)&0x01)
#define IS_AM_PM ((mode>>2)&0x01)
#define IS_ALARM_ARM ((mode>>3)&0x01)
#define IS_SNOOZING (mode>>4)

#define LCD_ALARM_OFF 1
#define LCD_ALARM_ON 2
#define LCD_ALARM_TRIG 3

#define SNOOZE_MINS 1
#define SNOOZE_SECS 10

#define CW 1
#define CCW 2

void init_globals();
int8_t debounce_switch(uint8_t pin);
int8_t debounce_spi_buttons(uint8_t enc_pin, uint8_t enc_data);
uint8_t decode_digit(int8_t digit);
uint8_t spi_send_read(uint8_t data);
uint8_t find_direction (uint8_t state, uint8_t i);
void read_pushbuttons ();
void print_get_up ();
void decode_time(uint8_t hours, uint8_t minutes);

volatile int8_t time[3];
volatile int8_t alarm[3];
volatile int8_t snooze[3];
volatile uint8_t SEGS[5];
volatile uint8_t mode;
volatile uint8_t encoder_mode;
volatile uint8_t alarm_mode;
volatile uint8_t lcd_mode;

#endif
