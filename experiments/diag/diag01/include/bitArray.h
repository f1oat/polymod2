// Inspired by https://github.com/RobTillaart/Arduino/tree/master/libraries/BitArray

#pragma once
#include "Arduino.h"

class bitArray
{
public:
    bitArray(const uint8_t size);
    ~bitArray();

    uint8_t capacity() { return _size; };
    bool get(const uint8_t idx);
    void set(const uint8_t idx, bool value);
    void toggle(const uint8_t idx);
    void clear();

private:
    static const uint8_t MSIZE = 4;
    static const uint8_t BSIZE = 8 * MSIZE;

    uint8_t   _size = 0;
    union {
        uint8_t staticArray[MSIZE];     // to HEAP save memory, we do not use malloc if the size is too small
        uint8_t *dynamicArray;          // We will allocate memory if size > MSIZE bytes
    } storage;
};