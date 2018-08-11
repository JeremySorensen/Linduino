/*!
LTC2607: 16-Bit, Dual Rail-to-Rail DACs with I2C Interface.
LTC2609: Quad 16-/14-/12-Bit Rail-to-Rail DACs with I²C Interface.
LTC2606: 16-Bit Rail-to-Rail DACs with I²C Interface.

@verbatim

The LTC2607/LTC2617/LTC2627 are dual 16-, 14- and 12-bit, 2.7V to 5.5V
rail-to-rail voltage output DACs in a 12-lead DFN package. They have built-in
high performance output buffers and are guaranteed monotonic.

These parts establish new board-density benchmarks for 16- and 14-bit DACs and
advance performance standards for output drive and load regulation in single-
supply, voltage-output DACs.

The parts use a 2-wire, I2C compatible serial interface. The
LTC2607/LTC2617/LTC2627 operate in both the standard mode (clock rate of 100kHz)
and the fast mode (clock rate of 400kHz). An asynchronous DAC update pin (LDAC)
is also included.

The LTC2607/LTC2617/LTC2627 incorporate a power-on reset circuit. During power-
up, the voltage outputs rise less than 10mV above zero scale; and after power-
up, they stay at zero scale until a valid write and update take place. The
power-on reset circuit resets the LTC2607-1/LTC2617-1/ LTC2627-1 to mid-scale.
The voltage outputs stay at midscale until a valid write and update takes place.

@endverbatim

http://www.linear.com/product/LTC2607
http://www.linear.com/product/LTC2609
http://www.linear.com/product/LTC2606

http://www.linear.com/product/LTC2607#demoboards
http://www.linear.com/product/LTC2609#demoboards
http://www.linear.com/product/LTC2606#demoboards

REVISION HISTORY
 $Revision: 4780 $
 $Date: 2016-03-14 13:58:55 -0700 (Mon, 14 Mar 2016) $

Copyright (c) 2013, Linear Technology Corp.(LTC)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of Linear Technology Corp.

The Linear Technology Linduino is not affiliated with the official Arduino team.
However, the Linduino is only possible because of the Arduino team's commitment
to the open-source community.  Please, visit http://www.arduino.cc and
http://store.arduino.cc , and consider a purchase that will help fund their
ongoing work.
*/

/*! @file
    @ingroup LTC2607
    Header File for LTC2607: 16-Bit, Dual Rail-to-Rail DACs with I2C Interface
*/

#include <Arduino.h>
#include <math.h>
#include <Wire.h>
#include "LT_I2C.h"
#include "ltc2607_comm.h"

#define FAKE

static int8_t ltc2607_write_dac(Ltc2607State* state, uint8_t command, uint8_t address, uint16_t code)
{
#ifdef FAKE
    return LTC2607_ERR_OK;
#else
    uint8_t nack = i2c_write_word_data(state->address, (command | address), code);
    return nack == 0 ? LTC22668_ERR_OK : LTC2268_ERR_NAK;
#endif
}

int8_t ltc2607_volts_to_code(Ltc2607State* state, int8_t selected_dac, float volts, uint16_t* code)
{
    float lsb;
    float offset;
    
    bool is_channel_a;
    if (selected_dac == LTC2607_BOTH) {
        if (!state->same_cal) {
            return LTC2607_ERR_NOT_SAME_CAL;
        }
        is_channel_a = true;
    } else {
        is_channel_a = selected_dac == LTC2607_DAC_A;
    }
    
    if (is_channel_a) {
        lsb = state->channel_a_lsb;
        offset = state->channel_a_offset;
    } else {
        lsb = state->channel_b_lsb;
        offset = state->channel_b_offset;
    }
    
    float float_code = floor((volts - offset) / lsb);
    
    if (float_code > LTC2607_FULL_SCALE) {
        *code = LTC2607_FULL_SCALE;
    } else if (float_code < 0) {
        *code = 0;
    } else {
        *code = uint16_t(float_code);
    }
    
    return LTC2607_ERR_OK;
}

int8_t ltc2607_code_to_volts(Ltc2607State* state, int8_t selected_dac, uint16_t code, float* volts)
{
    float lsb;
    float offset;
    bool is_channel_a;
    if (selected_dac == LTC2607_BOTH) {
        if (!state->same_cal) {
            return LTC2607_ERR_NOT_SAME_CAL;
        }
        is_channel_a = true;
    } else {
        is_channel_a = selected_dac == LTC2607_DAC_A;
    }
        
    if (is_channel_a) {
        lsb = state->channel_a_lsb;
        offset = state->channel_a_offset;
    } else {
        lsb = state->channel_b_lsb;
        offset = state->channel_b_offset;
    }
    *volts = lsb * code + offset;
    return LTC2607_ERR_OK;
}

void ltc2607_init(Ltc2607State* state)
{
    ltc2607_set_i2c_address(state, LTC2607_I2C_AD_LOW, LTC2607_I2C_AD_LOW, LTC2607_I2C_AD_LOW);
    ltc2607_clear_calibration(state);
}

int8_t ltc2607_set_i2c_address(Ltc2607State* state, uint8_t ad2, uint8_t ad1, uint8_t ad0)
{
    uint8_t i = ad2 * 9 + ad1 * 3 + ad0;
    state->address = ((i / 4 + 1) << 4) | (i % 4);
    return LTC2607_ERR_OK;
}

int8_t ltc2607_write_dac_input_register(const Ltc2607State* state, int8_t selected_dac, uint16_t code)
{
    quikeval_I2C_connect();
    return ltc2607_write_dac(state, LTC2607_WRITE_COMMAND, selected_dac, code);
}
    
int8_t ltc2607_write_and_update_dac(const Ltc2607State* state, int8_t selected_dac, uint16_t code)
{
    quikeval_I2C_connect();
    return ltc2607_write_dac(state, LTC2607_WRITE_UPDATE_COMMAND, selected_dac, code);
}

int8_t ltc2607_update_power_up_dac(const Ltc2607State* state, int8_t selected_dac)
{
    quikeval_I2C_connect();
    return ltc2607_write_dac(state, LTC2607_UPDATE_COMMAND, selected_dac, 0);
}
    
int8_t ltc2607_power_down_dac(const Ltc2607State* state, int8_t selected_dac)
{
    quikeval_I2C_connect();
    return ltc2607_write_dac(state, LTC2607_POWER_DOWN_COMMAND, selected_dac, 0);
}
  
int8_t ltc2607_clear_calibration(Ltc2607State* state)
{
    state->channel_a_lsb = LTC2607_TYPICAL_lsb;
    state->channel_a_offset = LTC2607_TYPICAL_OFFSET;

    state->channel_b_lsb = LTC2607_TYPICAL_lsb;
    state->channel_b_offset = LTC2607_TYPICAL_OFFSET;
    
    state->same_cal = true;
    
    return LTC2607_ERR_OK;
}
    
    

