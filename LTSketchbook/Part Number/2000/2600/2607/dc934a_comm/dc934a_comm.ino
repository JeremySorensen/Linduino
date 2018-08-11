/*!
Linear Technology DC934 Demonstration Board.
LTC2607: 16-Bit, Dual Rail-to-Rail DACs with I2C Interface

Linear Technology DC936 Demonstration Board.
LTC2609: Quad 16-/14-/12-Bit Rail-to-Rail DACs with I²C Interface.

Linear Technology DC812 Demonstration Board.
LTC2606: 16-Bit Rail-to-Rail DACs with I²C Interface.

@verbatim

NOTES
  Setup:
   Set the terminal baud rate to 115200 and select the newline terminator.
   Calibration requires a precision voltmeter.
   No external power supply is required.

USER INPUT DATA FORMAT:
 decimal : 1024
 hex     : 0x400
 octal   : 02000  (leading 0 "zero")
 binary  : B10000000000
 float   : 1024.0

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
*/

#include <Arduino.h>
#include <stdint.h>
//#include "Linduino.h"
#include "LT_I2C.h"
//#include "LT_SPI.h"
#include "ltc2607_comm.h"
//#include "LTC2422.h"
//#include "QuikEval_EEPROM.h"
#include <Wire.h>
#include <SPI.h>
#include "SerialInterface.hpp"
#include "ParseNum.h"

// Globals
//////////

// The serialInteface dispatches commands to your callbacks and helps handle errors
// template numbers are:
// number of serial commands
// number of characters in longest command including args (and spaces between)
// max number of args a command can take
SerialInterface<5, 64, 3> s_inter("LTC2607", "DC934A");

// Keep track of state between commands
Ltc2607State ltc2607_state;
//Ltc2422State ltc2422_state;

// Helper functions
///////////////////

uint8_t get_addr(const char* jumper) {
    if (strcasecmp(jumper, "low") == 0) { return LTC2607_I2C_AD_LOW; }
    if (strcasecmp(jumper, "float") == 0) { return LTC2607_I2C_AD_FLOAT; }
    if (strcasecmp(jumper, "high") == 0) { return LTC2607_I2C_AD_HIGH; }
    
    sinter_error(F("bad_arg"), F("ad(2/1/0) must be low, float, or high, got "), jumper);
    return -1;
}

int8_t get_selected_dac(const char* dac) {
    if (strcasecmp(dac, "a") == 0) { return LTC2607_DAC_A; }
    if (strcasecmp(dac, "b") == 0) { return LTC2607_DAC_B; }
    if (strcasecmp(dac, "BOTH") == 0) { return LTC2607_BOTH; }

    sinter_error(F("bad_arg"), F("DAC channel must be a, b, or both, got "), dac);
    return -1;
}

bool get_write_inputs(char* const argv[], int8_t* dac, uint16_t* code)
{
    bool is_volts;
    if (!get_is_volts(argv[0], &is_volts)) {
        return false;
    }
    
    *dac = get_selected_dac(argv[1]);
    if (*dac < 0) {
        return false;
    }
    
    int8_t result;
    
    if (is_volts) {
        float input_volts;
        if (!parse_f32(argv[2], &input_volts)) {
            return false;
        }
        
        result = ltc2607_volts_to_code(&ltc2607_state, *dac, input_volts, code);
        if (!check_result(result)) { return false; }
    } else {
        if (!parse_u16(argv[2], code)) {
            return false;
        }
    }
}

bool check_result(int8_t result) {
    if (result == LTC2607_ERR_OK) {
        return true;
    } else if (result == LTC2607_ERR_NAK) {
        sinter_error(F("device_error"), F("Device NAKed"));
        return false;
    } else if (result == LTC2607_ERR_NOT_SAME_CAL) {
        sinter_error(
            F("invalid_operation"),
            F("can't set both DACs to same volts with different calibrations"));
        return false;
    } else {
        sinter_error(F("logic_error"), F("An unexpected error occured"));
        return false;
    }
}

void print_results(int8_t result, uint16_t code, int8_t selected_dac, bool do_update) {
    if (!check_result(result)) {
        return;
    }
    
    auto lastStr = do_update ? F(" and updated") : F("");
    
    if (selected_dac == LTC2607_BOTH && !ltc2607_state.same_cal) {
        Serial.print(F("Both DACs set to "));
            Serial.print(code);
            Serial.println(lastStr);
    } else {
        float volts;
        ltc2607_code_to_volts(&ltc2607_state, selected_dac, code, &volts);
        
        if (selected_dac == LTC2607_BOTH) {
            Serial.print(F("Both DACs set to "));
        } else {
            Serial.print(F("DAC "));
            Serial.print(selected_dac == 0 ? F("a") : F("b"));
            Serial.print(F(" set to "));
        }
        Serial.print(code);
        Serial.print(F(" ("));
        Serial.print(volts);
        Serial.print(F(" volts)"));
        Serial.println(lastStr);
    }
}

bool get_is_volts(const char* str, bool* is_volts) {
    if (strcasecmp(str, "volts") == 0) {
        *is_volts = true;
        return true;
    } else if (strcasecmp(str, "code") == 0) {
        *is_volts = false;
        return true;
    } else {
        sinter_error(
            F("bad_arg"),
            F("Expected code or volts, got "),
            str);
        return false;
    }
}

// Command callbacks
////////////////////

void set_i2c_address(int argc, char* const argv[])
{
    int8_t ad2 = get_addr(argv[0]);
    if (ad2 < 0) { return; }

    int8_t ad1 = get_addr(argv[1]);
    if (ad1 < 0) { return; }

    int8_t ad0 = get_addr(argv[2]);
    if (ad0 < 0) { return; }

    uint8_t result = ltc2607_set_i2c_address(&ltc2607_state, ad2, ad1, ad0);
    if (check_result(result)) {
        sinter_println(F("I2C address set to "), ltc2607_state.address);
    }
}

void write_dac_input_register(int argc, char* const argv[])
{
    uint8_t selected_dac;
    uint16_t code;
    get_write_inputs(argv, &selected_dac, &code);

    int8_t result = ltc2607_write_dac_input_register(&ltc2607_state, selected_dac, code);
    
    print_results(result, code, selected_dac, false);
}

void write_and_update_dac(int argc, char* const argv[])
{
    uint8_t selected_dac;
    uint16_t code;
    get_write_inputs(argv, &selected_dac, &code);

    int8_t result = ltc2607_write_and_update_dac(&ltc2607_state, selected_dac, code);
    
    print_results(result, code, selected_dac, true);
}

void update_power_up_dac(int argc, char* const argv[])
{
    int8_t selected_dac = get_selected_dac(argv[0]);
    if (selected_dac < 0) {
        return;
    }

    int8_t result = ltc2607_update_power_up_dac(&ltc2607_state, selected_dac);
    if (check_result(result)) {
        sinter_println(F("DAC channel "), argv[0], F(" updated and powered up"));
    }
}

void power_down_dac(int argc, char* const argv[])
{
    int8_t selected_dac = get_selected_dac(argv[0]);
    if (selected_dac < 0) {
        return;
    }

    int8_t result = ltc2607_power_down_dac(&ltc2607_state, selected_dac);
    if (check_result(result)) {
        sinter_println(F("DAC channel "), argv[0], F(" powered down"));
    } 
}

void setup() {
       
    Serial.begin(115200);

    ltc2607_init(&ltc2607_state);

    s_inter.add_command(
       "i2c_address",
       F("AD2 AD1 AD0 - Set I2C address.\n"
         "    AD(2/1/0) is one of 'low', 'float', or 'high' to match jumpers"),
       set_i2c_address, 
       3);
    
    s_inter.add_command(
        "write",
        F("(volts | code) CH VALUE - Write DAC input registers.\n"
          "    CH is one of 'a', 'b', or 'both'\n"
          "    VALUE is ADC code to write or Volts to set.\n"
          "    Example: write_dac_input volts a 1.23"),
        write_dac_input_register,
        3);
    
    s_inter.add_command(
        "write_update",
        F("(volts | code) CH VALUE - Write DAC input registers and update output.\n"
          "    CH is one of 'a', 'b', or 'both'\n"
          "    VALUE is ADC code to write or Volts to set.\n"
          "    Example: write_update_dac volts a 1.23"),
        write_and_update_dac,
        3);
    
    s_inter.add_command(
        "update",
        F("CH - Update and power up DAC channel.\n"
          "    CH is one of 'a', 'b', or 'both'"),
        update_power_up_dac,
        1);

    s_inter.add_command(
        "power_down",
        F("CH - Power down DAC channel.\n"
          "    CH is one of 'a', 'b', or 'both'"),
        power_down_dac,
        1);
    
    s_inter.greet();
}

// listen for commands entered by the user and dispatch them
void loop() {
    s_inter.listen_for_command();
}
