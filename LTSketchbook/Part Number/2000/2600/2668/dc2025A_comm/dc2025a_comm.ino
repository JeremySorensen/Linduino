/*!
Linear Technology DC2025A Demonstration Board.
LTC2668: 16 Channel SPI 16-/12-Bit Rail-to-Rail DACs with 10ppm/C Max Reference.

@verbatim
NOTES
  Setup:
   Set the terminal baud rate to 115200 and select the newline terminator.

   An external +/- 15V power supply is required to power the circuit.

   The program displays calculated voltages which are based on the voltage
   of the reference used, be it internal or external. A precision voltmeter
   is needed to verify the actual measured voltages against the calculated
   voltage displayed.

   If an external reference is used, a precision voltage
   source is required to apply the external reference voltage. A
   precision voltmeter is also required to measure the external reference
   voltage.

@endverbatim

http://www.linear.com/product/LTC2668

http://www.linear.com/product/LTC2668#demoboards

REVISION HISTORY
$Revision: 3659 $
$Date: 2015-07-01 10:19:20 -0700 (Wed, 01 Jul 2015) $

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
*/

#include <Arduino.h>
#include <stdint.h>
#include "Linduino.h"
#include "LT_SPI.h"
#include "SerialInterface.hpp"
#include "LT_I2C.h"
#include "QuikEval_EEPROM.h"
#include "ltc2668_comm.h"
#include <SPI.h>
#include <Wire.h>
#include "ParseNum.h"

// Globals
//////////

SerialInterface<10, 64, 4> s_inter("LTC2668", "DC2505A");

// Keep track of state between commands
Ltc2668State ltc2668_state;

// Helper Functions
///////////////////

bool get_is_volts(const char* str, bool* is_volts) {
    if (strcasecmp(str, "volts") == 0) {
        *is_volts = true;
        return true;
    } else if (strcasecmp(str, "code") == 0) {
        *is_volts = false;
        return true;
    } else {
        sinter_error(F("bad_arg"), F("Expected code or volts, got "), str);
        return false;
    }
}

bool get_selected_dac(const char* str, int8_t* selected_dac) {
    if (strcasecmp(str, "all") == 0) {
        *selected_dac = LTC2668_ALL_DACS;
    } else if (!parse_u8(str, selected_dac)) {
        Serial.println(str);
        return false;
    }
    return true;
}

bool get_write_inputs(char* const argv[], int8_t* dac, uint16_t* code)
{
    bool is_volts;
    if (!get_is_volts(argv[0], &is_volts)) {
        return false;
    }
    
    if (!get_selected_dac(argv[1], dac)) {
        return false;
    }
    
    int8_t result;
    
    if (is_volts) {
        float input_volts;
        if (!parse_f32(argv[2], &input_volts)) {
            return false;
        }
        result = ltc2668_volts_to_code(&ltc2668_state, *dac, input_volts, code);
        if (!check_result(result)) { return false; }
    } else {
        if (!parse_u16(argv[2], code)) {
            return false;
        }
    }
}

bool check_result(int8_t result) {
    if (result == LTC2668_ERR_OK) {
        return true;
    } else if (result == LTC2668_ERR_MISMATCH) {
        sinter_error(F("device_error"), F("Mismatch during readback of previous value"));
        return false;
    } else if (result == LTC2668_ERR_NOT_SAME_SPAN) {
        sinter_error(
            F("invalid_operation"),
            F("can't set all DACs to same volts with different spans"));
        return false;
    } else if (result == LTC2668_ERR_BAD_SPAN) {
        sinter_error(F("logic_error"), F("bad span"));
        return false;
    } else {
        sinter_error(F("logic_error"), F("An unexpected error occured"));
        return false;
    }
}

void print_results(Ltc2668State* state, int8_t result, uint16_t code, int8_t selected_dac, bool do_update) {
    if (!check_result(result)) {
        return;
    }
    
    auto lastStr = do_update ? F(" and updated") : F("");
    
    if (selected_dac == LTC2668_ALL_DACS && !state->all_same_span) {
        Serial.print(F("All DACs set to "));
            Serial.print(code);
            Serial.println(lastStr);
    } else {
        float volts;
        ltc2668_code_to_volts(state, selected_dac, code, &volts);
        
        if (selected_dac == LTC2668_ALL_DACS) {
            Serial.print(F("All DACs set to "));
        } else {
            Serial.print(F("DAC "));
            Serial.print(selected_dac);
            Serial.print(F(" set to "));
        }
        Serial.print(code);
        Serial.print(F(" ("));
        Serial.print(volts);
        Serial.print(F(" volts)"));
        Serial.println(lastStr);
    }
}

bool build_mask(const char* str, uint16_t* mask) {
    *mask = 0;
    if (str == 0) {
        return true;
    }

    if (strcasecmp(str, "all") == 0) {
        *mask = 0xFFFF;
        return true;
    }
    
    uint16_t mask_temp = 0;
    uint8_t index = 0;
    bool started = false;
    for (int i = 0; ; ++i) {
        if (str[i] == ',' || str[i] == '\0') {
            if (index > 15) {
                sinter_error(F("bad_arg"), F("got bad channel for toggle bits"));
                return false;
            }
            if (started) {
                started = false;
                mask_temp |= 1 << index;
            }
            index = 0;
            if (str[i] == '\0') {
                *mask = mask_temp;
                return true;
            }
        } else if ((str[i] < '0') || (str[i] > '9')) {
            sinter_error(F("bad_arg"), F("got bad channel for toggle bits"));
                return false;
        } else {
            started = true;
            index *= 10;
            index += str[i] - '0';
        }
    }   
}

// Command callbacks
////////////////////

void set_reference(int argc, char* const argv[])
{
    bool is_internal;
    if (strcasecmp(argv[0], "internal") == 0) {
        is_internal = true;
    } else if (strcasecmp(argv[0], "external") == 0) {
        is_internal = false;
    } else {
        sinter_error(F("bad_arg"), F("Expected internal or external, got "), argv[0]);
        return;
    }
    
    int8_t result = ltc2668_set_reference_mode(&ltc2668_state, is_internal);
    if (check_result(result)) {
        sinter_println(F("reference set to "), argv[0]);
    }
}

void write_input(int argc, char* const argv[])
{
    int8_t selected_dac;
    uint16_t code;
    if (!get_write_inputs(argv, &selected_dac, &code)) {
        return false;
    }
    
    int8_t result = ltc2668_write_dac_input_register(&ltc2668_state, selected_dac, code);
    
    print_results(&ltc2668_state, result, code, selected_dac, false);
}

void write_and_update(int argc, char* const argv[])
{
    int8_t selected_dac;
    uint16_t code;
    if (!get_write_inputs(argv, &selected_dac, &code)) {
        return false;
    }
    
    int8_t result = ltc2668_write_and_update_dac(&ltc2668_state, selected_dac, code);
    
    print_results(&ltc2668_state, result, code, selected_dac, true);
}

void update_power_up(int argc, char* const argv[])
{  
    int8_t selected_dac;
    if (!get_selected_dac(argv[0], &selected_dac)) {
        return;
    }
    
    int8_t result = ltc2668_update_power_up_dac(&ltc2668_state, selected_dac);
    if (check_result(result)) {
        if (selected_dac == LTC2668_ALL_DACS) {
            sinter_println(F("All DACs updated and powered up"));
        } else {
            sinter_println(F("DAC "), selected_dac, F(" updated and powered up"));
        }
    }
}

void power_down(int argc, char* const argv[])
{   
    int8_t selected_dac;
    if (!get_selected_dac(argv[0], &selected_dac)) {
        return;
    }
    
    int8_t result = ltc2668_power_down_dac(&ltc2668_state, selected_dac);
    if (check_result(result)) {
        if (selected_dac == LTC2668_ALL_DACS) {
            sinter_println(F("All DACs powered down"));
        } else {
            sinter_println(F("DAC "), selected_dac, F(" powered down"));
        }
    }
}

void set_span(int argc, char* const argv[])
{
    int8_t selected_dac;
    if (!get_selected_dac(argv[0], &selected_dac)) {
        return;
    }
    
    uint8_t span;
    if (strcmp(argv[1], "5") == 0) {
        span = LTC2668_SPAN_0_TO_5V;
    } else if (strcmp(argv[1], "10") == 0) {
        span = LTC2668_SPAN_0_TO_10V;
    } else if (strcmp(argv[1], "+-5") == 0) {
        span = LTC2668_SPAN_PLUS_MINUS_5V;
    } else if (strcmp(argv[1], "+-10") == 0) {
        span = LTC2668_SPAN_PLUS_MINUS_10V;
    } else if (strcmp(argv[1], "+-2.5") == 0) {
        span = LTC2668_SPAN_PLUS_MINUS_2V5;
    } else {
        sinter_error(
            F("bad_arg"),
            F("expected one of 5, 10, +-5, +-10 or +-2.5 got "),
            argv[1]);
    }
    
    int8_t result = ltc2668_set_softspan(&ltc2668_state, selected_dac, span);
    if (check_result(result)) {
        if (selected_dac == LTC2668_ALL_DACS) {
            sinter_println(
                F("All DACs span set to "),
                argv[1]);
        } else {
            sinter_println(
                F("DAC "),
                selected_dac,
                F(" span set to "),
                argv[1]);
        }
    }
}

void toggle_select(int argc, char* const argv[]) 
{
    if (argc == 3) {
        sinter_error(F("bad_arg"), F("Expected 2 or 4 args, got 3"));
        return;
    }
    
    char* set_arg = 0;
    char* unset_arg = 0;
    if (strcasecmp(argv[0], "set") == 0) {
        set_arg = argv[1];
    } else if (strcasecmp(argv[0], "clear") == 0) {
        unset_arg = argv[1];
    } else {
        sinter_error(F("bad_arg"), F("Expected 'set' or 'clear', got "), argv[0]);
        return;
    }
    
    if (argc == 4) {
        if (strcasecmp(argv[2], "set") == 0) {
            set_arg = argv[3];
        } else if (strcasecmp(argv[2], "clear") == 0) {
            unset_arg = argv[3];
        } else {
            sinter_error(F("bad_arg"), F("Expected 'set' or 'clear', got "), argv[2]);
        } 
    }
    
    uint16_t set_mask = 0;
    uint16_t unset_mask = 0;
    
    if (set_arg != 0 && !build_mask(set_arg, &set_mask)) {
        return;
    }
    
    if (unset_arg != 0 && !build_mask(unset_arg, &unset_mask)) {
        return;
    }
    
    uint16_t select_bits = ltc2668_state.select_bits;
    select_bits |= set_mask;
    select_bits &= ~unset_mask;
    
    ltc2668_state.select_bits = select_bits;
    
    int8_t result = ltc2668_toggle_select(&ltc2668_state, select_bits);
    
    if (check_result(result)) {
        if (select_bits == 0) {
            Serial.println("No bits are set");
        } else {
            Serial.print("These bits are set: ");
            for (int i = 0; select_bits != 0; ++i) {
                bool is_set = (select_bits & 1) != 0;
                select_bits >>= 1;
                if (is_set) {
                    Serial.print(i);
                    if (select_bits != 0) {
                        Serial.print(", ");
                    } else {
                        Serial.println("");
                    }
                }
            }
        }
    }
}

void set_mux(int argc, char* const argv[]) {
    int8_t selected_dac = ltc2668_state.mux_selected_dac;
    bool enable = true;
    if (strcasecmp(argv[0], "enable") == 0) {
        // defaults are good
    } else if (strcasecmp(argv[0], "disable") == 0) {
        enable = false;
    } else if (!get_selected_dac(argv[0], &selected_dac)) {
        return;
    }
    
    if (selected_dac == LTC2668_ALL_DACS) {
        sinter_error(F("bad_arg"), F("Mux only works on a single channel"));
        return;
    }
    
    int8_t result = ltc2668_set_mux(&ltc2668_state, enable, selected_dac);
    if (check_result(result)) {
        if (enable) {
            sinter_println(F("Monitor MUX enabled for channel "), ltc2668_state.mux_selected_dac);
        } else {
            sinter_println(F("Monitor MUX disabled"));
        }
    }
}

void set_global_toggle(int argc, char* const argv[]) {
    uint8_t is_high;
    if (strcasecmp(argv[0], "high") == 0) {
        is_high = 1;
    } else if (strcasecmp(argv[0], "low") == 0) {
        is_high = 0;
    } else if (strcasecmp(argv[0], "toggle") == 0) {
        is_high ^= ltc2668_state.global_toggle;
    } else {
        sinter_error(
            F("bad_arg"),
            F("expected one of high, low, or toggle got "),
            argv[0]);
    }
    
    int8_t result = ltc2668_set_global_toggle(&ltc2668_state, is_high);
    if (check_result(result)) {
        sinter_println(F("Set global toggle to "), is_high ? F("high") : F("low"));
    }
}

void ramp(int argc, char* const argv[]) {
    int8_t result = ltc2668_ramp(&ltc2668_state);
    if (check_result(result)) {
        sinter_println(F("Set a ramp of codes accross all channels"));
    }
}

void setup() {
       
    Serial.begin(115200);

    ltc2668_init(&ltc2668_state);

    s_inter.add_command(
       "reference",
       F("internal | external - Set the reference to be internal or external\n"
         "    Example: reference internal"),
       set_reference, 
       1);
    
    s_inter.add_command(
        "write",
        F("(volts | code) CH VALUE - Write input register(s).\n"
          "    CH is 0-16, or 'all'\n"
          "    VALUE is ADC code to write or Volts to set.\n"
          "    Example: write_input volts 2 1.23"),
        write_input,
        3);
    
    s_inter.add_command(
        "write_update",
        F("(volts | code) CH VALUE - Write input register(s) and update output(s).\n"
          "    CH is 0-16 or 'all' \n"
          "    VALUE is ADC code to write or Volts to set.\n"
          "    Example: write_update volts 2 1.23"),
        write_and_update,
        3);
    
    s_inter.add_command(
        "update",
        F("CH - Update and power up DAC channel(s).\n"
          "    CH is 0-16, or 'all'\n"
          "    Example: update 0"),
        update_power_up,
        1);

    s_inter.add_command(
        "power_down",
        F("CH - Power down DAC channel(s).\n"
          "    CH is 0-16, or 'all'\n"
          "    Example: power_down 3"),
        power_down,
        1);
        
    s_inter.add_command(
        "span",
        F("CH SPAN - Set the span for the channel(s).\n"
          "    CH is 0-15 or 'all'\n"
          "    SPAN is 5, 10, +-5, +-10, or +-2.5\n"
          "    Example: span all 10"),
        set_span,
        2);
        
    s_inter.add_command(
        "select_bits",
        F("[set SET_BITS] [clear CLEAR_BITS] - Set and unset channel select bits\n"
          "    SET_BITS is comma separated list of bit indices to set or 'all'\n"
          "    CLEAR_BITS is comma separated list of bit indices to clear or 'all'\n"
          "    must have 'set SET_BITS' or 'clear CLEAR_BITS' or both\n"
          "    Example: select_bits set 1,3,7 clear 2,6\n"
          "    Example: select_bits set all\n"
          "    Example: select_bits clear 5"),
        toggle_select,
        2,
        4);
        
    s_inter.add_command(
        "mux",
        F("(CH | enable | disable) - Set mux to monitor a channel or disable it.\n"
          "    CH is 0-15\n"
          "    if 'enable' is passed previously enabled channel is reenabled\n"
          "    Example: mux 1\n"
          "    Example: mux enable\n"
          "    Example: mux disable\n"),
        set_mux,
        1);
          
    s_inter.add_command(
        "global_toggle",
        F("(high | low) - Set global toggle bit high or low"),
        set_global_toggle,
        1);
        
    s_inter.add_command(
        "ramp",
        F("- Set a ramp over all channels, each channel has a higher code than the previous"),
        ramp);
    
    s_inter.greet();
}

// listen for commands entered by the user and dispatch them
void loop() {
    s_inter.listen_for_command();
}

    
    
