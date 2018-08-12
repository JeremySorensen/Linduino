/*!
LTC2422: 1-/2-Channel 20-Bit uPower No Latency Delta-Sigma ADC in MSOP-10

@verbatim

The LTC2421/LTC2422 are 1- and 2-channel 2.7V to 5.5V micropower 20-bit analog-
to-digital converters with an integrated oscillator, 8ppm INL and 1.2ppm RMS
noise. These ultrasmall devices use delta-sigma technology and a new digital
filter architecture that settles in a single cycle. This eliminates the latency
found in conventional delta-sigma converters and simplifies multiplexed
applications. Through a single pin, the LTC2421/LTC2422 can be configured for
better than 110dB rejection at 50Hz or 60Hz +/-2%, or can be driven by an
external oscillator for a user defined rejection frequency in the range 1Hz to
120Hz. The internal oscillator requires no external frequency setting
components.

@endverbatim

http://www.linear.com/product/LTC2422

http://www.linear.com/product/LTC2422#demoboard


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

//! @ingroup Analog_to_Digital_Converters
//! @{
//! @defgroup LTC2422 LTC2422: 1-/2-Channel 20-Bit uPower No Latency Delta-Sigma ADC in MSOP-10
//! @}

/*! @file
    @ingroup LTC2422
    Library for LLTC2422: 1-/2-Channel 20-Bit uPower No Latency Delta-Sigma ADC in MSOP-10
*/

#include <stdint.h>
#include <Arduino.h>
#include "Linduino.h"
#include "LT_I2C.h"
#include "LT_SPI.h"
#include "ltc2422_comm.h"
#include <SPI.h>

static int8_t ltc2422_EOC_timeout()
{
    output_low(QUIKEVAL_CS);
    for (uint16_t timer_count = 0; timer_count < MISO_TIMEOUT; ++timer_count) {
        if (input(MISO) == 0) {
            return -1;
        }
    }
    return 0;
}

int8_t ltc2422_adc_read(const Ltc2422State* state, int32_t *code_a, int32_t *code_b)
{
    int8_t result = ltc2422_EOC_timeout();
    if (result != 0) { return result; }
    
    int8_t command[] = { 0, 0, 0, 0 };
    int8_t data[] = { 0, 0, 0, 0};
    spi_transfer_block(QUIKEVAL_CS, command, data, 3);
    bool is_channel_a = (data[2] & 0x40) == 0;
    
    int32_t code;
    memcpy(&code, data, sizeof(uint32_t));
    
    spi_transfer_block(QUIKEVAL_CS, command, data, sizeof(uint32_t));
    
    if (is_channel_a) {
        *code_a = code;
        memcpy(code_b, data, sizeof(int32_t));
    } else {
        *code_b = code;
        memcpy(code_a, data, sizeof(int32_t));
    }
    
    return 0;
}

// Calculates the voltage given the ADC code and lsb weight.
float ltc2422_code_to_volts(const Ltc2422State* state, int32_t adc_code)
{
  return (adc_code - 0x00200000) * state->lsb;
}

// Calculates the lsb weight from the given reference voltage.
void ltc2422_set_reference(Ltc2422State* state, float ref_volts)
{
  state->lsb = ref_volts / 1048575.0f;
}


