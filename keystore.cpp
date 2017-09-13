#include "keystore.h"

#include <pal.h>
#include <pal_plat_fileSystem.h>
#include <pal_plat_rtos.h>
#include <SDBlockDevice.h>
#include <FATFileSystem.h>

using namespace std;

#define KEYSTORE_SUBDIR "keystore"
#define KEYSTORE_FILENAME "keystore.data"

string Keystore::_strdir;
string Keystore::_strfilepath;

extern SDBlockDevice sd(MBED_CONF_SD_SPI_MOSI,
                        MBED_CONF_SD_SPI_MISO,
                        MBED_CONF_SD_SPI_CLK,
                        MBED_CONF_SD_SPI_CS);
FATFileSystem fs("sd", &sd);

Keystore::Keystore()
{
}

Keystore::~Keystore()
{
    //do cleanup
}

int Keystore::init()
{
    int ret;
    palStatus_t pal_status;
    char mount_path[PAL_MAX_FOLDER_DEPTH_CHAR] = { 0 };

    ret = sd.init();
    if (ret != BD_ERROR_OK) {
        return ret;
    }

    pal_status = pal_fsGetMountPoint(PAL_FS_PARTITION_PRIMARY,
                                     sizeof(mount_path),
                                     mount_path);
    if (pal_status != PAL_SUCCESS) {
        return pal_status;
    }

    Keystore::_strdir = mount_path;
    Keystore::_strdir += "/";
    Keystore::_strdir += KEYSTORE_SUBDIR;

    pal_status = pal_fsMkDir(Keystore::_strdir.c_str());
    if ((pal_status != PAL_SUCCESS)
        && (pal_status != PAL_ERR_FS_NAME_ALREADY_EXIST)) {
        return pal_status;
    }

    Keystore::_strfilepath = Keystore::_strdir + "/" + KEYSTORE_FILENAME;

    return 0;
}

void Keystore::kill_all()
{
    pal_fsRmFiles(Keystore::_strdir.c_str());
}

void Keystore::open()
{
    //read the file into this
    std::string strfile;

    //our file*
    palFileDescriptor_t fd = (uintptr_t)NULL;

    //file operation status
    palStatus_t rt = PAL_SUCCESS;

    //open our file
    rt = pal_fsFopen(Keystore::_strfilepath.c_str(), PAL_FS_FLAG_READONLY, &fd);

    //if it worked
    if(rt == PAL_SUCCESS) {
        //buffer to read into
        char buffer[64];

        //bytes read on last pass
        size_t bytes_read = 0;

        //start reading
        for (;;) {

            //clear our buffer
            memset(buffer, 0, sizeof(buffer));

            //Read a chunk of the database
            rt = pal_fsFread(&fd, buffer, sizeof(buffer) - 1, &bytes_read);

            //are we done reading?
            if (rt || bytes_read == 0) {
                //convert the file to the internal state
                to_db(strfile);

                //exit the loop
                break;
            }

            //append the read to the file string
            strfile += buffer;
        }
    }

    //close the file
    pal_fsFclose(&fd);
};

void Keystore::close()
{
    //convert the database to file writable string
    std::string strfile = to_file();

    //our file pointer
    palFileDescriptor_t fd = (uintptr_t)NULL;

    //file operation status
    palStatus_t rt = PAL_SUCCESS;

    //open the file
    rt = pal_fsFopen(Keystore::_strfilepath.c_str(),
                     PAL_FS_FLAG_READWRITETRUNC,
                     &fd);

    //if we succed
    if (rt == PAL_SUCCESS) {
        //bytes written
        size_t numberOfBytesWritten = 0;

        //write the file at once
        rt = pal_fsFwrite(&fd,
                          strfile.c_str(),
                          strfile.length(),
                          &numberOfBytesWritten);
    }

    //close the file
    pal_fsFclose(&fd);

};

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

