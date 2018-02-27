/*!
Linear Technology DC1534A Demonstration Board.
LTC2637: Octal 12-/10-/8-Bit I2C VOUT DACs with 10ppm/°C Reference.

Linear Technology DC1593A Demonstration Board.
LTC2635: Quad 12-/10-/8-Bit I2C VOUT DACs with 10ppm/°C Reference

@verbatim
NOTES
 Setup:
   Set the terminal baud rate to 115200 and select the newline terminator.
   The program displays calculated voltages which are based on the voltage
   of the reference used, be it internal or external. A precision voltmeter
   is needed to verify the actual measured voltages against the calculated
   voltage displayed. If an external reference is used, a precision voltage
   source is required to apply the external reference voltage. A
   precision voltmeter is also required to measure the external reference
   voltage. No external power supply is required. Any assembly option may
   be used: DC1534A-A, DC1534A-B, DC1534A-C, DC1534A-D.


 Explanation of Commands:
   1- Select DAC: Select one of eight DACs to test : A, B, C, D, E, F, G, H.

   2- Write to DAC input register: Value is stored in the DAC for updating
      later, allowing multiple channels to be updated at once, either
      through a software "Update All" command or by asserting the LDAC# pin.
      User will be prompted to enter either a code in hex or decimal, or a
      voltage. If a voltage is entered, a code will be calculated based on
      the active scaling and reference parameters - ideal values if no
      calibration was ever stored.

   3- Write and Update: Similar to item 1, but DAC is updated immediately.

   4- Update DAC: Copies the value from the input register into the DAC
      Register. Note that a "write and update" command writes the code to
      BOTH the input register and DAC register, so subsequent "update"
      commands will simply re-copy the same data (no change in output.)

   5- Power Down DAC: Disable DAC output. Power supply current is reduced.
      DAC code present in DAC registers at time of shutdown are preserved.

   6- Set reference mode, either internal or external: Selecting external
      mode prompts for the external reference voltage, which is used directly
      if no individual DAC calibration is stored. The selection and entered
      voltage are stored to EEPROM so it is persistent across reset / power cycles.

   7- Calibrate DAC: Use a precision voltmeter to obtain and enter VOUT
      readings taken with different DAC codes. Set reference mode FIRST,
      as values are stored separately for internal and external reference
      mode. Entries are used to calculate the closest code to send to the
      DAC to achieve an entered voltage.

   8- Enable / Disable calibration: Switch between stored calibration
      values and defaults. Calibration parameters are stored separately
      for internal and external reference modes. Ideal calibration will
      be used if the calibration parameter valid key is not set.


USER INPUT DATA FORMAT:
 decimal : 1024
 hex     : 0x400
 octal   : 02000  (leading 0 "zero")
 binary  : B10000000000
 float   : 1024.0

@endverbatim

http://www.linear.com/product/LTC2637
http://www.linear.com/product/LTC2635

http://www.linear.com/product/LTC2637#demoboards
http://www.linear.com/product/LTC2635#demoboards

REVISION HISTORY
$Revision: 5074 $
$Date: 2016-05-09 16:57:13 -0700 (Mon, 09 May 2016) $

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
    @ingroup LTC2637
*/

#include <Arduino.h>
#include <stdint.h>
#include "Linduino.h"
#include "LT_SPI.h"
#include "UserInterface.h"
#include "LT_I2C.h"
#include "QuikEval_EEPROM.h"
#include "LTC2637.h"
#include <SPI.h>
#include <Wire.h>

#define EEPROM_CAL_KEY_INT  0x5678          //!< Calibration associated with internal reference
#define EEPROM_CAL_KEY_EXT  0x9ABC          //!< Calibration associated with external reference

// DAC Reference State
// Could have been zero or 1, this allows you to use the
// variable "reference_mode" as the command argument to a write
#define REF_INTERNAL LTC2637_CMD_INTERNAL_REFERENCE   //!< Stored reference state is Internal
#define REF_EXTERNAL LTC2637_CMD_EXTERNAL_REFERENCE   //!< Stored reference state is External

// EEPROM memory locations
#define STORED_REF_STATE_BASE     EEPROM_CAL_STATUS_ADDRESS     //!< Base address of the stored reference state
#define INT_CAL_VALID_BASE        STORED_REF_STATE_BASE + 2     //!< Base address of the "internal ref calibration valid" flag
#define INT_CAL_PARAMS_BASE       INT_CAL_VALID_BASE + 2        //!< Base address of the internal ref calibration parameters
#define EXT_CAL_VALID_BASE        INT_CAL_PARAMS_BASE + 32      //!< Base address of the "external ref calibration valid" flag
#define EXT_CAL_PARAMS_BASE       EXT_CAL_VALID_BASE + 2        //!< Base address of the external ref calibration parameters
#define EXT_REF_V_BASE            EXT_CAL_PARAMS_BASE + 32      //!< Base address of the stored external reference voltage

// Function Declaration
int8_t restore_calibration();               // Read the DAC calibration from EEPROM, Return 1 if successful, 0 if not
void print_title();                         // Print the title block
void print_prompt(int16_t selected_dac);    // Prompt the user for an input command
int16_t prompt_voltage_or_code();
uint16_t get_voltage(float LTC2637_lsb, int16_t LTC2637_offset);
uint16_t get_code();
int8_t calibrate_dac(uint8_t index);        // Calibrate the selected DAC using a voltmeter. The routine does a linear curve fit given two data points.

int8_t menu_1_select_dac(int16_t *selected_dac);
int8_t menu_2_write_to_input_register(int16_t selected_dac);
int8_t menu_3_write_and_update_dac(int16_t selected_dac);
int8_t menu_4_update_power_up_dac(int16_t selected_dac);
int8_t menu_5_power_down_dac(int16_t selected_dac);
int8_t menu_6_set_reference_mode();         // int, ext, if ext, prompt for voltage
int8_t menu_7_calibrate_dacs();

// Global variables
static uint8_t demo_board_connected;               //!< Set to 1 if the board is connected
static uint8_t shift_count;                    //!< The data align shift count. For 16-bit=0, for 12-bits=4
static uint8_t reference_mode;                     //!< Tells whether to set internal or external reference

// Global calibration variables
static float reference_voltage;                //!< Reference voltage, either internal or external
static int16_t LTC2637_offset[9];                  //!< DAC offset - index 8 for "all DACs"
static float LTC2637_lsb[9];                       //!< The LTC2637 lsb - index 8 for "all DACs"
static uint8_t num_of_channels = 8;     // Change to 4 for LTC2635.

// Constants

//! Lookup table for DAC address. Allows the "All DACs" address to be indexed right after DAC D in loops.
//! This technique is very useful for devices with non-monotonic channel addresses.
const uint8_t address_map[9] = {LTC2637_DAC_A, LTC2637_DAC_B, LTC2637_DAC_C, LTC2637_DAC_D, LTC2637_DAC_E, LTC2637_DAC_F, LTC2637_DAC_G, LTC2637_DAC_H, LTC2637_DAC_ALL};  //!< Map entered option 0..2 to DAC address

//! Used to keep track to print voltage or print code
enum
{
  PROMPT_VOLTAGE = 0, /**< 0 */
  PROMPT_CODE = 1     /**< 1 */
};

//! Initialize Linduino
void setup()
// Setup the program
{
  char demo_name[] = "DC1534";      // Demo Board Name stored in QuikEval EEPROM

  quikeval_I2C_init();              // Configure the EEPROM I2C port for 100kHz
  quikeval_I2C_connect();           // Connect I2C to main data port
  Serial.begin(115200);             // Initialize the serial port to the PC
  print_title();
  demo_board_connected = discover_demo_board(demo_name);
  if (demo_board_connected)
  {
    restore_calibration();
    print_prompt(0);
  }
}

//! Repeats Linduino loop
void loop()
{
  int8_t ack=0;
  int16_t user_command;
  static  int16_t selected_dac = 0;     // The selected DAC to be updated (0=A, 1=B ... 8=All).  Initialized to "A".
  // The main control loop
  if (demo_board_connected)             // Do nothing if the demo board is not connected
  {
    if (Serial.available())             // Check for user input
    {
      user_command = read_int();        // Read the user command
      Serial.println(user_command);
      Serial.flush();
      ack = 0;
      switch (user_command)
      {
        case 1:
          ack |= menu_1_select_dac(&selected_dac);
          break;
        case 2:
          ack |= menu_2_write_to_input_register(selected_dac);
          break;
        case 3:
          ack |= menu_3_write_and_update_dac(selected_dac);
          break;
        case 4:
          ack |= menu_4_update_power_up_dac(selected_dac);
          break;
        case 5:
          ack |= menu_5_power_down_dac(selected_dac);
          break;
        case 6:
          ack |= menu_6_set_reference_mode(); // int, ext, if ext, prompt for voltage
          break;
        case 7:
          ack |= menu_7_calibrate_dacs();
          restore_calibration();
          break;
        default:
          Serial.println("Incorrect Option");
          break;
      }
      if (ack) Serial.println("I2C NACK received, check address\n");
      Serial.println("\n*****************************************************************");
      print_prompt(selected_dac);
    }
  }
}

// Function Definitions

//! Select which DAC to operate on
//! @return 0
int8_t menu_1_select_dac(int16_t *selected_dac)
{
  // Select a DAC to operate on
  Serial.print("Select DAC to operate on (0=A, 1=B, 2=C, 3=D, 4=E, 5=F, 6=G, 7=H, 8=All): ");
  *selected_dac = read_int();
  if (*selected_dac != num_of_channels)
    Serial.println((char) (*selected_dac + 0x41));
  else
    Serial.println(F("All"));
  return 0;
}

//! Write data to input register, but do not update DAC output
//! @return ACK bit (0=acknowledge, 1=no acknowledge)
int8_t menu_2_write_to_input_register(int16_t selected_dac)
{
  int8_t ack=0;
  uint16_t dac_code;

  if (prompt_voltage_or_code() == PROMPT_VOLTAGE)
    dac_code = get_voltage(LTC2637_lsb[selected_dac], LTC2637_offset[selected_dac]);
  else
    dac_code = get_code();

  ack |= LTC2637_write(LTC2637_I2C_ADDRESS, LTC2637_CMD_WRITE, address_map[selected_dac], dac_code << shift_count);
  return (ack);
}

//!Write data to DAC register (which updates output immediately)
//! @return ACK bit (0=acknowledge, 1=no acknowledge)
int8_t menu_3_write_and_update_dac(int16_t selected_dac)
{
  int8_t ack=0;
  uint16_t dac_code;

  if (prompt_voltage_or_code() == PROMPT_VOLTAGE)
    dac_code = get_voltage(LTC2637_lsb[selected_dac], LTC2637_offset[selected_dac]);
  else
    dac_code = get_code();

  ack |= LTC2637_write(LTC2637_I2C_ADDRESS, LTC2637_CMD_WRITE_UPDATE, address_map[selected_dac], dac_code << shift_count);
  return (ack);
}

//! Update DAC with data that is stored in input register, power up if sleeping
//! @return ACK bit (0=acknowledge, 1=no acknowledge)
int8_t menu_4_update_power_up_dac(int16_t selected_dac)
{
  // Update DAC
  int8_t ack=0;
  ack |= LTC2637_write(LTC2637_I2C_ADDRESS, LTC2637_CMD_UPDATE, address_map[selected_dac], 0x0000);
  Serial.print("DAC ");
  if (selected_dac != num_of_channels)
    Serial.print((char) (selected_dac + 0x41));
  else
    Serial.print("All");
  Serial.println(" powered up!");
  return (ack);
}

//! Power down DAC
//! @return ACK bit (0=acknowledge, 1=no acknowledge)
int8_t menu_5_power_down_dac(int16_t selected_dac)
{
  // Power down DAC
  int8_t ack=0;
  ack |= LTC2637_write(LTC2637_I2C_ADDRESS, LTC2637_CMD_POWER_DOWN, address_map[selected_dac], 0x0000);
  Serial.print("DAC ");
  if (selected_dac != num_of_channels)
    Serial.print((char) (selected_dac + 0x41));
  else
    Serial.print("All");
  Serial.println(" powered down!");
  return (ack);
}

//! Set reference mode and store to EEPROM
//! @return 0
int8_t menu_6_set_reference_mode(void) // int, ext, if ext, prompt for voltage
{
  int16_t user_input;
  Serial.println("Select reference mode - 0 for Internal, 1 for External");
  user_input = read_int();
  if (user_input == 1)
  {
    reference_mode = REF_EXTERNAL;
    Serial.println(F("External reference mode selected"));
  }
  else
  {
    reference_mode = REF_INTERNAL;
    Serial.println(F("Internal reference mode selected"));
  }
  restore_calibration();
}

//! Calibrate all DACs by measuring two known outputs
//! @return 0
int8_t menu_7_calibrate_dacs()
{
  // Calibrate the DACs using a multi-meter
  uint8_t i;
  for (i = 0; i < num_of_channels; i++)
  {
    calibrate_dac(i);   // Run calibration routine
  }
  return (0);
}


//! Read stored calibration parameters from nonvolatile EEPROM on demo board
//! @return Return 1 if successful, 0 if not
int8_t restore_calibration()
// Read the DAC calibration from EEPROM
// Return 1 if successful, 0 if not
{
  int8_t ack=0;
  uint8_t i;
  float dac_count;                                  // The number of codes, 4096 for 12 bits, 65536 for 16 bits

  if (reference_mode == REF_EXTERNAL)
  {
    Serial.println(F("External reference mode set"));
    Serial.print("Enter external reference voltage: ");
    reference_voltage = read_float();
    Serial.print(reference_voltage, 3);
    Serial.println("V");
  }
  else  // EITHER reference is set to internal, OR not programmed in which case default to internal
  {
    reference_mode = REF_INTERNAL; // Redundant if already set
    Serial.println("Internal reference mode set");
  }

  // Write the reference mode to the DAC right away
  ack |= LTC2637_write(LTC2637_I2C_ADDRESS, reference_mode, LTC2637_DAC_ALL, 0x0000);

  // The following two IF statements are used to allow the program to run with
  // a QuikEval string that does not contain the demo board option.
  // If the demo board option is found then these values are overwritten.
  if (strcmp(demo_board.product_name, "LTC2637-LMI") == 0)
  {
    // LTC2636-LM, 12-bits, 2.5V full scale
    shift_count = 4;
    if (reference_mode == REF_INTERNAL)
    {
      reference_voltage = 2.5;
      Serial.println("Internal reference voltage = 2.5 V");
    }
    dac_count = 4096;
  }
  if (strcmp(demo_board.product_name, "LTC2637-HZ") == 0)
  {
    // LTC2636-HZ, 12-bits, 4.096V full scale
    shift_count = 4;
    if (reference_mode == REF_INTERNAL)
    {
      reference_voltage = 4.096;
      Serial.println("Internal reference voltage = 4.096 V");
    }
    dac_count = 4096;
  }
  if (strcmp(demo_board.product_name, "LTC2637-HMI") == 0)
  {
    // LTC2636-HM, 12-bits, 4.096V full scale
    shift_count = 4;
    if (reference_mode == REF_INTERNAL)
    {
      reference_voltage = 4.096;
      Serial.println("Internal reference voltage = 4.096 V");
    }
    dac_count = 4096;
  }
  if (strcmp(demo_board.product_name, "LTC2637-LZ") == 0)
  {
    // LTC2636-LZ, 12-bits, 4.096V full scale
    shift_count = 4;
    if (reference_mode == REF_INTERNAL)
    {
      reference_voltage = 2.5;
      Serial.println("Internal reference voltage = 2.5 V");
    }
    dac_count = 4096;
  }

  for (i = 0; i <= num_of_channels; i++)
  {
    LTC2637_offset[i] = 0;
    LTC2637_lsb[i] = reference_voltage / dac_count;
  }
  for (i=0; i<=num_of_channels; ++i)
  {
    Serial.print("DAC ");
    Serial.print((char) ('A' + i));
    Serial.print(" offset: ");
    Serial.print(LTC2637_offset[i]);
    Serial.print(" , lsb: ");
    Serial.print(LTC2637_lsb[i]*1000, 4);
    Serial.println(" mv");
  }
  return ack;
}

//! Prompt user to enter a voltage or digital code to send to DAC
//! @return prompt type
int16_t prompt_voltage_or_code()
{
  int16_t user_input;
  Serial.print(F("Type 1 to enter voltage, 2 to enter code:"));
  Serial.flush();
  user_input = read_int();
  Serial.println(user_input);

  if (user_input != 2)
    return(PROMPT_VOLTAGE);
  else
    return(PROMPT_CODE);
}

//! Get voltage from user input, calculate DAC code based on lsb, offset
//! @return Returns DAC voltage
uint16_t get_voltage(float LTC2637_lsb, int16_t LTC2637_offset)
{
  float dac_voltage;

  Serial.print(F("Enter Desired DAC output voltage: "));
  dac_voltage = read_float();
  Serial.print(dac_voltage);
  Serial.println(" V");
  Serial.flush();
  return(LTC2637_voltage_to_code(dac_voltage, LTC2637_lsb, LTC2637_offset));
}

//! Get code to send to DAC directly, in decimal, hex, or binary
//! @return Returns DAC code from user
uint16_t get_code()
{
  uint16_t returncode;
  Serial.println("Enter Desired DAC Code");
  Serial.print("(Format 32768, 0x8000, 0100000, or B1000000000000000): ");
  returncode = (uint16_t) read_int();
  Serial.print("0x");
  Serial.println(returncode, HEX);
  Serial.flush();
  return(returncode);
}

//! Prints the title block when program first starts.
void print_title()
{
  Serial.println("");
  Serial.println(F("*****************************************************************"));
  Serial.println(F("* DC1534 Demonstration Program                                  *"));
  Serial.println(F("*                                                               *"));
  Serial.println(F("* This program demonstrates how to send data to the LTC2637     *"));
  Serial.println(F("* Octal 12/10/8-bit DAC found on the DC1534 demo board.            *"));
  Serial.println(F("*                                                               *"));
  Serial.println(F("* Set the baud rate to 115200 and select the newline terminator.*"));
  Serial.println(F("*                                                               *"));
  Serial.println(F("*****************************************************************"));
}

//! Prints main menu.
void print_prompt(int16_t selected_dac)
{
  Serial.println(F("\nCommand Summary:"));
  Serial.println(F("  1-Select DAC"));
  Serial.println(F("  2-Write to input register (no update)"));
  Serial.println(F("  3-Write and update DAC"));
  Serial.println(F("  4-Update / power up DAC"));
  Serial.println(F("  5-Power down DAC"));
  Serial.println(F("  6-Set reference mode"));
  Serial.println(F("  7-Calibrate DAC"));

  Serial.println("\nPresent Values:");
  Serial.print("  Selected DAC: ");
  if (selected_dac != 8)
    Serial.println((char) (selected_dac + 0x41));
  else
    Serial.println("All");
  Serial.print("  DAC Reference: ");
  if (reference_mode == REF_INTERNAL)
    Serial.println("Internal");
  else
  {
    Serial.print(F("External "));
    Serial.print(reference_voltage, 5);
    Serial.println(F("V reference, please verify"));
  }
  Serial.print(F("Enter a command:"));
  Serial.flush();
}

//! Calibrate the selected DAC using a voltmeter. The routine
//! does a linear curve fit given two data points.
//! @return ACK bit (0=acknowledge, 1=no acknowledge)
int8_t calibrate_dac(uint8_t index)
{
  int8_t ack=0;
  uint16_t code1 = 0x0200;                            //! Calibration code 1
  uint16_t code2 = 0x0FFF;                            //! Calibration code 2
  float voltage1;                                     //! Calibration voltage 1
  float voltage2;                                     //! Calibration voltage 2
  Serial.println("");
  Serial.print("Calibrating DAC ");
  Serial.println((char) (0x41 + index));
  // Left align 12-bit code1 to 16 bits & write to DAC
  ack |= LTC2637_write(LTC2637_I2C_ADDRESS, LTC2637_CMD_WRITE_UPDATE, index, code1 << shift_count);
  Serial.print("DAC code set to 0x");
  Serial.println(code1, HEX);
  Serial.print("Enter measured DAC voltage:");
  voltage1 = read_float();
  Serial.print(voltage1, 6);
  Serial.println(" V");
  // Left align 12-bit code2 to 16 bits & write to DAC
  ack |= LTC2637_write(LTC2637_I2C_ADDRESS, LTC2637_CMD_WRITE_UPDATE, index, code2 << shift_count);
  Serial.print("DAC code set to 0x");
  Serial.println(code2, HEX);
  Serial.print("Enter measured DAC voltage:");
  voltage2 = read_float();
  Serial.print(voltage2, 6);
  Serial.println(" V");
  LTC2637_calibrate(code1, code2, voltage1, voltage2, &LTC2637_lsb[index], &LTC2637_offset[index]);
  return(ack);
}
