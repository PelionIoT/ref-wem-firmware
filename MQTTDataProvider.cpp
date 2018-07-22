#include <cstdio>

#include "mbed.h"
#include "rtos.h"
#undef MBED_CONF_APP_ESP8266_DEBUG
#include "EthernetInterface.h"
#include "MQTTThreadedClient.h"
#include "mbedtls/platform.h"
#include "MQTTDataProvider.h"
#include <pal.h>

using namespace MQTT;

#define MBED_CLOUD_CERT

#ifdef MBED_CLOUD_CERT
       #include "mbed_cloud_dev_credentials.c"
       bool isDER = true;
       static const char * topic_1 = "topic/test";
       const char* hostname = "ingest.mqtt.data.mbedcloudintegration.net";
#elif defined(LOCAL_CERT)
      #include "local_mqtt_conf.h"
      bool isDER=false;
#endif

int arrivedcount = 0;

void messageArrived(MessageData& md)
{
    Message &message = md.message;
    printf("Arrived Callback 1 : qos %d, retained %d, dup %d, packetid %d\r\n", message.qos, message.retained, message.dup, message.id);
    printf("Payload [%.*s]\r\n", message.payloadlen, (char*)message.payload);
    ++arrivedcount;
}

string MQTTDataProvider::getData() {

   //returns JSON as described here: https://confluence.arm.com/display/IoTBU/Message+Structure
   char str_time[32];
   uint64_t currTimeSeconds = pal_osGetTime();
   long long int currTimeMillis = currTimeSeconds * 1000;
   sprintf(str_time, "%lld", currTimeMillis);

   string json="{\"f\": \"1\",";
   json += "\"id\": \"";
   json += deviceId;
   json += "\",";
   json += "\"d\": [";

   size_t j=0; // resource counter - it is used to print or not to print the comma after 
   for( std::map<string,DeviceResource*>::const_iterator it = resources.begin(); it != resources.end(); ++it )
   {
      //printf ("%d=>  str_resource key=%s  value=%s \r\n",j,(it->first).c_str(), (it->second)->get_value_string().c_str());

      j++;
      json += "{";
      json += "\"";
      json += it->first;   //resource_path
      json += "\": [";
      json += "{";
      json += "\"t\": ";
      json +=str_time;
      json += ",";

      json += "\"v\": {";

      json += "\"";
      json += (it->second)->resource_type();
      json += "\":";
      json += "\"";
      json += (it->second)->get_value_string();
      json += "\"";
      json += "}";
      json += "}";
      json += "]";

      json += "}";
      if (resources.size() > j) json += ",";
         json += "";

   }

    json += "]}";

    // printf(" ===> END getData() counter=%d \r\n", i);
    return json;
}

void MQTTDataProvider::run(NetworkInterface *network){

    Thread msgSender(osPriorityNormal); // there are optional args: stack_size, etc

    //NetworkInterface* network = easy_connect(true); //use if no Mbed Cloud Client
    //NetworkInterface* network = easy_get_netif(true); // already created by Mbed Cloud Client
    // do not create new connection, because it will raise the error:
    // error NSAPI_ERROR_IS_CONNECTED  -3015 socket is already connected (error code is defined in nsapi_types.h)
    if (!network) {
        mbedtls_printf("===ERROR=== easy_get_netif inside MQTTDataProvider::run \r\n");
        return ;
    }

    int port = 8883;
    // int port = 1883;

#ifdef MBED_CLOUD_CERT
      // DER format
      MQTTThreadedClient mqtt(network, (const unsigned char*)(MBED_CLOUD_DEV_LWM2M_SERVER_ROOT_CA_CERTIFICATE), (const unsigned char*)MBED_CLOUD_DEV_BOOTSTRAP_DEVICE_CERTIFICATE, (const unsigned char*)MBED_CLOUD_DEV_BOOTSTRAP_DEVICE_PRIVATE_KEY, isDER);
      // MQTTThreadedClient mqtt(network, (const unsigned char*)(NULL), (const unsigned char*)MBED_CLOUD_DEV_BOOTSTRAP_DEVICE_CERTIFICATE, (const unsigned char*)MBED_CLOUD_DEV_BOOTSTRAP_DEVICE_PRIVATE_KEY, isDER);

      mqtt.ssl_ca_len          = sizeof(MBED_CLOUD_DEV_LWM2M_SERVER_ROOT_CA_CERTIFICATE);
      mqtt.ssl_client_cert_len = sizeof(MBED_CLOUD_DEV_BOOTSTRAP_DEVICE_CERTIFICATE);
      mqtt.ssl_client_pkey_len = sizeof(MBED_CLOUD_DEV_BOOTSTRAP_DEVICE_PRIVATE_KEY);

#else
      // PEM format
      MQTTThreadedClient mqtt(network, (const unsigned char*)TLS_CA_PEM, (const unsigned char*)TLS_CLIENT_CERT, (const unsigned char*)TLS_CLIENT_PKEY, isDER);
#endif

    MQTTPacket_connectData logindata = MQTTPacket_connectData_initializer;
    logindata.MQTTVersion = 3;


    logindata.clientID.cstring = (char *)  deviceId;

    mqtt.setConnectionParameters(hostname, port, logindata);
    mqtt.addTopicHandler(topic_1, messageArrived);

    msgSender.start(mbed::callback(&mqtt, &MQTTThreadedClient::startListener));


    while(true)
    {
         Thread::wait(1 * 2 * 1000);
         PubMessage message;
         message.qos = QOS0;
         message.id = 123;

         // if (i > 3) continue;  // This is temporary statement to concentrate on TLS handshake issue

         strcpy(&message.topic[0], topic_1);


         string json=getData();  //temporary commented to concentrate on TLS handshake issue

         if  (json.length() >= MAX_MQTT_PAYLOAD_SIZE){
            printf("ERROR json lengh > %d  \r\n", MAX_MQTT_PAYLOAD_SIZE);
            break;
         }

         sprintf(&message.payload[0], "%s", json.c_str());
         printf("sending payload to topic=%s payload=%s \r\n", &message.topic[0],   &message.payload[0] );

         message.payloadlen = strlen((const char *) &message.payload[0]);
         int ret = mqtt.publish(message);
         if (ret) printf("ERROR mqtt.publish() ret=%d  ", ret);
         if (ret) Thread::wait(6000);
     }

}
