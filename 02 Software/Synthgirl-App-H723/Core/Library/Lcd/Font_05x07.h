#ifndef __FONT_05x07_H
#define __FONT_05x07_H

#include "main.h"

const uint8_t kFontWidth_05x07 = 5;
const uint8_t kFontHeight_05x07 = 7;
const uint8_t kFontSpacing_05x07 = 2;

const uint16_t kFontData_05x07[][7] = {
    {0b01110,  // 0   -->   00
     0b10001,
     0b10011,
     0b10101,
     0b11001,
     0b10001,
     0b01110},
    {0b00100,  // 1   -->   01
     0b01100,
     0b10100,
     0b00100,
     0b00100,
     0b00100,
     0b11111},
    {0b01110,  // 2   -->   02
     0b10001,
     0b00001,
     0b00010,
     0b00100,
     0b01000,
     0b11111},
    {0b01110,  // 3   -->   03
     0b10001,
     0b00001,
     0b00110,
     0b00001,
     0b10001,
     0b01110},
    {0b00001,  // 4   -->   04
     0b00011,
     0b00101,
     0b01001,
     0b11111,
     0b00001,
     0b00001},
    {0b11111,  // 5   -->   05
     0b10000,
     0b10000,
     0b11110,
     0b00001,
     0b10001,
     0b01110},
    {0b00010,  // 6   -->   06
     0b00100,
     0b01000,
     0b11110,
     0b10001,
     0b10001,
     0b01110},
    {0b11111,  // 7   -->   07
     0b00001,
     0b00010,
     0b00100,
     0b01000,
     0b10000,
     0b10000},
    {0b01110,  // 8   -->   08
     0b10001,
     0b10001,
     0b01110,
     0b10001,
     0b10001,
     0b01110},
    {0b01110,  // 9   -->   09
     0b10001,
     0b10001,
     0b01111,
     0b00010,
     0b00100,
     0b01000},
    {0b01110,  // A   -->   10
     0b10001,
     0b10001,
     0b11111,
     0b10001,
     0b10001,
     0b10001},
    {0b11110,  // B   -->   11
     0b10001,
     0b10001,
     0b11110,
     0b10001,
     0b10001,
     0b11110},
    {0b01110,  // C   -->   12
     0b10001,
     0b10000,
     0b10000,
     0b10000,
     0b10001,
     0b01110},
    {0b11110,  // D   -->   13
     0b10001,
     0b10001,
     0b10001,
     0b10001,
     0b10001,
     0b11110},
    {0b11111,  // E   -->   14
     0b10000,
     0b10000,
     0b11110,
     0b10000,
     0b10000,
     0b11111},
    {0b11111,  // F   -->   15
     0b10000,
     0b10000,
     0b11110,
     0b10000,
     0b10000,
     0b10000},
    {0b01110,  // G   -->   16
     0b10001,
     0b10000,
     0b10111,
     0b10001,
     0b10001,
     0b01111},
    {0b10001,  // H   -->   17
     0b10001,
     0b10001,
     0b11111,
     0b10001,
     0b10001,
     0b10001},
    {0b11111,  // I   -->   18
     0b00100,
     0b00100,
     0b00100,
     0b00100,
     0b00100,
     0b11111},
    {0b01111,  // J   -->   19
     0b00001,
     0b00001,
     0b00001,
     0b00001,
     0b10001,
     0b01110},
    {0b10001,  // K   -->   20
     0b10010,
     0b10100,
     0b11000,
     0b10100,
     0b10010,
     0b10001},
    {0b10000,  // L   -->   21
     0b10000,
     0b10000,
     0b10000,
     0b10000,
     0b10000,
     0b11111},
    {0b10001,  // M   -->   22
     0b11011,
     0b10101,
     0b10101,
     0b10001,
     0b10001,
     0b10001},
    {0b10001,  // N   -->   23
     0b10001,
     0b11001,
     0b10101,
     0b10011,
     0b10001,
     0b10001},
    {0b01110,  // O   -->   24
     0b10001,
     0b10001,
     0b10001,
     0b10001,
     0b10001,
     0b01110},
    {0b11110,  // P   -->   25
     0b10001,
     0b10001,
     0b11110,
     0b10000,
     0b10000,
     0b10000},
    {0b01110,  // Q   -->   26
     0b10001,
     0b10001,
     0b10001,
     0b10101,
     0b10010,
     0b01101},
    {0b11110,  // R   -->   27
     0b10001,
     0b10001,
     0b11110,
     0b10100,
     0b10010,
     0b10001},
    {0b01110,  // S   -->   28
     0b10001,
     0b10000,
     0b01110,
     0b00001,
     0b10001,
     0b01110},
    {0b11111,  // T   -->   29
     0b00100,
     0b00100,
     0b00100,
     0b00100,
     0b00100,
     0b00100},
    {0b10001,  // U   -->   30
     0b10001,
     0b10001,
     0b10001,
     0b10001,
     0b10001,
     0b01110},
    {0b10001,  // V   -->   31
     0b10001,
     0b10001,
     0b10001,
     0b10001,
     0b01010,
     0b00100},
    {0b10001,  // W   -->   32
     0b10001,
     0b10001,
     0b10101,
     0b10101,
     0b10101,
     0b01010},
    {0b10001,  // X   -->   33
     0b10001,
     0b01010,
     0b00100,
     0b01010,
     0b10001,
     0b10001},
    {0b10001,  // Y   -->   34
     0b10001,
     0b10001,
     0b01010,
     0b00100,
     0b00100,
     0b00100},
    {0b11111,  // Z   -->   35
     0b00001,
     0b00010,
     0b00100,
     0b01000,
     0b10000,
     0b11111},
    {0b00001,  // /   -->   36
     0b00001,
     0b00010,
     0b00100,
     0b01000,
     0b10000,
     0b10000},
    {0b00000,  // -   -->   37
     0b00000,
     0b00000,
     0b11111,
     0b00000,
     0b00000,
     0b00000},
    {0b00000,  // +   -->   38
     0b00100,
     0b00100,
     0b11111,
     0b00100,
     0b00100,
     0b00000},
    {0b10001,  // %   -->   39
     0b10001,
     0b00010,
     0b00100,
     0b01000,
     0b10001,
     0b10001},
    {0b00000,  // .   -->   40
     0b00000,
     0b00000,
     0b00000,
     0b00000,
     0b00100,
     0b00100},
    {0b00000,  // ,   -->   41
     0b00000,
     0b00000,
     0b00000,
     0b00100,
     0b00100,
     0b01000},
    {0b00000,  // :   -->   42
     0b00000,
     0b00100,
     0b00000,
     0b00000,
     0b00100,
     0b00000},
    {0b00010,  // <   -->   43
     0b00100,
     0b01000,
     0b10000,
     0b01000,
     0b00100,
     0b00010},
    {0b01000,  // >   -->   44
     0b00100,
     0b00010,
     0b00001,
     0b00010,
     0b00100,
     0b01000},
    {0b01110,  // [   -->   45
     0b01000,
     0b01000,
     0b01000,
     0b01000,
     0b01000,
     0b01110},
    {0b01110,  // ]   -->   46
     0b00010,
     0b00010,
     0b00010,
     0b00010,
     0b00010,
     0b01110},
    {0b00000,  // _   -->   47
     0b00000,
     0b00000,
     0b00000,
     0b00000,
     0b00000,
     0b11111},
    {0b00100,  // |   -->   48
     0b00100,
     0b00100,
     0b00100,
     0b00100,
     0b00100,
     0b00100},
    {0b01110,  // ?   -->   49
     0b10001,
     0b10001,
     0b00010,
     0b00100,
     0b00000,
     0b00100},
    {0b01010,  // #   -->   50
     0b11111,
     0b01010,
     0b01010,
     0b01010,
     0b11111,
     0b01010},
    {0b00100,  // !   -->   51
     0b00100,
     0b00100,
     0b00100,
     0b00000,
     0b00100,
     0b00100},
    {0b00000,  //     -->   52
     0b00000,
     0b00000,
     0b00000,
     0b00000,
     0b00000,
     0b00000},
};

#endif
