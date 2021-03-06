#ifndef UART_FUNCTIONS_H_ECE473
#define UART_FUNCTIONS_H_ECE473

//UART Functions header file 
//Roger Traylor 11.l6.11
//For controlling the UART and sending debug data to a terminal
//as an aid in debugging.

void uart_putc(char data);
void uart_puts(char *str);
void uart_puts_p(const char *str);
void uart_init();
uint8_t uart_getc(char *);

#endif
