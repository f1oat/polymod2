#include <ArduinoSTL.h>
#include <stdexcept>

#include "bitArray.h"


bitArray::bitArray(const uint8_t size)
{
    _size = size;

    if (size >= BSIZE) {
        storage.dynamicArray = new uint8_t[(size+7)/8];
        if (!storage.dynamicArray) {
            Serial.println(F("bitArray malloc error"));
            _size = 0;
        }
    }

    clear();
}

bitArray::~bitArray()
{
    if (_size >= BSIZE) delete storage.dynamicArray;
    _size = 0;
}

bool bitArray::get(const uint8_t idx)
{
    if (idx >= _size) return false;
    if (_size >= BSIZE) return bitRead(storage.dynamicArray[idx/8], idx & 7);
    else return bitRead(storage.staticArray[idx/8], idx & 7);
}

void bitArray::set(const uint8_t idx, bool value)
{
    if (idx >= _size) return;
    if (_size >= BSIZE) bitWrite(storage.dynamicArray[idx/8], idx & 7, value);
    else bitWrite(storage.staticArray[idx/8], idx & 7, value);
}

void bitArray::toggle(const uint8_t idx)
{
    set(idx, !get(idx));
}

void bitArray::clear()
{
    if (_size >= BSIZE) memset(storage.dynamicArray, 0, (_size+7)/8);
    else memset(storage.staticArray, 0, (_size+7)/8);
}