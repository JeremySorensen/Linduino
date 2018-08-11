/*
ParseNum.h

Routines to parse numbers stored as strings.

Copyright 2018(c) Analog Devices, Inc.

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
 - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
 - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in
   the documentation and/or other materials provided with the
   distribution.
 - Neither the name of Analog Devices, Inc. nor the names of its
   contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.
 - The use of this software may or may not infringe the patent rights
   of one or more patent holders.  This license does not release you
   from the requirement that you obtain separate licenses from these
   patent holders to use this software.
 - Use of the software either in source or binary form, must be run
   on or directly connected to an Analog Devices Inc. component.

THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <Arduino.h>
#include <limits.h>
#include "ParseNum.h"
#include "SerialInterface.hpp"

static bool parse_long(const char* str, int32_t* value, int32_t min, int32_t max) {
    uint8_t len = strlen(str);
    char* endptr;
    int32_t long_value = strtol(str, &endptr, 0);
    uint8_t num_read = endptr - str;

    if (num_read == 0 || num_read != len) {
        sinter_error(F("bad_arg"), F("Expected integer, got "), str);
        return false;
    } else if (long_value < min || long_value > max) {
        sinter_error(F("bad_arg"), F("Value out of range: "), long_value);
        return false;
    }
    *value = long_value;
    return true;
}

static bool parse_ulong(const char* str, uint32_t* value, int32_t max) {
    uint8_t len = strlen(str);
    char* endptr;
    uint32_t long_value = strtoul(str, &endptr, 0);
    uint8_t num_read = endptr - str;

    if (num_read == 0 || num_read != len) {
        sinter_error(F("bad_arg"), F("Expected integer, got "), str);
        return false;
    } else if (long_value < 0 || long_value > max) {
        sinter_error(F("bad_arg"), F("Value out of range: "), long_value);
        return false;
    }
    *value = long_value;
    return true;
}

bool parse_i32(const char* str, uint32_t* value)
{
    return parse_long(str, value, LONG_MIN, LONG_MAX);
}

bool parse_uint32(const char* str, uint32_t* value)
{
    return parse_ulong(str, value, ULONG_MAX);
}

bool parse_i16(const char* str, int16_t* value)
{
    int32_t long_value;
    bool result = parse_long(str, &long_value, INT_MIN, INT_MAX);
    *value = int16_t(value);
    return result;
}

bool parse_u16(const char* str, int16_t* value)
{
    int32_t long_value;
    bool result = parse_ulong(str, &long_value, UINT_MAX);
    *value = uint16_t(long_value);
    return result;
}

bool parse_i8(const char* str, int8_t* value)
{
    int32_t long_value;
    bool result = parse_long(str, &long_value, SCHAR_MIN, SCHAR_MAX);
    *value = int8_t(long_value);
    return result;
}

bool parse_u8(const char* str, uint8_t* value)
{
    int32_t long_value;
    bool result = parse_ulong(str, &long_value, UCHAR_MAX);
    *value = uint8_t(long_value);
    return result;
}

bool parse_f32(const char* str, float* value)
{
    uint8_t len = strlen(str);
    char* endptr;
    *value = strtod(str, &endptr);
    uint8_t num_read = endptr - str;

    if (num_read == 0 || num_read != len) {
        sinter_error(F("bad_arg"), F("Expected float value, got "), str);
        return false;
    } else if (!isfinite(*value)) {
        sinter_error(F("bad_arg"), F("Value out of range: "), *value);
        return false;
    }
    return true;
}
