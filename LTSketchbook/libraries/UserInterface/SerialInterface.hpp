/*!
SerialInterface: The SerialInterface class allows the develper to easily
make a command based serial interface, complete with automagic help command,
ID command, standard greeting and easy hooking up of commands with callbacks.
See SerialInterfaceDemo.ino for an example.

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
*/

#ifndef SERIAL_INTERFACE
#define SERIAL_INTERFACE

#include "SortedDictionary.hpp"

class StringComparer {
public:
    static int compare(const char* a, const char* b) {
        return strcmp(a, b);
    }
};

typedef void(*CommandFunc)(int argc, char* const argv[]);

struct Command {
    const char* name;                      
    const __FlashStringHelper* description;
    CommandFunc callback;
    int min_args;
    int max_args;
};

//! SerialInterface class lets the develper add commands. When the user enters a command
//! possibly with arguments, the appropriate callback function is called passing in the
//! arguments. Also provides a standard greeting, a help function and an id function.
//! @tparam NCommands the number of commands that will be added
//! @tparam CommandSize the size of the buffer used to hold each command and all arguments
//! @tparam MaxArgs the maximum number of arguments taken by any one command
template <int NCommands, int CommandSize, int MaxArgs>
class SerialInterface
{
    SortedDictionary<const char*, StringComparer, Command, NCommands> command_dict;
    char command_buff[CommandSize];
    char* arg_buff[MaxArgs];
    const char* part_name;
    const char* board_name;

    int read_char(int timeout)
    {
        unsigned long start = millis;
        while (Serial.available() <= 0) {
            if ((timeout > 0) && ((millis - start) > timeout)) {
                return -1;
            }
        }
        return Serial.read();
    }

    int read_command()
    {
        memset(command_buff, 0, CommandSize);
        int arg_index = 0;
        int timeout = 0;
        bool too_long = false;
        for (int i = 0; ; ++i) {
            if (i >= CommandSize) {
                too_long = true;
                i = 0;
            }
            int result = read_char(timeout);
            timeout = 2000;
            if (result < 0) {
                error(F("timeout"), F("timed out waiting for newline"));
                return -1;
            }
            char c = result;
            if (c == '\n') {
                command_buff[i] = '\0';
                if (too_long) {
                    error(F("too_long"), F("command too long"));
                    return -1;
                } else {
                    return arg_index;   
                }
            } else if (c == ' ') {
                if (!too_long) {
                    if (arg_index < MaxArgs) {
                        command_buff[i] = '\0';
                        arg_buff[arg_index] = command_buff + i + 1;
                    }
                    ++arg_index;
                }
            } else {
                if (!too_long) {
                    command_buff[i] = c;
                }
            }
        }
    }
    
    void print_entry(const char* name, const __FlashStringHelper* desc)
    {
        Serial.print("* ");
        Serial.print(name);
        Serial.print(" ");
        Serial.println(desc);
    }
    
    void help()
    {
        Serial.println("Commands:");
        print_entry("help", F("[COMMAND] - Print help for COMMAND or all commands"));
        print_entry("id", F("- show part name and eval board name"));
        const Command* commands = command_dict.get_values();
        for (int i = 0; i < command_dict.get_num_entries(); ++i) {
            print_entry(commands[i].name, commands[i].description);
        }
    }
    
    void help_command(const char* command_str)
    {
        if (strcmp(command_str, "help") == 0) {
            print_entry("help", F("[COMMAND] - Print help for COMMAND or all commands"));
            return;
        }
        
        if (strcmp(command_str, "id") == 0) {
            print_entry("id", F("- show part name and eval board name"));
            return;
        }
        
        Command command;
        if (!command_dict.get_value(command_str, command))
        {
            help();
        } else {
            print_entry(command.name, command.description);
        }
    }
    
    void id()
    {
        Serial.print(part_name);
        Serial.print(F(","));
        Serial.println(board_name);
    }
    
    void print_error_code(const __FlashStringHelper* code)
    {
        Serial.print(F("error ["));
        Serial.print(code);
        Serial.print(F("]: "));
    }

public:
    //! Constructor
    SerialInterface(
        const char* part_name, //!< The name of the part
        const char* board_name //!< The name of the eval board
        )
    {
        this->part_name = part_name; 
        this->board_name = board_name; 
    }
    
    //! Prints a standard greeting to the serial terminal.
    void greet()
    {
        Serial.print(part_name);
        Serial.print(F(","));
        Serial.print(board_name);
        Serial.println(F(" enter 'help' for commands"));
    }

    //! Adds a command to the list of known commands
    void add_command(
        const char* name,                       //!< What the user enters to invoke the command
        const __FlashStringHelper* description, //!< Description for help command
        CommandFunc callback,                   //!< Callback to execute when command is invoked
        int min_args = 0,                       //!< Minimum number of arguments
        int max_args = -1                       //!< Maximum number of arguments (default=min_args)
        )
    {    
        if (command_dict.get_num_entries() >= NCommands) {
            error(F("logic_error"), F("Tried to add too many commands (increase NCommands)"));
        }
        
        if (max_args == -1) { max_args = min_args; }
        
        Command command;
        command.name = name;
        command.description = description;
        command.callback = callback;
        command.min_args = min_args;
        command.max_args = max_args;
        if (!command_dict.insert(name, command)) {
            error(F("logic_error"), F("Command '"), name, F("' already added"));
        }
    }

    //! listen for the user to enter a command and then dispatch it to the correct callback
    void listen_for_command()
    {
        int num_args = read_command();
        if (num_args == -1) { return; }

        if (strcmp(command_buff, "help") == 0) {
            if (num_args == 1) {
                help_command(arg_buff[0]);
            } else {
                help();
            }
            return;
        }
        
        if (strcmp(command_buff, "id") == 0) {
            id();
            return;
        }

        Command command;
        if (!command_dict.get_value(command_buff, command)) {
            error(F("bad_command"), F("command '"), command_buff, "' not found");
            return;
        }

        if (num_args < command.min_args) {
            error(
                F("not_enough_args"),
                F("not enough args for command '"),
                command_buff,
                F("' expected "),
                command.min_args,
                F(" got "),
                num_args);
            return;
        } else if (num_args > command.max_args) {
            error(
                F("too_many_args"),
                F("too many args for command '"),
                command_buff,
                F("' expected "),
                command.max_args,
                F(" got "),
                num_args);
            return;
        }

        command.callback(num_args, &arg_buff[0]);
    }
    
    //! Print a line to Serial
    template <class T>
    void println(
        T item) //!< item to print, can be F("literal") const char* or number
    {
        Serial.println(item);
    }
    
    //! Print 2 things to Serial as a line
    template <class T, class U>
    void println(
        T item_1, //!< 1st item to print, can be F("literal") const char* or number
        U item_2) //!< 2nd item to print, can be F("literal") const char* or number
    {
        Serial.print(item_1);
        Serial.println(item_2);
    }
    
    //! Print 3 things to Serial as a line
    template <class T, class U, class V>
    void println(
        T item_1, //!< 1st item to print, can be F("literal") const char* or number
        U item_2, //!< 2nd item to print, can be F("literal") const char* or number
        V item_3) //!< 3rd item to print, can be F("literal") const char* or number
    {
        Serial.print(item_1);
        Serial.print(item_2);
        Serial.println(item_3);
    }
    
    //! Print 4 things to Serial as a line
    template <class T, class U, class V, class W>
    void println(
        T item_1, //!< 1st item to print, can be F("literal") const char* or number
        U item_2, //!< 2nd item to print, can be F("literal") const char* or number
        V item_3, //!< 3rd item to print, can be F("literal") const char* or number
        W item_4) //!< 4th item to print, can be F("literal") const char* or number
    {
        Serial.print(item_1);
        Serial.print(item_2);
        Serial.print(item_3);
        Serial.println(item_4);
    }
    
    //! Print 5 things to Serial as a line
    template <class T, class U, class V, class W, class X>
    void println(
        T item_1, //!< 1st item to print, can be F("literal") const char* or number
        U item_2, //!< 2nd item to print, can be F("literal") const char* or number
        V item_3, //!< 3rd item to print, can be F("literal") const char* or number
        W item_4, //!< 4th item to print, can be F("literal") const char* or number
        X item_5) //!< 5th item to print, can be F("literal") const char* or number
    {
        Serial.print(item_1);
        Serial.print(item_2);
        Serial.print(item_3);
        Serial.print(item_4);
        Serial.println(item_5);
    }
    
    //! Print 6 things to Serial as a line
    template <class T, class U, class V, class W, class X, class Y>
    void println(
        T item_1, //!< 1st item to print, can be F("literal") const char* or number
        U item_2, //!< 2nd item to print, can be F("literal") const char* or number
        V item_3, //!< 3rd item to print, can be F("literal") const char* or number
        W item_4, //!< 4th item to print, can be F("literal") const char* or number
        X item_5, //!< 5th item to print, can be F("literal") const char* or number
        Y item_6) //!< 5th item to print, can be F("literal") const char* or number
    {
        Serial.print(item_1);
        Serial.print(item_2);
        Serial.print(item_3);
        Serial.print(item_4);
        Serial.print(item_5);
        Serial.println(item_6);
    }
    
    //! print an error message with a code and a standard format
    template <class T>
    void error(
        const __FlashStringHelper* code, //!< Error code
        T message                        //!< Error message
        ) 
    {
        print_error_code(code);
        println(message);
    }
    
    //! print an error message with a code and a standard format
    template <class T, class U>
    void error(
        const __FlashStringHelper* code, //!< Error code
        T item_1,                        //!< Error message item 1
        U item_2)                        //!< Error message item 2
    {
        print_error_code(code);
        println(item_1, item_2);
    }
    
    //! print an error message with a code and a standard format
    template <class T, class U, class V>
    void error(
        const __FlashStringHelper* code, //!< Error code
        T item_1,                        //!< Error message item 1
        U item_2,                        //!< Error message item 2
        V item_3)                        //!< Error message item 3
    {
        print_error_code(code);
        println(item_1, item_2, item_3);
    }
    
    //! print an error message with a code and a standard format
    template <class T, class U, class V, class W>
    void error(
        const __FlashStringHelper* code, //!< Error code
        T item_1,                        //!< Error message item 1
        U item_2,                        //!< Error message item 2
        V item_3,                        //!< Error message item 3
        W item_4)                        //!< Error message item 4
    {
        print_error_code(code);
        println(item_1, item_2, item_3, item_4);
    }
    
    //! print an error message with a code and a standard format
    template <class T, class U, class V, class W, class X>
    void error(
        const __FlashStringHelper* code, //!< Error code
        T item_1,                        //!< Error message item 1
        U item_2,                        //!< Error message item 2
        V item_3,                        //!< Error message item 3
        W item_4,                        //!< Error message item 4
        X item_5)                        //!< Error message item 5
    {
        print_error_code(code);
        println(item_1, item_2, item_3, item_4, item_5);
    }
    
    //! print an error message with a code and a standard format
    template <class T, class U, class V, class W, class X, class Y>
    void error(
        const __FlashStringHelper* code, //!< Error code
        T item_1,                        //!< Error message item 1
        U item_2,                        //!< Error message item 2
        V item_3,                        //!< Error message item 3
        W item_4,                        //!< Error message item 4
        X item_5,                        //!< Error message item 5
        Y item_6)                        //!< Error message item 6
    {
        print_error_code(code);
        println(item_1, item_2, item_3, item_4, item_5, item_6);
    }
}; 

#endif //SERIAL_INTERFACE