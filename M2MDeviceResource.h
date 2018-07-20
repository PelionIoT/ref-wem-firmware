#ifndef _DEVICE_RESOURCE_1_H_
#define _DEVICE_RESOURCE_1_H_
#include <string>

#include <m2mresource.h>

class M2MDeviceResource : public DeviceResource {
private:
	M2MResource *res;

public:

 	M2MDeviceResource(M2MResource *source_res)
 	{
 		res=source_res;
 	}

    std::string resource_type() {
    	 m2m::String m = res->resource_type();
    	 std::string s(m.c_str());
    	 return s;

    	//return "Hello1";
    };

    std::string get_value_string() {
    	 m2m::String m = res->get_value_string();
    	 std::string s(m.c_str());
    	 return s;

    	//return "Hello2";
    };
};

#endif
