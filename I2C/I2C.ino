#define F_CPU 14745600UL
#include <avr/io.h>
// #include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdbool.h>
#include "src/usart.h"
#include "src/BitOps.h"

#define DDR  DDRF
#define PORT PORTF
#define PIN  PINF
#define SDA  1
#define SCL  0
#define addr_w 0xD0
#define addr_r 0xD1

const char day_ch[7][3] = {{'P','o','\0'}, {'U','t','\0'}, {'S','t','\0'}, {'C','t','\0'}, {'P','a','\0'}, {'S','o','\0'}, {'N','e','\0'}};

typedef struct 
{
  uint8_t hour;
  uint8_t min;
  uint8_t sec;
  uint8_t day;
  uint8_t date;
  uint8_t month;
  uint16_t year;
} Time;

void Setup(void)
{
  DDR  = (1 << SCL) | (1 << SDA);
  PORT = (1 << SCL) | (1 << SDA);
  usart_setup(9600, bits8, parityNone, stop1);
}

void Clk(void)
{
  PORT = Nastavit(PORT, SCL);  // H
  _delay_us(5);
  PORT = Vynulovat(PORT, SCL); // L
  _delay_us(5);
}

uint8_t ToDEC(uint8_t value) // Convert value to DEC
{
  return ((value & 0xF0) >> 4) * 10 + (value & 0x0F);
}

uint8_t ToBCD(uint8_t value) // Convert value to BCD
{
  return (((value / 10 ) << 4) + (value % 10));
}

void StartBit(void) // First SDA LOW then SCL LOW
{
  DDR  = Nastavit(DDR, SDA);   // Write SDA

  PORT = Vynulovat(PORT, SDA); // SDA L
  _delay_us(5);
  PORT = Vynulovat(PORT, SCL); // SCL L
  _delay_us(5);
}

void RepeatedStart(void)
{
  DDR = Nastavit(DDR, SDA);

  PORT = Nastavit(PORT, SDA);
  _delay_us(5);
  PORT = Nastavit(PORT, SCL);
  _delay_us(5);

  StartBit();
}

void StopBit(void) // SDA must be LOW, then SCL HIGH then SDA HIGH
{
  DDR  = Nastavit(DDR, SDA);   // Write SDA

  PORT = Vynulovat(PORT, SDA); // SDA L
  _delay_us(5);

  PORT = Nastavit(PORT, SCL);  // SCL H
  _delay_us(5);
  PORT = Nastavit(PORT, SDA);   // SDA H
  _delay_us(5);
}

void WriteByte(uint8_t byte) // Sending a byte - MSB bit first
{
  DDR = Nastavit(DDR, SDA);       // Write SDA

  for (int8_t i = 7; i >= 0; i--)
  {
    if (JeNastaven(byte, i))
      PORT = Nastavit(PORT, SDA); // SDA H
    else
      PORT = Vynulovat(PORT,SDA); // SDA L
    _delay_us(5);
    
    Clk(); 
  }
  ReadBit();
}

uint8_t ReadByte(bool ack)
{
  uint8_t data = 0x00;
  DDR = Vynulovat(DDR, SDA);    // Read SDA

  for (int8_t i = 7; i >= 0; i--)
  {
    if (ReadBit() == true)
      data = Nastavit(data, i); // Write 1 if bit == 1
  }

  DDR = Nastavit(DDR, SDA);     // Write SDA
  WriteBit(ack);                // Send ACK
  return data;
}

bool ReadBit(void)
{
  DDR = Vynulovat(DDR, SDA);   // Read SDA
  _delay_us(5);
  PORT = Nastavit(PORT, SCL);  // SCL H 
  bool bit = JeNastaven(PIN, SDA); // Read PIN SDA
  _delay_us(5);
  PORT = Vynulovat(PORT, SCL); // SCL L
  _delay_us(5);
  return bit;
}

void WriteBit(bool bit)
{
  PORT = (bit == true) ? Nastavit(PORT, SDA) : Vynulovat(PORT, SDA); // Send bit HIGH if bit == true else LOW
  _delay_us(5);
  Clk();
}

void WriteReg(uint8_t reg, uint8_t value)
{
  StartBit();
  WriteByte(addr_w); 
  WriteByte(reg);
  WriteByte(value);
  StopBit();
}

uint8_t ReadReg(uint8_t reg)
{
  uint8_t data = 0;
  StartBit();
  WriteByte(addr_w);
  WriteByte(reg);

  RepeatedStart();
  WriteByte(addr_r);
  data = ReadByte(true);
  StopBit();
  return data;
}

void SetTime(uint8_t hour, uint8_t min, uint8_t sec, uint8_t day, uint8_t date, uint8_t month, uint16_t year)
{
  uint8_t month_bcd = ToBCD(month);
  uint8_t year_short = year % 2000;
  if (year / 100 == 21)
  	Nastavit(month_bcd, 7);
  
  WriteReg(0x00, ToBCD(sec)); // Seconds
  WriteReg(0x01, ToBCD(min)); // Minutes
  WriteReg(0x02, Vynulovat(ToBCD(hour), 6)); // Hours
  WriteReg(0x03, ToBCD(day)); // Day
  WriteReg(0x04, ToBCD(date)); // Date
  WriteReg(0x05, month_bcd); // Month
  WriteReg(0x06, ToBCD(year_short)); // Year
}

Time ReadTime(void)
{
  Time time;
  time.sec  = ToDEC(ReadReg(0x00));
  time.min  = ToDEC(ReadReg(0x01));
  time.hour = ToDEC(ReadReg(0x02));
  time.day  = ToDEC(ReadReg(0x03));
  time.date = ToDEC(ReadReg(0x04));
  uint8_t month_bcd   = ReadReg(0x05);
  uint8_t year_short  = ReadReg(0x06);


  time.month = ToDEC(month_bcd & 0x1F);
  time.year = 2000 + ToDEC(year_short);

  if (JeNastaven(month_bcd, 7) == true)
	time.year += 100;

  return time; 
}

int main(void)
{
  Setup();
  Time time;
  // Set values in RTC registers
  SetTime(8,0,0,2,17,3,2026);

  while(1)
  {
  // Read values from RTC registers and wait
  time = ReadTime();
  printf("%02d:%02d:%02d | %s | %02d.%02d.%04d\r\n", time.hour, time.min, time.sec, day_ch[time.day - 1], time.date, time.month, time.year);
  _delay_ms(1000);
  }
  return 0;
}