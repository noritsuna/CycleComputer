/* mbed TextLCD Library
 * Copyright (c) 2007-2009 sford
 * Released under the MIT License: http://mbed.org/license/mit
 *
 * TODO: Needs serious rework/neatening up!
 */

/*
 *  2010/05/14 modified for 20X4 LCD by ym1784
 */

#ifndef MBED_TEXTLCD_20X4_H
#define MBED_TEXTLCD_20X4_H

#include "Stream.h"
#include "DigitalOut.h"
#include "BusOut.h"

namespace mbed {

/* Class: TextLCD_20X4
 * A 20x4 Text LCD controller ... ym1784
 *
 * Allows you to print to a Text LCD screen, and locate/cls. Could be
 * turned in to a more generic libray.
 *
 * If you are connecting multiple displays, you can connect them all in
 * parallel except for the enable (e) pin, which must be unique for each
 * display.
 *
 * Example:
 * > #include "mbed.h"
 * > #include "TextLCD_20X4.h"
 * >
 * > TextLCD_20X4 lcd(p21, p22, p23, p24, p25, p26); // rs, e, d0, d1, d2, d3
 * >
 * > int main() {
 * >     lcd.printf("Hello World!");
 * > }
 */
class TextLCD_20X4 : public Stream {

public:
    /* Constructor: TextLCD_20X4
     * Create a TextLCD_20X4 object, connected to the specified pins
     *
     * All signals must be connected to DigitalIn compatible pins.
     *
     * Variables:
     *  rs -  Used to specify data or command
     *  e - enable
     *  d0..d3 - The data lines
     */
    TextLCD_20X4(PinName rs, PinName e, PinName d0, PinName d1,
        PinName d2, PinName d3, int columns = 20, int rows = 4);

#if 0 // Inhereted from Stream, for documentation only
    /* Function: putc
     *  Write a character
     *
     * Variables:
     *  c - The character to write to the serial port
     */
    int putc(int c);

    /* Function: printf
     *  Write a formated string
     *
     * Variables:
     *  format - A printf-style format string, followed by the
     *      variables to use in formating the string.
     */
    int printf(const char* format, ...);
#endif

    /* Function: locate
     * Locate to a certian position
     *
     * Variables:
     *  column - the column to locate to, from 0..19
     *  row - the row to locate to, from 0..3
     */
    virtual void locate(int column, int row);

    /* Function: cls
     * Clear the screen
     */
    virtual void cls();

    virtual void reset();

//protected:

    void clock();
    void writeData(int data);
    void writeCommand(int command);
    void writeByte(int value);
    void writeNibble(int value);
    virtual int _putc(int c);
    virtual int _getc();
    virtual void newline();

    int _row;
    int _column;
    DigitalOut _rs, _e;
    BusOut _d;
    int _columns;
    int _rows;
    int address;
};

}

#endif
