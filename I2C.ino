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

uint8_t hours   = 23;
uint8_t minutes = 30;
uint8_t seconds = 20;
uint8_t day     = 5;
uint8_t date    = 4;
uint8_t month   = 2;
uint16_t year   = 2040;

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
  return ToDEC(data);
}

uint8_t ToDEC(uint8_t value) // Convert value to DEC
{
  return ((value & 0xF0) >> 4) * 10 + (value & 0x0F);
}

uint8_t ToBCD(uint8_t value) // Convert value to BCD
{
  return (((value / 10 ) << 4) + (value % 10));
}

uint8_t SetTime(void)
{
  WriteReg(0x00, Seconds()); // Seconds
  WriteReg(0x01, Minutes()); // Minutes
  WriteReg(0x02, Hours()); // Hours
  WriteReg(0x03, Day()); // Day
  WriteReg(0x04, Date()); // Date
  WriteReg(0x05, Month()); // Month
  WriteReg(0x06, Year()); // Year
}

uint8_t ReadTime(void)
{
  printf("%d:", ReadReg(0x02)); // Hours
  printf("%d:", ReadReg(0x01)); // Minutes
  printf("%d ", ReadReg(0x00)); // Seconds
  printf("%d ", ReadReg(0x03)); // Day
  printf("%d.", ReadReg(0x04)); // Date
  printf("%d.", ReadReg(0x05)); // Month
  printf("%d" , ReadReg(0x06)); // Year
  printf("\n\r");
}

uint8_t Seconds()
{
  return ToBCD(seconds);
}

uint8_t Minutes()
{
  return ToBCD(minutes);
}

uint8_t Hours()
{
  uint8_t hours_local = ToBCD(hours);
  hours_local = Vynulovat(hours_local, 6);
  return hours_local;
}

uint8_t Day()
{
  uint8_t day_local = 0;
  if (day > 7)
    day_local = 7;
  else if (day < 1)
    day_local = 1;
  else
  day_local = day;
  return ToBCD(day_local);
}

uint8_t Date()
{
  uint8_t date_local = 0;
  if (date > 31)
    date_local = 31;
  else if (date < 1)
    date_local = 1;
  else
    date_local = date;
  return ToBCD(date_local);
}

uint8_t Month()
{
  uint8_t month_local = 0;
  if (month > 12)
	  month_local = 12;
  else if (month < 1)
	  month_local = 1;
  else
	  month_local = month;
  month_local = ToBCD(month_local);
  
  if (year / 100 == 21)
	  month_local = Nastavit(month_local, 7);
  else 
	  month_local = Vynulovat(month_local, 7);
  return month_local;
}

uint8_t Year()
{
  uint8_t year_local = year;
  return ToBCD(year_local);
}

int main(void)
{
  uint8_t data = 0;
  Setup();

  // Zapsání hodnoty do registru sekund
  SetTime();

  while(1)
  {
  // Čtení a vypsání každou vteřinu
  ReadTime();
  _delay_ms(1000);
  }
  return 0;
}