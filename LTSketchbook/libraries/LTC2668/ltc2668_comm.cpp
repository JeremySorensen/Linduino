/*!
LTC2668: 16-Channel SPI 16-/12-Bit +/-10V Vout SoftSpan DACs with 10ppm/C Max Reference

http://www.linear.com/product/LTC2668

http://www.linear.com/product/LTC2668#demoboards

REVISION HISTORY
$Revision: 4430 $
$Date: 2015-11-30 11:42:19 -0800 (Mon, 30 Nov 2015) $

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
    @ingroup LTC2668
    Library for LTC2668 16-Channel SPI 16-/12-Bit +/-10V Vout SoftSpan DACs with 10ppm/C Max Reference
*/

#include <stdint.h>
#include <math.h>
#include <Arduino.h>
#include "Linduino.h"
#include "LT_SPI.h"
#include "ltc2668_comm.h"
#include <SPI.h>

#define FAKE

void ltc2668_init(Ltc2668State* state)
{
    for (int i = 0; i < LTC2668_NUM_CHANNELS; ++i) {
        state->soft_spans[i] = LTC2668_SPAN_0_TO_5V;
    }
    state->mux_selected_dac = 0;
    state->select_bits = 0;
    state->all_same_span = 1;
    state->global_toggle = 0;
    
    // First turn everything off, set span, and set to 0 and enable
    // ignore any errors for now
    
    ltc2668_power_down_dac(state, LTC2668_ALL_DACS);
    ltc2668_set_softspan(state, LTC2668_ALL_DACS, LTC2668_SPAN_0_TO_5V);
    ltc2668_write_and_update_dac(state, LTC2668_ALL_DACS, 0);
}

static int8_t ltc2668_check_and_copy_transaction(Ltc2668State* state, uint8_t rx[])
{
    int8_t result = LTC2668_ERR_OK;
    for (int i = 0; i < LTC2668_COMMAND_WORD_SIZE; ++i) {
        if (state->previous_data[i] != rx[i]) {
            result = LTC2668_ERR_MISMATCH;
        }
        state->previous_data[i] = rx[i];
    }
    return result;
}

static int8_t ltc2668_write(Ltc2668State* state, uint8_t dac_command, uint8_t dac_address, uint16_t dac_code)
// Write the 16-bit dac_code to the LTC2668
{
    uint8_t data[LTC2668_COMMAND_WORD_SIZE];
    uint8_t rx[LTC2668_COMMAND_WORD_SIZE];
  
    data[0] = dac_code & 0xFF;
    data[1] = (dac_code >> 8) & 0xFF;
    data[2] = dac_command | dac_address;
    data[3] = 0;

#ifdef FAKE
    for (int i = 0; i < LTC2668_COMMAND_WORD_SIZE; ++i) {
        rx[i] = state->previous_data[i];
    }
#else
    spi_transfer_block(QUIKEVEL_CS, data, rx, LTC2668_COMMAND_WORD_SIZE);
#endif
  
  return ltc2668_check_and_copy_transaction(state, rx);
}

static int8_t ltc2668_span_to_min_max(Ltc2668State* state, int8_t selected_dac, float* min, float* max)
{
    uint8_t span;
    if (selected_dac == LTC2668_ALL_DACS) {
        if (!state->all_same_span) {
            return LTC2668_ERR_NOT_SAME_SPAN;
        } else {
            span = state->soft_spans[0];
        }
    } else {
        span = state->soft_spans[selected_dac];
    }
    
    switch (span) {
        case LTC2668_SPAN_0_TO_5V:
            *min = 0.0;
            *max = 5.0;
            return 0;
        case LTC2668_SPAN_0_TO_10V:
            *min = 0.0;
            *max = 10.0;
            return 0;
        case LTC2668_SPAN_PLUS_MINUS_5V:
            *min = -5.0;
            *max = 0.5;
            return 0;
        case LTC2668_SPAN_PLUS_MINUS_10V:
            *min = -10;
            *max = 10;
            return 0;
        case LTC2668_SPAN_PLUS_MINUS_2V5:
            *min = -2.5;
            *max = 2.5;
            return 0;
        default:
            return LTC2668_ERR_BAD_SPAN;
    }
}

int8_t ltc2668_volts_to_code(
    const Ltc2668State* state,
    int8_t selected_dac,
    float dac_voltage,
    uint16_t* code)
{
    float min, max;
    int8_t result = ltc2668_span_to_min_max(state, selected_dac, &min, &max);
    if (result != LTC2668_ERR_OK) {
        return result;
    }
    
    float float_code = floor(LTC2668_FULL_SCALE * (dac_voltage - min) / (max - min));
    
    if (float_code < 0.0) {
        *code = 0;
    } else if (float_code > LTC2668_FULL_SCALE) {
        *code = LTC2668_FULL_SCALE;
    } else {
        *code = uint16_t(float_code);
    }
    return 0;
}

int8_t ltc2668_code_to_volts(
    const Ltc2668State* state,
    int8_t selected_dac,
    uint16_t dac_code, 
    float* volts)
{
    float min, max;
    int8_t result = ltc2668_span_to_min_max(state, selected_dac, &min, &max);
    if (result != LTC2668_ERR_OK) {
        return result;
    }
    
    *volts = (float(dac_code) / LTC2668_FULL_SCALE) * (max - min) + min;
    return 0;
}

int8_t ltc2668_set_reference_mode(const Ltc2668State* state, uint8_t is_internal)
{
    return ltc2668_write(
        state,
        LTC2668_CMD_CONFIG,
        0,
        is_internal ? LTC2668_REF_ENABLE : LTC2668_REF_DISABLE);
}

int8_t ltc2668_write_dac_input_register(
    const Ltc2668State* state,
    int8_t selected_dac,
    uint16_t code)
{
    return ltc2668_write(
        state,
        selected_dac == LTC2668_ALL_DACS ? LTC2668_CMD_WRITE_ALL : LTC2668_CMD_WRITE_N,
        selected_dac,
        code);
}

int8_t ltc2668_write_and_update_dac(const Ltc2668State* state, int8_t selected_dac, uint16_t code)
{
    return ltc2668_write(
        state,
        selected_dac == LTC2668_ALL_DACS ? LTC2668_CMD_WRITE_ALL_UPDATE_ALL : LTC2668_CMD_WRITE_N_UPDATE_N,
        selected_dac,
        code);
}

int8_t ltc2668_update_power_up_dac(const Ltc2668State* state, int8_t selected_dac)
{
    return ltc2668_write(
        state,
        selected_dac == LTC2668_ALL_DACS ? LTC2668_CMD_UPDATE_ALL : LTC2668_CMD_UPDATE_N,
        selected_dac,
        0);
}
    
int8_t ltc2668_power_down_dac(const Ltc2668State* state, int8_t selected_dac)
{
    return ltc2668_write(
        state,
        selected_dac == LTC2668_ALL_DACS ? LTC2668_CMD_POWER_DOWN_ALL : LTC2668_CMD_POWER_DOWN_N,
        selected_dac,
        0);
}

int8_t ltc2668_set_softspan(Ltc2668State* state, int8_t selected_dac, uint8_t soft_span)
{    
    // keep track of the span of each channel and whether they are all the same
    if (selected_dac == LTC2668_ALL_DACS) {       
        state->all_same_span = true;
        for (int i = 0; i < LTC2668_NUM_CHANNELS; ++i)
        {
            state->soft_spans[i] = soft_span;
        }
    } else {       
        state->soft_spans[selected_dac] = soft_span;
        for (int i = 0; i < LTC2668_NUM_CHANNELS; ++i)
        {
            if (state->soft_spans[i] != soft_span)
            {
                state->all_same_span = false;
                break;
            }
        }
    }
    
    // actually set the span
    return ltc2668_write(
        state,
        selected_dac == LTC2668_ALL_DACS ? LTC2668_CMD_SPAN_ALL : LTC2668_CMD_SPAN,
        selected_dac,
        0);
}
    
int8_t ltc2668_toggle_select(Ltc2668State* state, uint16_t select_bits)
{
    return ltc2668_write(
        state,
        LTC2668_CMD_TOGGLE_SEL,
        0,
        select_bits);
    
}
    
int8_t ltc2668_set_mux(Ltc2668State* state, uint8_t is_enabled, int8_t selected_dac)
{
    state->mux_selected_dac = selected_dac;
    
    return ltc2668_write(
        state,
        LTC2668_CMD_MUX,
        0,
        selected_dac | (is_enabled ? LTC2668_MUX_ENABLE : LTC2668_MUX_DISABLE));
}
    
int8_t ltc2668_set_global_toggle(Ltc2668State* state, uint8_t is_high)
{
    state->global_toggle = is_high;
    return ltc2668_write(
        state,
        LTC2668_CMD_GLOBAL_TOGGLE,
        0,
        is_high ? 1 : 0);
}
    
int8_t ltc2668_ramp(Ltc2668State* state)
{
    for (int i = 0; i < LTC2668_NUM_CHANNELS; ++i) {
        ltc2668_write_and_update_dac(state, i, i * LTC2668_FULL_SCALE / LTC2668_NUM_CHANNELS);
    }
}