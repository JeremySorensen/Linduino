/*!
DC934A - LTC2607 and LTC2422:

@verbatim

This file imports the functions for the LTC2607 and LTC2422. It also adds a
calibration function which depends on functions for both parts.
The LTC2607 code is found at LTSketchbook\libraries\LTC2607\ltc2607_comm.(h/cpp)
The LTC2422 code is found at LTSketchbook\libraries\LTC2422\ltc2422_comm.(h/cpp)

@endverbatim

http://www.linear.com/product/LTC2422

http://www.linear.com/product/LTC2607
http://www.linear.com/product/LTC2609
http://www.linear.com/product/LTC2606

http://www.linear.com/product/LTC2607#demoboards
http://www.linear.com/product/LTC2609#demoboards
http://www.linear.com/product/LTC2606#demoboards


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

/*! @file
    @ingroup LTC2422
    Library for DC934A
*/

#include "dc934a_comm.h"

#define DC934A_CAL_LOW_DAC_CODE 0x00FF
#define DC934A_CAL_HIGH_DAC_CODE 0xFF00

#define DC934A_ERR_ADC_READ -9

static void cal_one_channel(
    Ltc2422State* ltc2422_state,
    uint32_t low_code,
    uint32_t high_code,
    float* offset,
    float* lsb)
{
    float low_volts = ltc2422_code_to_volts(ltc2422_state, low_code);
    float high_volts = ltc2422_code_to_volts(ltc2422_state, high_code);   

    *lsb = (high_volts - low_volts) / (high_code - low_code);
    *offset = low_volts - (*lsb) * low_code;   
}

int8_t dc934a_calibration(Ltc2607State* ltc2607_state, Ltc2422State* ltc2422_state)
{
    int8_t result = ltc2607_write_and_update_dac(ltc2607_state, LTC2607_BOTH, DC934A_CAL_LOW_DAC_CODE);
    if (result != LTC2607_ERR_OK) {
        return result;
    }
    
    uint32_t low_code_a, low_code_b;
    result = ltc2422_adc_read(ltc2422_state, &low_code_a, low_code_b);
    if (result != 0) {
        return DC934A_ERR_ADC_READ;
    }
    
    result = ltc2607_write_and_update_dac(ltc2607_state, LTC2607_BOTH, DC934A_CAL_HIGH_DAC_CODE);
    if (result != LTC2607_ERR_OK) {
        return result;
    }
    
    uint32_t high_code_a, high_code_b;
    result = ltc2422_adc_read(ltc2422_state, &high_code_a, high_code_b);
    if (result != 0) {
        return DC934A_ERR_ADC_READ;
    }
    
    cal_one_channel(
        ltc2422_state,
        low_code_a,
        high_code_a,
        &(ltc2607_state->dac_a_lsb),
        &(ltc2607_state->dac_a_offset));
    
    cal_one_channel(
        ltc2422_state,
        low_code_b,
        high_code_b,
        &(ltc2607_state->dac_b_lsb),
        &(ltc2607_state->dac_b_offset));
        
    ltc2607_state->same_cal = false;
    
    return LTC2607_ERR_OK;
}