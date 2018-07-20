#ifndef _MQTT_THREADED_CLIENT_H_
#define _MQTT_THREADED_CLIENT_H_

#include "mbed.h"
#include "rtos.h"
#include "MQTTPacket.h"
#include "NetworkInterface.h"
#include "FP.h"
#include "mbedtls/debug.h"

#define MQTT_DEBUG 1

#define DEBUG_LEVEL 0

#ifdef MQTT_DEBUG
#define DBG(fmt, args...)    printf(fmt, ## args)
#else
#define DBG(fmt, args...)    /* Don't do anything in release builds */
#endif

#include <cstdio>
#include <string>
#include <map>

#define COMMAND_TIMEOUT 5000
#define DEFAULT_SOCKET_TIMEOUT 1000
#define MAX_MQTT_PACKET_SIZE 500
#define MAX_MQTT_PAYLOAD_SIZE 1000

namespace MQTT
{
    
typedef enum { QOS0, QOS1, QOS2 } QoS;

// all failure return codes must be negative
typedef enum { BUFFER_OVERFLOW = -3, TIMEOUT = -2, FAILURE = -1, SUCCESS = 0 } returnCode;


typedef struct
{
    QoS qos;
    bool retained;
    bool dup;
    unsigned short id;
    void *payload;
    size_t payloadlen;
}Message, *pMessage;

// TODO:
// Merge this struct with the one above, in order to use the same
// data structure for sending and receiving. I need to simplify
// the PubMessage to not contain pointers like the one above.
typedef struct
{
    char topic[100];
    QoS qos;
    unsigned short id;
    size_t payloadlen;
    char payload[MAX_MQTT_PAYLOAD_SIZE];
}PubMessage, *pPubMessage;

struct MessageData
{
    MessageData(MQTTString &aTopicName, Message &aMessage)  : message(aMessage), topicName(aTopicName)
    { }
    Message &message;
    MQTTString &topicName;
};

class PacketId
{
public:
    PacketId()
    {
        next = 0;
    }

    int getNext()
    {
        //return next = (next == MAX_PACKET_ID) ? 1 : ++next;
        if (next == MAX_PACKET_ID)
            next = 1;
        else
            next++;

        return next;
    }

private:
    static const int MAX_PACKET_ID = 65535;
    int next;
};



class MQTTThreadedClient
{
public:
    MQTTThreadedClient(NetworkInterface * aNetwork, const unsigned char * ca = NULL, const unsigned char * clientCert = NULL, const unsigned char * clientPkey = NULL, bool isDER = false)
        : network(aNetwork),
          ssl_ca(ca),
          ssl_client_cert(clientCert),
          ssl_client_pkey(clientPkey),
          port((ca != NULL) ? 8883 : 1883),
          queue(32 * EVENTS_EVENT_SIZE),                //TODO: Hardcoded 32  
          isConnected(false),          
          hasSavedSession(false),
          isDERformat(isDER),
          useTLS(ca != NULL)
    {
        tcpSocket = new TCPSocket();
        setupTLS();
    }
    
    ~MQTTThreadedClient()
    {
        // TODO: signal the thread to shutdown
        freeTLS();
           
        if (isConnected)
            disconnect();
                
    }
    /** 
     *  Sets the connection parameters. Must be called before running the startListener as a thread.
     *
     *  @param host - pointer to the host where the MQTT server is running
     *  @param port - the port number to connect, 1883 for non secure connections, 8883 for 
     *                secure connections
     *  @param options - the connect data used for logging into the MQTT server.
     */
    void setConnectionParameters(const char * host, uint16_t port, MQTTPacket_connectData & options);
    int publish(PubMessage& message);
    

    void addTopicHandler(const char * topic, void (*function)(MessageData &));

    void startListener();
    
    void stopListener();

    size_t ssl_ca_len;
    size_t ssl_client_cert_len;
    size_t ssl_client_pkey_len;

protected:

    int handlePublishMsg();
    void disconnect();  
    int connect();      


private:
    NetworkInterface * network;
    const unsigned char * ssl_ca;
    const unsigned char * ssl_client_cert;
    const unsigned char * ssl_client_pkey;



    TCPSocket * tcpSocket;
    PacketId packetid;
    //const char *DRBG_PERS;
    nsapi_error_t _error;    
    // Connection options
    std::string host;
    uint16_t port;
    MQTTPacket_connectData connect_options;
    // Event queue
    EventQueue queue;
    bool isConnected;
    bool hasSavedSession;    
    bool isDERformat;
    
    // TODO: Because I'm using a map, I can only have one handler
    // for each topic (one that's mapped to the topic string).
    // Attaching another handler on the same topic is not possible.
    // In the future, use a vector instead of maps to allow multiple
    // handlers for the same topic.
    std::map<std::string, F_P<void, MessageData &> > topicCBMap;
    
    unsigned char sendbuf[MAX_MQTT_PACKET_SIZE];
    unsigned char readbuf[MAX_MQTT_PACKET_SIZE];

    unsigned int keepAliveInterval;
    Timer comTimer;

    // SSL/TLS functions
    bool useTLS;
    void setupTLS();
    int initTLS();    
    void freeTLS();
    int doTLSHandshake();
    
    //int processSubscriptions();
    int readPacket();
    int sendPacket(size_t length);
    int readPacketLength(int* value);
    int readUntil(int packetType, int timeout);
    int readBytesToBuffer(char * buffer, size_t size, int timeout);
    int sendBytesFromBuffer(char * buffer, size_t size, int timeout);
//    bool isTopicMatched(char* topic, MQTTString& topicName);
    int  sendPublish(PubMessage& message);
    void resetConnectionTimer();
    void sendPingRequest();
    bool hasConnectionTimedOut();
    int login();
};

}
#endif
