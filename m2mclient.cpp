#include "m2mclient.h"

std::string M2MClient::get_resource_value_str(M2MResource *res)
{
    return res->get_value_string().c_str();
}

std::string M2MClient::get_resource_value_str(enum M2MClientResource resource)
{
    M2MResource *res;

    res = get_resource(resource);
    if (NULL == res) {
        return "";
    }

    return get_resource_value_str(res);
}

void M2MClient::set_resource_value(M2MResource *res,
                                   const char *val,
                                   size_t len)
{
    res->set_value((const uint8_t *)val, len);
}

void M2MClient::set_resource_value(enum M2MClientResource resource,
                                   const char *val,
                                   size_t len)
{
    M2MResource *res;

    res = get_resource(resource);
    if (NULL == res) {
        return;
    }

    set_resource_value(res, val, len);
}

void M2MClient::set_resource_value(enum M2MClientResource resource,
                                   const std::string &val)
{
    M2MResource *res;

    res = get_resource(resource);
    if (NULL == res) {
        return;
    }

    set_resource_value(res, val.c_str(), val.length());
}

void M2MClient::register_objects()
{
    M2MObject *obj;
    M2MResource *res;
    M2MObjectList obj_list;
    std::map<std::string, struct resource_entry>::iterator it;

    for (it = _res_map.begin(); it != _res_map.end(); ++it) {
        res = it->second.res;
        obj = &res->get_parent_object_instance().get_parent_object();
        obj_list.push_back(obj);
    }

    _cloud_client.add_objects(obj_list);
}

void M2MClient::add_resource(M2MResource *res, enum M2MClientResource type)
{
    struct resource_entry entry = {res, type};
    _res_map[res->uri_path()] = entry;
}

#ifdef DEBUG
void print_object_list(M2MObjectList &obj_list)
{
    M2MObjectList::const_iterator obj_it;
    M2MObjectInstanceList::const_iterator obj_inst_it;
    M2MResourceList::const_iterator res_it;
    M2MResourceInstanceList::const_iterator res_inst_it;

    for (obj_it = obj_list.begin(); obj_it != obj_list.end(); obj_it++) {

        M2MObject *obj = (*obj_it);
        printf("object path: %s\n", obj->uri_path());
        for (obj_inst_it = obj->instances().begin();
             obj_inst_it != obj->instances().end();
             obj_inst_it++) {

            M2MObjectInstance *obj_inst = (*obj_inst_it);
            printf("object instance path: %s\n", obj_inst->uri_path());
            for (res_it = obj_inst->resources().begin();
                 res_it != obj_inst->resources().end();
                 res_it++) {

                 M2MResource *res = (*res_it);
                 printf("resource path: %s\n", res->uri_path());
                 for (res_inst_it = res->resource_instances().begin();
                      res_inst_it != res->resource_instances().end();
                      res_inst_it++) {

                      M2MResourceInstance *res_inst = (*res_inst_it);
                      printf("resource instance path: %s\n",
                             res_inst->uri_path());
                  }
             }
         }
     }
}
#endif

struct M2MClient::resource_entry *
M2MClient::get_resource_entry(const char *uri_path)
{
    std::map<std::string, struct resource_entry>::iterator it;

    it = _res_map.find(uri_path);
    if (it == _res_map.end()) {
        return NULL; 
    }

    return &it->second;
}

M2MResource *M2MClient::get_resource(const char *uri_path)
{
    struct resource_entry *entry;

    entry = get_resource_entry(uri_path);
    if (NULL == entry) {
        return NULL;
    }
    return entry->res;
}

struct M2MClient::resource_entry *
M2MClient::get_resource_entry(enum M2MClientResource type)
{
    struct resource_entry *entry;
    std::map<std::string, struct resource_entry>::iterator it;

    if (type >= M2MClientResourceCount) {
        return NULL;
    }

    entry = NULL;
    for (it = _res_map.begin(); it != _res_map.end(); ++it) {
        entry = &it->second;
        if (type == entry->type) {
            break;
        }
        entry = NULL;
    }
    return entry;
}

M2MResource *M2MClient::get_resource(enum M2MClientResource resource)
{
    struct resource_entry *entry;

    entry = get_resource_entry(resource);
    if (NULL == entry) {
        return NULL;
    }
    return entry->res;
}

void M2MClient::add_light_sensor()
{
    M2MObject *obj;
    M2MResource *res;
    M2MObjectInstance *inst;

    obj = M2MInterfaceFactory::create_object("3301");
    inst = obj->create_object_instance();
    res = inst->create_dynamic_resource("1", "light_resource",
                                        M2MResourceInstance::FLOAT,
                                        true /* observable */);
    res->set_operation(M2MBase::GET_ALLOWED);
    add_resource(res, M2MClientResourceLightSensor);
}

void M2MClient::add_temp_sensor()
{
    M2MObject *obj;
    M2MResource *res;
    M2MObjectInstance *inst;

    obj = M2MInterfaceFactory::create_object("3303");
    inst = obj->create_object_instance();
    res = inst->create_dynamic_resource("1", "temperature_resource",
                                        M2MResourceInstance::FLOAT,
                                        true /* observable */);
    res->set_operation(M2MBase::GET_ALLOWED);
    add_resource(res, M2MClientResourceTempSensor);
}

void M2MClient::add_humidity_sensor()
{
    M2MObject *obj;
    M2MResource *res;
    M2MObjectInstance *inst;

    obj = M2MInterfaceFactory::create_object("3304");
    inst = obj->create_object_instance();
    res = inst->create_dynamic_resource("1", "humidity_resource",
                                        M2MResourceInstance::FLOAT,
                                        true /* observable */);
    res->set_operation(M2MBase::GET_ALLOWED);
    add_resource(res, M2MClientResourceHumiditySensor);
}

int M2MClient::add_sensor_resources()
{
    add_light_sensor();
    add_temp_sensor();
    add_humidity_sensor();
    return 0;
}
