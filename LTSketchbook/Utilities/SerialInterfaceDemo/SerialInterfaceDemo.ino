/*!
SerialInterfaceDemo: Shows how to use the SerialInterface class to easily
make a command based serial interface, complete with automagic help command,
ID command, standard greeting and easy hooking up of commands with callbacks

Copyright (c) 2018, Analog Devices Corp.
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
either expressed or implied, of Analog Devices Corp.

The Analog Devices Linduino is not affiliated with the official Arduino team.
However, the Linduino is only possible because of the Arduino team's commitment
to the open-source community.  Please visit http://www.arduino.cc and
http://store.arduino.cc and consider a purchase that will help fund their
ongoing work.

Error codes used by SerialInterface:
"too_long"          - user sent a command with too many characters
"timeout"           - user sent part of a command but didn't send a newline 
                      before timeout
"bad_command"       - user sent a non-existant command
"not_enough_args"   - user sent a command without enough args
"too_many_args"     - user sent a command with too many args
"logic_error"       - developer tried to add same command twice or tried to add 
                      too many commands

Suggested error codes:
"bad_arg"           - argument doesn't make sense or is out of range for command
"not_supported"     - should work but currently doesn't, or sketch works for 
                      multiple parts and current part doesn't support operation
"invalid_operation" - command is not currently valid (must (or must not) execute
                      another command first, or needs jumper settings or something)
"device_error"      - device responded in an unexpected way
"logic_error"       - encountered a bug in the sketch

NOTE: Make sure you always print a string for every command, and all the output for
      a command must be a single line!

*/

#include <Arduino.h>
#include "SerialInterface.hpp"

// The serialInteface dispatches commands to your callbacks and helps handle errors
// template numbers are:
// number of serial commands
// number of characters in longest command including args (and spaces between)
// max number of args a command can take
SerialInterface<5, 64, 5> s_inter("LTC1234", "DC5678A-B");

const int LED_PIN = 13;

// Define all your callbacks here
// Typically these will parse argv and do some validation and then call
// functions in the library code for the part. This allows the user to 
// just look at the library code for a clean view of how to communicate
// with the part.

//! Prints BEEP! to serial
void beep(int argc, char* const argv[])
{
    s_inter.println(F("BEEP!"));
}

//! Turns led on or off
void led(
    int argc,          //!< 1
    char* const argv[] //!< {'on'} or {'off'}
    )
{
    if (strcmp(argv[0], "on") == 0) {
        digitalWrite(LED_PIN, HIGH);
        s_inter.println("LED On");
    } else if (strcmp(argv[0], "off") == 0) {
        digitalWrite(LED_PIN, LOW);
        s_inter.println("LED Off");
    } else {
        s_inter.error(
            F("bad_arg"),
            F("arg to led command must be 'on' or 'off', got '"),
            argv[0],
            F("'"));
    }
}



// Prints first and last name
void full_name(
    int argc,          //!< 2
    char* const argv[] //!< { first_name, last_name }
    )
{
    s_inter.println(F("Hello"), argv[0], argv[1]);
}

// Blinks the LED once, or 3 times for specified amount of time
void timed_blink(
    int argc,          //!< 1 or 2
    char* const argv[] //!< { on_time } to blink once or { on_time, off_time } for 3x
    )
{
    int on_time = atoi(argv[0]);
    if (on_time < 10 || on_time > 10000) {
        s_inter.error(
            F("bad_arg"),
            F("ON_TIME must be number between 10 and 10000, got "),
            argv[0]);
            return;
    }
    if (argc == 2) {
        int off_time = atoi(argv[1]);
        if (off_time < 10 || off_time > 10000) {
            s_inter.error(
            F("bad_arg"),
            F("OFF_TIME must be number between 10 and 10000, got "),
            argv[1]);
            return;
        }
        for (int i = 0; i < 2; ++i) {
            digitalWrite(LED_PIN, HIGH);
            delay(on_time);
            digitalWrite(LED_PIN, LOW);
            delay(off_time);
        }
    }
    digitalWrite(LED_PIN, HIGH);
    delay(on_time);
    digitalWrite(LED_PIN, LOW);
    
    s_inter.println("ok"); // always have to print something
}

// Prints the number of args received
void count_args(
    int argc,          //!< 0, 1, 2, 3, 4, or 5
    char* const argv[] //!< Any arguments you want, up to 5
    )
{
    s_inter.println(F("Number of args passed in: "), argc);
}

// In setup, add your commands, each with a name, description (for auto generated help command),
// callback (defined above), min args and max args
// then call the greet function for a standard greeting
void setup() {
    
    pinMode(LED_PIN, OUTPUT);
    
    Serial.begin(115200);
    
    // the order that you add the commands doesn't matter

    s_inter.add_command(
       "count_args",                                 // command name
       F("[IN1] [IN2] ... [IN5] - counts the args"), // description used by help
       count_args,                                   // callback
       0,                                            // min number of args (0 by default)
       5);                                           // max number of args (same as min by default)
    
    s_inter.add_command(
        "timed_blink",
        F("ON_TIME [OFF_TIME] - Blink once for ON_TIME ms, or 3 times on and off"),
        timed_blink,
        1,
        2);
    
    s_inter.add_command(
        "full_name",
        F("FIRST LAST - print Hello FIRST LAST"),
        full_name,
        2);
    
    s_inter.add_command(
        "led",
        F("STATE - if STATE='on' turn on led if 'off' turn off"),
        led,
        1);
    
    s_inter.add_command("beep", F("- print 'BEEP!'"), beep);
    
    s_inter.greet();
}

// listen for commands entered by the user and dispatch them
void loop() {
    s_inter.listen_for_command();
}
