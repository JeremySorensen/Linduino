/*!
LTC2668: 16-Channel SPI 16-/12-Bit +/-10V Vout SoftSpan DACs with 10ppm/C Max Reference

@verbatim

The LTC2668 is a family of 16-channel, 12/16-bit +/-10V digital-to-analog converters with integrated
precision references. They are guaranteed monotonic and have built-in rail-to-rail output buffers.
These SoftSpan DACs offer five output ranges up to +/-10V. The range of each channel is independently
programmable, or the part can be hardware-configured for operation in a fixed range.

The integrated 2.5V reference is buffered separately to each channel; an external reference can be used
for additional range options. The LTC2668 also includes toggle capability - alternating codes via the
software toggle command, or by providing a free-running clock to the TGP pin.

The SPI/Microwire-compatible 3-wire serial interface operates on logic levels as low as 1.71V, at clock
rates up to 50MHz.

@endverbatim

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

//! @defgroup LTC2668 LTC2668 16-Channel SPI 16-/12-Bit +/-10V Vout SoftSpan DACs with 10ppm/C Max Reference

/*! @file
    @ingroup LTC2668
    LTC2668 16-Channel SPI 16-/12-Bit +/-10V Vout SoftSpan DACs with 10ppm/C Max Reference
*/

#ifndef LTC2668_COMM_H
#define LTC2668_COMM_H

#include <stdint.h>

#define LTC2668_NUM_CHANNELS 16
#define LTC2668_FULL_SCALE 65535
#define LTC2668_COMMAND_WORD_SIZE 4
#define LTC2668_ALL_DACS -1


//! @name LTC2668 Command Codes
//! OR'd together with the DAC address to form the command byte
#define  LTC2668_CMD_WRITE_N              0x00  //!< Write to input register n
#define  LTC2668_CMD_UPDATE_N             0x10  //!< Update (power up) DAC register n
#define  LTC2668_CMD_WRITE_N_UPDATE_ALL   0x20  //!< Write to input register n, update (power-up) all
#define  LTC2668_CMD_WRITE_N_UPDATE_N     0x30  //!< Write to input register n, update (power-up) 
#define  LTC2668_CMD_POWER_DOWN_N         0x40  //!< Power down n
#define  LTC2668_CMD_POWER_DOWN_ALL       0x50  //!< Power down chip (all DAC's, MUX and reference)

#define  LTC2668_CMD_SPAN                 0x60  //!< Write span to dac n
#define  LTC2668_CMD_CONFIG               0x70  //!< Configure reference / toggle
#define  LTC2668_CMD_WRITE_ALL            0x80  //!< Write to all input registers
#define  LTC2668_CMD_UPDATE_ALL           0x90  //!< Update all DACs
#define  LTC2668_CMD_WRITE_ALL_UPDATE_ALL 0xA0  //!< Write to all input reg, update all DACs
#define  LTC2668_CMD_MUX                  0xB0  //!< Select MUX channel (controlled by 5 LSbs in data word)
#define  LTC2668_CMD_TOGGLE_SEL           0xC0  //!< Select which DACs can be toggled (via toggle pin or global toggle bit)
#define  LTC2668_CMD_GLOBAL_TOGGLE        0xD0  //!< Software toggle control via global toggle bit
#define  LTC2668_CMD_SPAN_ALL             0xE0  //!< Set span for all DACs
#define  LTC2668_CMD_NO_OPERATION         0xF0  //!< No operation
//! @}

//! @name LTC2668 Span Codes
//! @{
//! Descriptions are valid for a 2.5V reference.
//! These can also be interpreted as 0 to 2*Vref, 0 to 4*Vref, etc.
//! when an external reference other than 2.5V is used.
#define  LTC2668_SPAN_0_TO_5V             0x0000
#define  LTC2668_SPAN_0_TO_10V            0x0001
#define  LTC2668_SPAN_PLUS_MINUS_5V       0x0002
#define  LTC2668_SPAN_PLUS_MINUS_10V      0x0003
#define  LTC2668_SPAN_PLUS_MINUS_2V5      0x0004
//! @}

//! @name LTC2668 Minimums and Maximums for each Span
//! @{
//! Lookup tables for minimum and maximum outputs for a given span
const float LTC2668_MIN_OUTPUT[5] = {0.0, 0.0, -5.0, -10.0, -2.5};
const float LTC2668_MAX_OUTPUT[5] = {5.0, 10.0, 5.0, 10.0, 2.5};
//! @}

//! @name LTC2668 Configuration options
//! @{
//! Used in conjunction with LTC2668_CMD_CONFIG command
#define  LTC2668_REF_ENABLE               0x00  //! Enable internal reference
#define  LTC2668_REF_DISABLE              0x01  //! Disable internal reference to save power when using an ext. ref.
#define  LTC2668_THERMAL_SHUTDOWN_ENABLE  0x00  //! Enable thermal shutdoan
#define  LTC2668_THERMAL_SHUTDOWN_DISABLE 0x02  //! Disable thermal shutdown (NOT recommended)
//! @}

//! @name LTC2668 MUX enable
//! @{
//! Used in conjunction with LTC2668_CMD_MUX command
#define  LTC2668_MUX_DISABLE              0x0000  //! Disable MUX
#define  LTC2668_MUX_ENABLE               0x0010  //! Enable MUX, OR with MUX channel to be monitored
//! @}

//! @name LTC2668 Global Toggle
//! @{
//! Used in conjunction with LTC2668_CMD_GLOBAL_TOGGLE command, affects DACs whose
//! Toggle Select bits have been set to 1
#define  LTC2668_TOGGLE_REG_A              0x0000  //! Update DAC with register A
#define  LTC2668_TOGGLE_REG_B              0x0010  //! Update DAC with register B
//! @}

// Errors
//! @name LTC2668 Errors
//! @{
//! Errors that can be returned
#define LTC2668_ERR_OK             0 //!< No error
#define LTC2668_ERR_MISMATCH      -1 //!< Readback did not match previous command (device error)
#define LTC2668_ERR_NOT_SAME_SPAN -2 //!< Tried to do an operation that requires all DACs to have same span (invalid operation)
#define LTC2668_ERR_BAD_SPAN      -3 //!< Tried to look up a non-existant SPAN value (logic error)

typedef struct Ltc2668StateTag {
    uint8_t soft_spans[LTC2668_NUM_CHANNELS];
    int8_t mux_selected_dac;
    uint16_t select_bits;
    uint8_t all_same_span;
    uint8_t global_toggle;
    uint8_t previous_data[LTC2668_COMMAND_WORD_SIZE];
} Ltc2668State;

void ltc2668_init(Ltc2668State* state);

int8_t ltc2668_code_to_volts(
    const Ltc2668State* state,
    int8_t selected_dac,
    uint16_t code,
    float* volts);
    
int8_t ltc2668_volts_to_code(
    const Ltc2668State* state,
    int8_t selected_dac,
    float volts,
    uint16_t* code);
    
int8_t ltc2668_set_reference_mode(
    const Ltc2668State* state,
    uint8_t is_internal);

int8_t ltc2668_write_dac_input_register(
    const Ltc2668State* state,
    int8_t selected_dac,
    uint16_t code);
       
int8_t ltc2668_write_and_update_dac(
    const Ltc2668State* state,
    int8_t selected_dac,
    uint16_t code);

int8_t ltc2668_update_power_up_dac(
    const Ltc2668State* state,
    int8_t selected_dac);
    
int8_t ltc2668_power_down_dac(
    const Ltc2668State* state,
    int8_t selected_dac);

uint16_t ltc2668_volts_to_code(
    const Ltc2668State* state,
    uint8_t selected_dac,
    float volts);

int8_t ltc2668_set_softspan(
    Ltc2668State* state,
    int8_t selected_dac,
    uint8_t soft_span);
    
int8_t ltc2668_toggle_select(
    Ltc2668State* state,
    uint16_t select_bits);
    
int8_t ltc2668_set_mux(
    Ltc2668State* state,
    uint8_t is_enabled,
    int8_t selected_dac);
    
int8_t ltc2668_set_global_toggle(
    Ltc2668State* state,
    uint8_t is_high);
    
int8_t ltc2668_ramp(
    Ltc2668State* state);
    
#endif // LTC2668_COMM_H
