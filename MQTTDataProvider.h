#ifndef _MQTT_DATA_PROVIDER_H_
#define _MQTT_DATA_PROVIDER_H_

#include <string>
#include <map>

#include "DeviceResource.h"

class MQTTDataProvider{
 public:
     MQTTDataProvider( const char* aDeviceId,
     	               map<std::string, DeviceResource*>  aResources
                     ):
            deviceId(aDeviceId),
            resources(aResources)
       {
       }

    ~MQTTDataProvider(){}

    void run(NetworkInterface *net);
    std::string getData(int counter); //returns JSON as described here: https://confluence.arm.com/display/IoTBU/Message+Structure
    std::string getDataOld(int counter); //returns JSON in format used in 1st demo with plotting
    void publish_data(std::string key, std::string value);

    const char* deviceId;
    map<std::string, DeviceResource*> resources;

};

#endif
