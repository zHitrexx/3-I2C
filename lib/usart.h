/* usart.h - header file rsr232.c serial line access library

  2025-10-15 Tomas Kolousek - updated, suport for m328p (Arduino Uno), C++ compatibility
  2018-03-18 Tomas Kolousek, updated, flexible setup() routine, support for multiple MCUs.
                   - sputchar() and sgetchar() stream functions interface corrected         
  2013-09-28 Tomas Kolousek, created, support ATmega128 usart#1 only in fixed configuration
*/

#pragma once
// if defined, allows usage of printf() and scanf() functions over serial line for the cost of bit larger code footprint
#define USART_STDIO
// set usart number on multi-usart MCUs. Leave empty when only single usart is available on target MCU
#ifndef USART_NO
  #define USART_NO 1
#endif

#include <avr/pgmspace.h>
#ifdef USART_STDIO
  #include <stdio.h>
  FILE usart_io;	   // for usart stdio replacement
#endif

#include <avr/io.h>

#ifdef __cplusplus 
  extern "C" {
#endif

// enums for serial line options
typedef enum { bits5 = 0, bits6, bits7, bits8, bits9 = 7} eUsartBits;
typedef enum { stop1 = 0, stop2 = 1} eUsartStopBits;
typedef enum { parityNone = 0, parityEven = 2, parityOdd = 3} eUsartParity;

// setup usart line speed and frame format. Please check F_CPU and the maximum baudrate error for higher speeds
void usart_setup(uint32_t baud, eUsartBits dataBits, eUsartParity parity, eUsartStopBits stopBits);

// @returns 0 if no new data byte is available on port, non-zero if new byte is present in usart data register
int usart_dataready(void);

// send character via serial line. waits for empty tx buffer before sending
// @param c       Character to be sent to the serial line
void usart_putchar(char c);

// @returns character from serial line, waits for new character before retrieval
char usart_getchar(void);

// send null terminated string located in data/program memory
void usart_puts(char *s);
void usart_puts_P(const char *s);

#ifdef USART_STDIO
int usart_sputchar(char c, FILE *stream);
int usart_sgetchar(FILE *stream);
#endif

// support macros for flexible register access
#ifndef _CONCAT3
  #define _CONCAT3(a,b,c) a ## b ## c 
  #define CONCAT3(a,b,c) _CONCAT3(a,b,c)  // expand macros passed as values of macro 
#endif

#define USART_REG(basename,suffix) CONCAT3(basename,USART_NO,suffix)
#define USART_BIT(bitname) (CONCAT3(bitname,USART_NO,))

//definition of USART control/data registers for cross MCU usability, USART_NO taken in account
#ifndef _UCSRA 
  #define _UCSRA USART_REG(UCSR,A)
  #define _UCSRB USART_REG(UCSR,B)
  #define _UCSRC USART_REG(UCSR,C)
  #define _UBRRL USART_REG(UBRR,L)
  #define _UBRRH USART_REG(UBRR,L)
  #define _UDR   USART_REG(UDR,)
#endif
#ifndef RXEN
  #define RXEN   USART_BIT(RXEN)
  #define TXEN   USART_BIT(TXEN)
  #define RXC    USART_BIT(RXC)
  #define UDRE   USART_BIT(UDRE)
#endif

#ifdef __cplusplus
  } // extern C wrapper
#endif
