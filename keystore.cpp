/* mbed Microcontroller Library
 * Copyright (c) 2018 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "keystore.h"
#include "fs.h"

#include <errno.h>
#include <stdio.h>

using namespace std;

Keystore::Keystore() : _strfilepath(KEYSTORE_DEFAULT_PATH)
{
}

Keystore::Keystore(std::string path) : _strfilepath(path)
{
}

Keystore::~Keystore()
{
    //do cleanup
}

std::string Keystore::realpath()
{
    string path;

    path = FS_MOUNT_POINT;
    if (_strfilepath[0] != '/') {
        path += "/";
    }
    path += _strfilepath;

    return path;
}

std::string Keystore::path()
{
    return _strfilepath;
}

int Keystore::check_path()
{
    int ret;
    string dir;
    size_t slash;
    string path;

    path = realpath();
    slash = path.find_last_of("/");
    if (string::npos != slash) {
        dir = path.substr(0, slash);
        ret = ::mkdir(dir.c_str(), 0777);
        if (0 != ret && EEXIST != errno) {
            return ret;
        }
    }

    return 0;
}

void Keystore::kill_all()
{
    remove(realpath().c_str());
}

int Keystore::open()
{
    int ret;
    FILE *fp;
    char buffer[64];
    size_t bytes_read = 0;

    //read the file into this
    std::string strfile;

    ret = check_path();
    if (0 != ret) {
        return ret;
    }

    fp = fopen(realpath().c_str(), "r");
    if (NULL == fp) {
        return errno;
    }

    //start reading
    for (;;) {

        //clear our buffer
        memset(buffer, 0, sizeof(buffer));

        //Read a chunk of the database
        bytes_read = fread(buffer, 1, sizeof(buffer) - 1, fp);

        //are we done reading?
        if (bytes_read <= 0) {
            //convert the file to the internal state
            to_db(strfile);

            //exit the loop
            break;
        }

        //append the read to the file string
        strfile += buffer;
    }

    //close the file
    fclose(fp);

    return 0;
};

/* mbed-os does not implement tmpnam or tmpfile */
char *Keystore::mktmp(char *out)
{
    snprintf(out, L_tmpnam, "%s", FS_MOUNT_POINT "/keystore.tmp");
    return out;
}

void Keystore::write()
{
    int ret;
    FILE *fp;
    size_t bytes;
    string real;
    char fname[L_tmpnam] = {0};

    //convert the database to file writable string
    std::string strfile = to_file();

    //open the file
    mktmp(fname);
    if (strlen(fname) == 0) {
        printf("ERROR: failed to generate tmp file name\n");
        return;
    }

    fp = fopen(fname, "w");
    if (NULL == fp) {
        printf("ERROR: failed to open tmp file %s: %d\n", fname, -errno);
        return;
    }

    //write the file at once
    bytes = fwrite(strfile.c_str(), 1, strfile.length(), fp);
    fclose(fp);
    if (bytes == strfile.length()) {
        real = realpath();
        remove(real.c_str());
        ret = rename(fname, real.c_str());
        if (0 != ret) {
            printf("ERROR: failed to rename tmp file %s to real file %s\n",
                   fname, real.c_str());
        }
    } else {
        printf("ERROR: failed to write contents. length=%d, written=%u\n",
               strfile.length(), bytes);
    }
}

std::string Keystore::get(const char* szkey)
{
    //convert to std::string
    std::string key = szkey;

    //get value
    return get(key);
}

void Keystore::set(const char* szkey, const char* szvalue)
{
    //convert to std::string
    std::string key = szkey;
    std::string val = szvalue;

    //set value
    return set(key,val);
}

std::string Keystore::get(std::string& strkey)
{
    //default return value
    string strreturn = "";

    //does the key exist?
    if (_mapdb.find(strkey) != _mapdb.end()) {
        //if so get it
        strreturn = _mapdb[strkey];
    }

    //return the val
    return strreturn;
};

void Keystore::del(const char* key)
{
    //convert to std::string
    std::string strkey = key;

    //delete it
    del(strkey);
}

void Keystore::del(std::string& key)
{
    //does this key exist?
    if(exists(key)) {
        //if so delete it
        _mapdb.erase(key);
    }
}

bool Keystore::exists(const char* szkey)
{
    //convert to std::string
    std::string key = szkey;

    //get exists
    return exists(key);
}

bool Keystore::exists(std::string& strkey)
{
    //default return
    bool bexist = false;

    //check if the key exists
    if (_mapdb.find(strkey) != _mapdb.end()) {
        bexist = true;
    }

    return bexist;
}

void Keystore::set(const std::string& strkey, const std::string& strvalue)
{
    //set the given key to the given value
    _mapdb[strkey] = strvalue;
};

void Keystore::to_db(std::string& strfile)
{
    //lines of the file
    vector<std::string> vlines;

    //crack into lines
    tokenize(strfile, vlines, '\n');

    //walk the lines
    for (unsigned int n = 0; n < vlines.size(); n++) {
        std::vector<string> values;

        //if the line isn't empty
        if (vlines[n] != "") {
            //crack into name/value pairs
            tokenize(vlines[n], values, '=');

            //put in our map
            _mapdb[values[0]] = values[1];
        }
    }

    return;
};

vector<std::string> Keystore::keys()
{
    vector<std::string> vkeys;

    //our iterator
    std::map<std::string, std::string>::iterator iter;

    //walk the keys
    for (iter = _mapdb.begin(); iter != _mapdb.end(); ++iter) {
        //add to return
        vkeys.push_back(iter->first);
    }

    return vkeys;
}

std::string Keystore::to_file()
{
    //the file writeable format to return
    std::string strfile;

    //get all the keys to convert
    vector<std::string> vKeys = keys();

    //walk the keys
    for (unsigned int n = 0; n < vKeys.size(); n++) {
        //convert to file format and add the line
        strfile += vKeys[n] + "=" + _mapdb[vKeys[n]]+"\n";
    }

    return strfile;
};

void Keystore::tokenize(string& strin, vector<string>& lsresult, char token)
{
    //our temp string builder
    std::string strtemp;

    //lets walk our string and crack it!
    for (unsigned int n = 0; n < strin.length(); n++) {
        //do we have our delimiter?
        if (strin[n] == token) {
            //add it to our list
            lsresult.push_back(strtemp);
            strtemp = "";
        } else {
            //if not add to our string!
            strtemp += strin[n];
        }
    }

    //add our last string
    lsresult.push_back(strtemp);
}
