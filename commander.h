#ifndef _COMMANDER
#define _COMMANDER

/*
    Commander

    A simple command line processor and callback for a serial based shell
*/

#include <string>
#include <map>
#include <vector>
#include "mbed.h"


using namespace std;

/*
    callback type for the command
*/
typedef Callback<void(std::vector<std::string>&)> pFuncCB;


/*
    callback type for command ready
*/
typedef Callback<void()> pFuncReady;

/*
    class: cmd

    internal class used to store the commands added to command
*/
class Command
{
public:
    std::string strname;
    std::string strdesc;

    pFuncCB pCB;
};

/*
    class: commander

    this is the main commander class used to provide the RASH (Redteam Abstract SHell) shell over serial
*/
class Commander
{
public:

    /*
        constructor
    */
    Commander(PinName tx  = USBTX,
              PinName rx  = USBRX,
              int baud    = MBED_CONF_PLATFORM_STDIO_BAUD_RATE);

    /*
        destructor
    */
    ~Commander();

    /*
        Function: add

        add a command and its callback to commander

        Params:
        string strname      - the command to add to the console
        string strDesc      - the description of the command and help text
                              it does this and is used like "command <param1> <param2>"
        pFuncCB pcallback   - the handler for this cli command see commander.h for definition
    */
    int add(std::string strname,
            std::string strdesc,
            pFuncCB pcallback);

    /*
        Function: init

        init the serial console listening and processing

        Params:
        none.
    */
    void init();

    /*
        Function: pump

        pump the serial console listening and processing

        Params:
        none.

        Returns:
        boolean true = we got keys / false = we got nothing
    */
    bool pump();

    /*
        function: tokenize

        crack the strings up and put them into a list<string> with the command as 1st parameter
        the delimiter used is space

        params:
        string& strcmd            - the command to crack into parts
        vector<string>& lsresult  - the list to put the results in

        returns nothing.
    */
    void tokenize(std::string& strcmd,
                  std::vector<std::string>& lsresult);

    /*
        function: process

        process the command line

        params:
        string& strcommand - the command line to crack open and process
    */
    int process(std::string& strcommand);

    /*
        Function: printf

        char/wchar_t unicode asccii neutral printf function.
        uses same format and escapse as printf

        Parameters:
        Args&&... args - variable arguments and format of print string

        Returns:
        positive int = success
        negative int = failure

        see printf/wprintf for specifics
    */
    inline void printf(const char *format, ...)
    {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        _serial.puts(buffer);
        va_end (args);
    }

    /*
        Function: help

        dump the help for the current commands

        Params:
        none

        returns:
        nothing
    */
    void help(std::vector<std::string>&);

    /*
        Function: banner

        print our banner!

        Params:
        none

        returns:
        nothing
    */
    void banner();

    /*
        function: input_handler

        This is the io handler for the serialport. All it does is append
        our current input buffer with the last thing on the port.

        params:
        none.

        returns:
        nothing.
    */
    void input_handler();

    /*
        function: on_ready

        add a handler to recieve the on ready to process command
        for a more asynchronous behavior to the commander interface

        params:
        pFuncReady cb - the callback to get the ready command

        returns:
        nothing.
    */
    void on_ready(pFuncReady cb);

    /*
        function: del_ready

        remove a handler to recieve the on ready to process command

        params:
        pFuncReady cb - the callback to remove

        returns:
        nothing.
    */
    void del_ready(pFuncReady cb);

protected:

    //map of command name to info class with callback
    std::map<std::string, Command> _cmds;

    //our instance of the serial class
    RawSerial _serial;

    //our default strings
    std::string _prompt;
    std::string _banner;

    //our input buffer for serial
    std::vector<int> _buffer;

    //this is the string we are building with our pump
    std::string _strcommand;

    //callbacks interested it async cmd processing
    std::vector<pFuncReady> _vready;
};

//our serial interface cli class
extern Commander cmd;

#endif
