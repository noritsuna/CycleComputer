/* mbed TextLCD Library
 * Copyright (c) 2007-2009 sford
 * Released under the MIT License: http://mbed.org/license/mit
 *
 * TODO: Needs serious rework/neatening up!
 */

/*
 *  2010/05/14 modified for 20X4 LCD by ym1784
 */

#include "TextLCD_20X4.h"

#include "mbed.h"
//#include "error.h"

using namespace mbed;

/*
 * useful info found at http://www.a-netz.de/lcd.en.php
 *
 *
 * Initialization
 * ==============
 *
 * After attaching the supply voltage/after a reset, the display needs to be brought in to a defined state
 *
 * - wait approximately 15 ms so the display is ready to execute commands
 * - Execute the command 0x30 ("Display Settings") three times (wait 1,64ms after each command, the busy flag cannot be queried now).
 * - The display is in 8 bit mode, so if you have only connected 4 data pins you should only transmit the higher nibble of each command.
 * - If you want to use the 4 bit mode, now you can execute the command to switch over to this mode now.
 * - Execute the "clear display" command
 *
 * Timing
 * ======
 *
 * Nearly all commands transmitted to the display need 40us for execution.
 * Exceptions are the commands "Clear Display and Reset" and "Set Cursor to Start Position"
 * These commands need 1.64ms for execution. These timings are valid for all displays working with an
 * internal clock of 250kHz. But I do not know any displays that use other frequencies. Any time you
 * can use the busy flag to test if the display is ready to accept the next command.
 *
 * _e is kept high apart from calling clock
 * _rw is kept 0 (write) apart from actions that uyse it differently
 *  -> on this program, the _rw is not used ... ym1784
 * _rs is set by the data/command writes
 */

TextLCD_20X4::TextLCD_20X4(PinName rs, PinName e, PinName d0, PinName d1,
    PinName d2, PinName d3, int columns, int rows) : _rs(rs), _e(e), _d(d0, d1, d2, d3),
    _columns(columns), _rows(rows) {

    _rows = 4;
    _columns = 20;
    // Mon, 27 Apr 2009 23:32:34 +0200
    // Kevin Konradt:
    // When using a LCD with 1 row x 16 characters
    // instead of 2x16, try changing _columns to 8.
    // (display seems to split the 16 characters into 
    // 2 virtual rows with 8 characters each.)
    //
    // 2010/05/14 ym1784
    // This program is only for 4 rows x 20 characters specific

//  _rw = 0; // on this program, the _rw is not used ... ym1784
    _e  = 1;
    _rs = 0; // command mode

    // Should theoretically wait 15ms, but most things will be powered up pre-reset
    // so i'll disable that for the minute. If implemented, could wait 15ms post reset
    // instead
    wait(0.015); // for safety ... ym1784
        
    // send "Display Settings" 3 times (Only top nibble of 0x30 as we've got 4-bit bus)
    for(int i=0; i<3; i++) {
        writeNibble(0x3);
        wait(0.00164);  // this command takes 1.64ms, so wait for it
    }
    writeNibble(0x2);   // 4-bit mode

    writeCommand(0x28); // Function set 001 BW N F - -
    writeCommand(0x0C);
    writeCommand(0x6);  // Cursor Direction and Display Shift : 0000 01 CD S (CD 0-left, 1-right S(hift) 0-no, 1-yes

    cls();
}

int TextLCD_20X4::_putc(int value) {
    if(value == '\n') {
        newline();
    } else {
        writeData(value);
    }
    return value;
}

int TextLCD_20X4::_getc() {
    return 0;
}

void TextLCD_20X4::newline() {
    _column = 0;
    _row++;
    if(_row >= _rows) {
        _row = 0;
    }
    locate(_column, _row);
}

void TextLCD_20X4::locate(int column, int row) {
    if(column < 0 || column >= _columns || row < 0 || row >= _rows) {
        error("locate(%d,%d) out of range on %dx%d display", column, row, _columns, _rows);
        return;
    }
    _row = row;
    _column = column;
// modified for 20X4 LCD
    switch (_row) {
        case (0) : address = 0x80 + _column;
        break;
        case (1) : address = 0xc0 + _column;
        break;
        case (2) : address = 0x94 + _column;
        break;
        case (3) : address = 0xd4 + _column;
        break;
        } // switch
    writeCommand(address);
}

void TextLCD_20X4::cls() {
    writeCommand(0x01); // Clear Display
    wait(0.00164f);     // This command takes 1.64 ms
//    locate(0, 0);     // We don't have to do this here
}

void TextLCD_20X4::reset() {
    cls();
}

void TextLCD_20X4::clock() {
    wait(0.000040f);
    _e = 0;
    wait(0.000040f);  // most instructions take 40us
    _e = 1;
}

void TextLCD_20X4::writeNibble(int value) {
    _d = value;
    clock();
}

void TextLCD_20X4::writeByte(int value) {
    writeNibble(value >> 4);
    writeNibble(value >> 0);
}

void TextLCD_20X4::writeCommand(int command) {
    _rs = 0;
    writeByte(command);
}

void TextLCD_20X4::writeData(int data) {
    _rs = 1;
    writeByte(data);
    _column++;
    if(_column >= _columns) {
        newline();
    }
}
