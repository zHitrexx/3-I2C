/* usart.c - basic serial line access routines for ATmega128
   for PkDesign devel board, jumper JP4/JP5 must be set to 2-3 position!
   2019-01-31 Tomas Kolousek - improved readability, used generic bitnames instead of USART_BIT() macro
   2018-03-18 Tomas Kolousek - updated for ATmega16, universal register access and baud init
   2013-09-28 Tomas Kolousek - updated, generalization of rs232 routines
   2013-04-08 Tomas Kolousek - created as support of B3t Mit lecture, basic config & stdio streams support
*/

#include <stdint.h>
#include "usart.h"

// setup usart line format and speed
void usart_setup(uint32_t baud, eUsartBits dataBits, eUsartParity parity, eUsartStopBits stopBits)
{
  // configure usart hardware registers (see documentation topic USART Registers, page 188)
  uint16_t UBRR = F_CPU / (baud << 4) - 1;
  //uint16_t UBRR = (((F_CPU) + 8UL * (baud)) / (16UL * (baud)) -1UL);
  #ifdef URSEL
    #define URSEL_MASK (1<<URSEL)
  #else
    #define URSEL_MASK 0
  #endif

  _UCSRC &= ~(URSEL_MASK);   // mega8/16, selection of UBRRH/UCSRC reg access on same IO location
  _UBRRH = UBRR >> 8;   
  _UBRRL = UBRR & 0xff;

  _UCSRA = 0x00;                                      // baud x1, no MPCM 
  _UCSRB = (1<<RXEN) | (1<<TXEN) | (dataBits & 0x04);   // Enable Rx and TX, no interrupt
  _UCSRC = (URSEL_MASK) | (parity << 4) | (stopBits << 3) | ( (dataBits & 0x03)<<1); // async mode
  
  #ifdef USART_STDIO
    // stream config for usart IO
	   fdev_setup_stream( &usart_io, usart_sputchar, usart_sgetchar, _FDEV_SETUP_RW);  // setup IO stream ...
    //usart_io = FDEV_SETUP_STREAM( usart_sputchar, usart_sgetchar, _FDEV_SETUP_RW);  // setup IO stream ...
	stdin = &usart_io;   // ... and use this stream for standard input ...
	stdout = &usart_io;  //  ... and output
  #endif
}

// @returns 0 if no new data byte is available on port, non-zero if new byte is present in usart data register
int usart_dataready(void) 
{
  return (_UCSRA & (1<<RXC));
}

// send character via serial line. waits for empty tx buffer before sending
// @param c       Character to be sent to the serial line
void usart_putchar(char c) 
{
  while ( (~_UCSRA & (1<<UDRE))  );		// wait until Tx buffer is empty. Note the empty command (;) mark!
  _UDR = c;
}

// @returns character from serial line, waits for new character before retriaval
char usart_getchar(void) 
{
  while (!usart_dataready());		// wait until new byte is present on usart
  return(_UDR);
}

// send null terminated string located in data/program memory
void usart_puts(char *s) {
  while (*s != 0)
    usart_putchar(*s++);
}

void usart_puts_P(const char *s) {
  char c;
  while ( (c = pgm_read_byte(s++)) )
    usart_putchar(c); 
}

#ifdef USART_STDIO

// same as @see usart_putchar() 
int usart_sputchar(char c, FILE *stream) {
  usart_putchar(c);
  return 0;
}

// same as @see usart_getchar() 
int usart_sgetchar(FILE *stream) {
  while (!usart_dataready());  // wait for usart data
  return(usart_getchar());
}
#endif
