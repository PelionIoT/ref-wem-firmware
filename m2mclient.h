//----------------------------------------------------------------------------
// The confidential and proprietary information contained in this file may
// only be used by a person authorised under and to the extent permitted
// by a subsisting licensing agreement from ARM Limited or its affiliates.
//
// (C) COPYRIGHT 2016 ARM Limited or its affiliates.
// ALL RIGHTS RESERVED
//
// This entire notice must be reproduced on all copies of this file
// and copies of this file may only be made by a person if such person is
// permitted to do so under the terms of a subsisting license agreement
// from ARM Limited or its affiliates.
//----------------------------------------------------------------------------

#ifndef M2MCLIENT_H
#define M2MCLIENT_H

#include "m2mresources.h"
#include "update_ui_example.h"

#include <MbedCloudClient.h>
#include <m2mdevice.h>
#include <NetworkInterface.h>

#include <stdio.h>

class M2MClient {

public:
    M2MClient() :
        _registered(false),
        _register_called(false),
        _exec_resource(_cloud_client) {
    }

    void create_resources() {
        _obj_list.push_back(_observable_resource.get_object());
        _obj_list.push_back(_exec_resource.get_object());
        _obj_list.push_back(_writable_resource.get_object());

        _cloud_client.add_objects(_obj_list);
        _cloud_client.on_registered(this, &M2MClient::client_registered);
        _cloud_client.on_unregistered(this, &M2MClient::client_unregistered);
        _cloud_client.on_error(this, &M2MClient::error);
        _cloud_client.set_update_callback(&_writable_resource);
    }

    bool call_register(NetworkInterface *iface) {
        bool setup = _cloud_client.setup(iface);
        _register_called = true;
        if (!setup) {
            printf("m2m client setup failed\n");
            return false;
        }

        /* Set callback functions for authorizing updates and monitoring
         * progress.  Code is implemented in update_ui_example.cpp.
         * Both callbacks are completely optional. If no authorization callback
         * is set, the update process will proceed immediately in each step.
         */
        update_ui_set_cloud_client(&_cloud_client);
        _cloud_client.set_update_authorize_handler(update_authorize);
        _cloud_client.set_update_progress_handler(update_progress);
        return true;
    }

    void close() {
        _cloud_client.close();
    }

    void keep_alive() {
        _cloud_client.keep_alive();
    }

    void client_registered() {
        _registered = true;
        printf("\nClient registered\n\n");
        static const ConnectorClientEndpointInfo* endpoint = NULL;
        if (endpoint == NULL) {
            endpoint = _cloud_client.endpoint_info();
            if (endpoint) {
                printf("Cloud Client: Ready");
                printf("Internal Endpoint Name: %s\r\n",
                       endpoint->internal_endpoint_name.c_str());
                printf("Endpoint Name: %s\r\n",
                       endpoint->endpoint_name.c_str());
                printf("Device Id: %s\r\n",
                       endpoint->internal_endpoint_name.c_str());
                printf("Account Id: %s\r\n", endpoint->account_id.c_str());
                printf("Security Mode (-1=not set, 0=psk, 1=<undef>, 2=cert, 3=none): %d\r\n",
                       endpoint->mode);
            }
        }
    }

    void client_unregistered() {
        _registered = false;
        _register_called = false;
        printf("\nClient unregistered - Exiting application\n\n");

    }

    void error(int error_code) {
        const char *error;
        switch(error_code) {
            case MbedCloudClient::ConnectErrorNone:
                error = "MbedCloudClient::ConnectErrorNone";
                break;
            case MbedCloudClient::ConnectAlreadyExists:
                error = "MbedCloudClient::ConnectAlreadyExists";
                break;
            case MbedCloudClient::ConnectBootstrapFailed:
                error = "MbedCloudClient::ConnectBootstrapFailed";
                break;
            case MbedCloudClient::ConnectInvalidParameters:
                error = "MbedCloudClient::ConnectInvalidParameters";
                break;
            case MbedCloudClient::ConnectNotRegistered:
                error = "MbedCloudClient::ConnectNotRegistered";
                break;
            case MbedCloudClient::ConnectTimeout:
                error = "MbedCloudClient::ConnectTimeout";
                break;
            case MbedCloudClient::ConnectNetworkError:
                error = "MbedCloudClient::ConnectNetworkError";
                break;
            case MbedCloudClient::ConnectResponseParseFailed:
                error = "MbedCloudClient::ConnectResponseParseFailed";
                break;
            case MbedCloudClient::ConnectUnknownError:
                error = "MbedCloudClient::ConnectUnknownError";
                break;
            case MbedCloudClient::ConnectMemoryConnectFail:
                error = "MbedCloudClient::ConnectMemoryConnectFail";
                break;
            case MbedCloudClient::ConnectNotAllowed:
                error = "MbedCloudClient::ConnectNotAllowed";
                break;
            case MbedCloudClient::ConnectSecureConnectionFailed:
                error = "MbedCloudClient::ConnectSecureConnectionFailed";
                break;
            case MbedCloudClient::ConnectDnsResolvingFailed:
                error = "MbedCloudClient::ConnectDnsResolvingFailed";
                break;
            case MbedCloudClient::UpdateWarningCertificateNotFound:
                error = "MbedCloudClient::UpdateWarningCertificateNotFound";
                break;
            case MbedCloudClient::UpdateWarningIdentityNotFound:
                error = "MbedCloudClient::UpdateWarningIdentityNotFound";
                break;
            case MbedCloudClient::UpdateWarningCertificateInvalid:
                error = "MbedCloudClient::UpdateWarningCertificateInvalid";
                break;
            case MbedCloudClient::UpdateWarningSignatureInvalid:
                error = "MbedCloudClient::UpdateWarningSignatureInvalid";
                break;
            case MbedCloudClient::UpdateWarningVendorMismatch:
                error = "MbedCloudClient::UpdateWarningVendorMismatch";
                break;
            case MbedCloudClient::UpdateWarningClassMismatch:
                error = "MbedCloudClient::UpdateWarningClassMismatch";
                break;
            case MbedCloudClient::UpdateWarningDeviceMismatch:
                error = "MbedCloudClient::UpdateWarningDeviceMismatch";
                break;
            case MbedCloudClient::UpdateWarningURINotFound:
                error = "MbedCloudClient::UpdateWarningURINotFound";
                break;
            case MbedCloudClient::UpdateWarningRollbackProtection:
                error = "MbedCloudClient::UpdateWarningRollbackProtection";
                break;
            case MbedCloudClient::UpdateWarningUnknown:
                error = "MbedCloudClient::UpdateWarningUnknown";
                break;
            case MbedCloudClient::UpdateErrorWriteToStorage:
                error = "MbedCloudClient::UpdateErrorWriteToStorage";
                break;
            default:
                error = "UNKNOWN";
        }
        printf("Error occured : %s\r\n", error);
        printf("Error code : %d\r\n", error_code);
        printf("Error details : %s\r\n",_cloud_client.error_description());
    }

    bool is_client_registered() {
        return _registered;
    }

    bool is_register_called() {
        return _register_called;
    }
    void increment_resource_value() {
        if(_registered) {
            _observable_resource.increment_resource();
        }
    }

    MbedCloudClient& get_cloud_client() {
        return _cloud_client;
    }

private:
    M2MObjectList       _obj_list;
    MbedCloudClient     _cloud_client;
    bool                _registered;
    bool                _register_called;
    ObservableResource  _observable_resource;
    ExecutableResource  _exec_resource;
    WritableResource    _writable_resource;
};

#endif /* M2MCLIENT_H */
