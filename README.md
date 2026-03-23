# 3-I2C
Simple C firmware for the ATmega128, providing functions to initialize, set, and read time from a DS3231 RTC over the I2C bus.
Printing to terminal via UART implemented with library by Tomas Kolousek.

Supports setting and reading time. - Dynamic time initialization via terminal WIP.

Default port for I2C - PORTB (PB0 - SCL, PB1 - SDA)

Terminal setting - 9600 BAUD, 8 data bits, no parity, 1 stop bit
