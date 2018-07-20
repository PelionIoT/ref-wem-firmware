
#ifndef _DEVICE_RESOURCE_H_
#define _DEVICE_RESOURCE_H_

// Abstract class
class DeviceResource {
public:
    virtual std::string resource_type() =0;
    virtual std::string get_value_string() =0;
};

#endif
