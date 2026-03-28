/*
  LedControl.h - A library for controling Leds with a MAX7219/MAX7221
  Copyright (c) 2007 Eberhard Fahle
  Modified 2016 by Alvaro Cassinelli (reduce memory footprint - char not used, only numbers)

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef LedControl_h
#define LedControl_h

#include <Arduino.h>

/*
 * Segments to be switched on for characters and digits on
 * 7-Segment Displays

const static byte charTable[128] = {
    B01111110,B00110000,B01101101,B01111001,B00110011,B01011011,B01011111,B01110000,
    B01111111,B01111011,B01110111,B00011111,B00001101,B00111101,B01001111,B01000111,
    B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,
    B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,
    B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,
    B00000000,B00000000,B00000000,B00000000,B10000000,B00000001,B10000000,B00000000,
    B01111110,B00110000,B01101101,B01111001,B00110011,B01011011,B01011111,B01110000,
    B01111111,B01111011,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,
    B00000000,B01110111,B00011111,B00001101,B00111101,B01001111,B01000111,B00000000,
    B00110111,B00000000,B00000000,B00000000,B00001110,B00000000,B00000000,B00000000,
    B01100111,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,
    B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00001000,
    B00000000,B01110111,B00011111,B00001101,B00111101,B01001111,B01000111,B00000000,
    B00110111,B00000000,B00000000,B00000000,B00001110,B00000000,B00000000,B00000000,
    B01100111,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,
    B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000
};
 */

const static byte decimalTable[11] = { // from 0 to 9, plus the minus sign (last one)
    B01111110,B00110000,B01101101,B01111001,B00110011,B01011011,B01011111,B01110000,B01111111,B01111011,B00000001
};

class LedControl {
 private :
    /* The array for shifting the data to the devices */
    byte spidata[16];
    /* Send out a single command to the device */
    void spiTransfer(uint8_t addr, byte opcode, byte data);

    /* We keep track of the led-status for all 8 devices in this array */
    byte status[64];
    /* Data is shifted out of this pin*/
    uint8_t SPI_MOSI;
    /* The clock is signaled on this pin */
    uint8_t SPI_CLK;
    /* This one is driven LOW for chip selectzion */
    uint8_t SPI_CS;
    /* The maximum number of devices we use */
    uint8_t maxDevices;

 public:
    /*
     * Create a new controler
     * Params :
     * dataPin		pin on the Arduino where data gets shifted out
     * clockPin		pin for the clock
     * csPin		pin for selecting the device
     * numDevices	maximum number of devices that can be controled
     */
    LedControl(uint8_t dataPin, uint8_t clkPin, uint8_t csPin, uint8_t numDevices=1);

    /*
     * Gets the number of devices attached to this LedControl.
     * Returns :
     * uint8_t	the number of devices on this LedControl
     */
    uint8_t getDeviceCount();

    /*
     * Set the shutdown (power saving) mode for the device
     * Params :
     * addr	The address of the display to control
     * status	If true the device goes uint8_to power-down mode. Set to false
     *		for normal operation.
     */
    void shutdown(uint8_t addr, bool status);

    /*
     * Set the number of digits (or rows) to be displayed.
     * See datasheet for sideeffects of the scanlimit on the brightness
     * of the display.
     * Params :
     * addr	address of the display to control
     * limit	number of digits to be displayed (1..8)
     */
    void setScanLimit(uint8_t addr, uint8_t limit);

    /*
     * Set the brightness of the display.
     * Params:
     * addr		the address of the display to control
     * intensity	the brightness of the display. (0..15)
     */
    void setIntensity(uint8_t addr, uint8_t uint8_tensity);

    /*
     * Switch all Leds on the display off.
     * Params:
     * addr	address of the display to control
     */
    void clearDisplay(uint8_t addr);

    /*
     * Set the status of a single Led.
     * Params :
     * addr	address of the display
     * row	the row of the Led (0..7)
     * col	the column of the Led (0..7)
     * state	If true the led is switched on,
     *		if false it is switched off
     */
    void setLed(uint8_t addr, uint8_t row, uint8_t col, boolean state);

    /*
     * Set all 8 Led's in a row to a new state
     * Params:
     * addr	address of the display
     * row	row which is to be set (0..7)
     * value	each bit set to 1 will light up the
     *		corresponding Led.
     */
    void setRow(uint8_t addr, uint8_t row, byte value);

    /*
     * Set all 8 Led's in a column to a new state
     * Params:
     * addr	address of the display
     * col	column which is to be set (0..7)
     * value	each bit set to 1 will light up the
     *		corresponding Led.
     */
    void setColumn(uint8_t addr, uint8_t col, byte value);

    /*
     * Display a hexadecimal digit on a 7-Segment Display
     * Params:
     * addr	address of the display
     * digit	the position of the digit on the display (0..7)
     * value	the value to be displayed. (0x00..0x0F)
     * dp	sets the decimal point.
     */
    void setDigit(uint8_t addr, uint8_t digit, uint8_t value, boolean dp);

    /*
     * Display a character on a 7-Segment display.
     * There are only a few characters that make sense here :
     *	'0','1','2','3','4','5','6','7','8','9','0',
     *  'A','b','c','d','E','F','H','L','P',
     *  '.','-','_',' '
     * Params:
     * addr	address of the display
     * digit	the position of the character on the display (0..7)
     * value	the character to be displayed.
     * dp	sets the decimal point
     */
    //void setChar(uint8_t addr, uint8_t digit, char value, boolean dp);
};

#endif	//LedControl.h
