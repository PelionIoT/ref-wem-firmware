/*
    Commander.cpp

    A simple command line processor and callback for a serial based shell
*/

#include "commander.h"
#include <algorithm>

//our serial interface cli class
Commander cmd;

static void commander_cb_help(vector<string>& params)
{
    cmd.help();
}

Commander::Commander(PinName tx,
                     PinName rx,
                     int baud) : _serial(tx,rx)
{
    //set the prompt up
    _prompt = "rash> ";

    //create our banner
    _banner += "\r\n\r\n\r\n";
    _banner += "\r\n";
    _banner += "     _______  ______    _______      ___  _______  _______  _______ \r\n";
    _banner += "    |       ||    _ |  |       |    |   ||       ||       ||       |\r\n";
    _banner += "    |    _  ||   | ||  |   _   |    |   ||    ___||       ||_     _|\r\n";
    _banner += "    |   |_| ||   |_||_ |  | |  |    |   ||   |___ |       |  |   |\r\n";
    _banner += "    |    ___||    __  ||  |_|  | ___|   ||    ___||      _|  |   |\r\n";
    _banner += "    |   |    |   |  | ||       ||       ||   |___ |     |_   |   |\r\n";
    _banner += "    |___|    |___|  |_||_______||_______||_______||_______|  |___|\r\n";
    _banner += " ______    _______  ______   _______  __   __  ___   ______    _______\r\n";
    _banner += "|    _ |  |       ||      | |       ||  | |  ||   | |    _ |  |       |\r\n";
    _banner += "|   | ||  |    ___||  _    ||  _____||  |_|  ||   | |   | ||  |_     _|\r\n";
    _banner += "|   |_||_ |   |___ | | |   || |_____ |       ||   | |   |_||_   |   |\r\n";
    _banner += "|    __  ||    ___|| |_|   ||_____  ||       ||   | |    __  |  |   |\r\n";
    _banner += "|   |  | ||   |___ |       | _____| ||   _   ||   | |   |  | |  |   |\r\n";
    _banner += "|___|  |_||_______||______| |_______||__| |__||___| |___|  |_|  |___|\r\n";
    _banner += "\r\n";
    _banner += "                   Created by: The ARM Red Team\r\n";
    _banner += "   Michael Anderson, Mo Chen, Nic Costa, Earl Manning, Ben Menchaca,\r\n";
    _banner += "    Ryan Nowakowski, Cristian Prundeanu, Ryan Sherlock, Kyle Stein,\r\n";
    _banner += "                   J. Michael Welsh, Yixin Zhang\r\n";
    _banner += "\r\n\r\n\r\n";

    //set the line rate of the serial
    _serial.baud(115200);

    //hook up our help
    add("help",
        "Get help direct from the RASH!",
        commander_cb_help);
}

Commander::~Commander()
{

}

void Commander::help()
{
    map<string, Command>::const_iterator it;

    _serial.printf("\r\n");

    _serial.printf("RASH Help:\r\n\r\n");

    //walk our commands and print the cmd and it's description for help
    for (it = _cmds.begin(); it != _cmds.end(); ++it) {
        _serial.printf("%-12s - %s\r\n",
                        it->second.strname.c_str(),
                        it->second.strdesc.c_str());
    }
}

int Commander::add(string strname, string strdesc, pFuncCB pcallback)
{
    int nreturn = 0;

    Command mycmd;

    mycmd.strname = strname;
    mycmd.strdesc = strdesc;
    mycmd.pCB     = pcallback;

    _cmds[strname] = mycmd;

    return nreturn;
};

void Commander::banner()
{
    //print our welcome banner
    _serial.printf(_banner.c_str());
}

void Commander::input_handler()
{
    //the next serial read
    int nkey = cmd._serial.getc();

    //push our input into the buffer
    cmd._buffer.push_back(nkey);

    //walk the callbacks and call them one at a time
    for (size_t n = 0; n < cmd._vready.size(); n++) {
        cmd._vready[n]();
    }
}

void Commander::on_ready(pFuncReady cb)
{
    //push it real good
    _vready.push_back(cb);
}

void Commander::del_ready(pFuncReady cb)
{
    //find our callback
    std::vector<pFuncReady>::iterator it = std::find(_vready.begin(), _vready.end(), cb);

    //if this is it then kill it
    if (it != _vready.end()) {
        _vready.erase(it);
    }
}

void Commander::init()
{
    //hook up our serial input handler to the serial interrupt
    _serial.attach(&input_handler);

    //print the prompt!
    _serial.printf(_prompt.c_str());
};

bool Commander::pump()
{
    bool breturn = false;

    //did the user press a key?
    if (_buffer.size() > 0) {

        //signal we got data
        breturn = true;

        //get the the key and echo it back
        int nInput = _buffer.front();

        _buffer.erase(_buffer.begin());

        //if this is enter then process!
        if (nInput == 13) {
            //do we have a blank command?
            if (_strcommand == "") {
                _serial.printf("\r\n");
            } else {
                process(_strcommand);
            }

            //kill the command
            _strcommand = "";

            //print the prompt!
            _serial.printf(_prompt.c_str());

        } else if (nInput == 8 && _strcommand.length() > 0) {
            //if this is delete then truncate our string!
            //remove last char in our command string
            _strcommand = _strcommand.substr(0, _strcommand.length() - 1);

            //print the output to the backspace
            _serial.putc(nInput);

        } else {
            // we are adding to our string
            if (nInput > 31 && nInput < 127) {
                _strcommand += (char)nInput;
                //print the output to the serial!
                _serial.putc(nInput);
            }
        }
    }

    return breturn;
};

int Commander::process(string& strcommand)
{
    int nreturn = 0;
    map<string, Command>::const_iterator it;

    //where our cracked string goes
    vector<string> lsresults;

    //break the command apart
    tokenize(strcommand, lsresults);

    //if we got anything
    if (lsresults.size() > 0) {
        _serial.printf("\r\n\r\n");

        //get our element
        it = _cmds.find(lsresults[0]);
        if (it != _cmds.end()) {
            //call our guy
            it->second.pCB(lsresults);
        } else {
            //we have an unknown command
            _serial.printf("Error Unknown Command!\r\n");
            for (vector<string>::iterator it = lsresults.begin();
                 it != lsresults.end();
                 ++it) {
                _serial.printf(it->c_str());
                _serial.printf("\r\n");
            }
        }

        _serial.printf("\r\n\r\n");
    }

    return nreturn;
}

void Commander::tokenize(string& strcmd, vector<string>& lsresult)
{
    //our temp string builder
    std::string strtemp;

    //lets walk our string and crack it!
    for (unsigned int n = 0; n < strcmd.length(); n++) {
        //do we have our delimiter?
        if (strcmd[n] == ' ') {
            //add it to our list
            lsresult.push_back(strtemp);
            strtemp = "";
        } else {
            //if not add to our string!
            strtemp += strcmd[n];
        }
    }

    //add our last string
    lsresult.push_back(strtemp);
}
