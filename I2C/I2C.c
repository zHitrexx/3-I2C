#define F_CPU 14745600UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
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

void SetTime(Time time)
{
  time = CheckValues(time);

  uint8_t month_bcd = ToBCD(time.month);
  uint8_t year_short = time.year % 2000;
  if (time.year / 100 == 21) // Set century bit high if 21. century
  	Nastavit(month_bcd, 7);
  
  WriteReg(0x00, ToBCD(time.sec)); // Seconds
  WriteReg(0x01, ToBCD(time.min)); // Minutes
  WriteReg(0x02, Vynulovat(ToBCD(time.hour), 6)); // Hours in 24h format (6. bit low)
  WriteReg(0x03, ToBCD(time.day));  // Day
  WriteReg(0x04, ToBCD(time.date)); // Date
  WriteReg(0x05, month_bcd);   // Month
  WriteReg(0x06, ToBCD(year_short)); // Year
}

Time ReadTime(void) // Vracet struct + nastavovat pomocí struct
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

Time ProcessUSART(char *string, bool *is_set) // Processing the command via USART
{
  Time time;
  char *hour  = strtok(string, ":"); // Seperating the parameters 
  char *min   = strtok(NULL, ":");
  char *sec   = strtok(NULL, ",");
  char *day   = strtok(NULL, ",");
  char *date  = strtok(NULL, ".");
  char *month = strtok(NULL, ".");
  char *year  = strtok(NULL, "\0");

  if (strcmp(hour, "HELP") == 0)
  {
    if (min == NULL)
      printf("HELP:format | HELP:range\r\n");
    else if (strcmp(min, "format") == 0)
      printf("hours:minutes:seconds,day,date.month.year\r\n");
    else if (strcmp(min, "range") == 0)
      printf("[0-23]:[0-59]:[0-59],[1-7],[1-31].[1-12].[2000-2199]\r\n");
	  else
      printf("HELP:format | HELP:range\r\n");
    return;
  }

  if (hour == NULL || min == NULL || sec == NULL || day == NULL || date == NULL || month == NULL || year == NULL)
  {
    printf("Missing parameter! Try HELP :)\r\n");
    return;
  }
	
  time.sec   = atoi(sec);  // Conversion from string to number
  time.min   = atoi(min);
  time.hour  = atoi(hour);
  time.day   = atoi(day);
  time.date  = atoi(date);
  time.month = atoi(month);
  time.year  = atoi(year);

  *is_set = true;
  return CheckValues(time);
}

Time CheckValues(Time time)
{
  if (time.hour > 23)
    time.hour = 0;
  if (time.min > 59)
    time.min = 0;
  if (time.sec > 59)
    time.sec = 0;
  if (time.day > 7 || time.day == 0)
    time.day = 1;
  if (time.date > 31 || time.date == 0)
    time.date = 1;
  if (time.month > 12 || time.month == 0)
    time.month = 1;
  if (time.year > 2199 || time.year < 2000)
    time.year = 2000;

  if (time.date > 29 && time.month == 2)
    time.date = 1;
  if (time.date > 30 && ((time.month < 8 && time.month % 2 == 0) || (time.month > 7 && time.month % 2 != 0)))
    time.date = 1;

  return time;
}

int main(void)
{
  Time time;
  char znak;
  char string[23];
  uint8_t index = 0;
  bool is_set = false;

  Setup();

  printf("Zadejte cas (HELP)!\r\n");

  while(is_set == false)
  {
    if (usart_dataready())
	  {
	    znak = usart_getchar();
      	printf("%c", znak); // Vymazat a nastavit local line editing a local echo v terminal v putty
	    if (znak == '\r' || znak == '\n')
	    {
	      if (index == 0)
	        continue;
          string[index] = '\0';
          time = ProcessUSART(string, &is_set);
	      index = 0;
	    }
	    else
	    {
	      if (index < 23)
	      {
	        string[index] = znak;
	        index++;
	      }
	    }
	  }
  }

  // Set values in RTC registers
  SetTime(time);

  while(1)
  {
  // Read values from RTC registers and wait
  time = ReadTime();
  printf("%02d:%02d:%02d | %s | %02d.%02d.%04d\r\n", time.hour, time.min, time.sec, day_ch[time.day - 1], time.date, time.month, time.year);
  _delay_ms(1000);
  }
  return 0;
}
