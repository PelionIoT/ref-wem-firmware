#include "mbed.h"
#include "rtos.h"
#include "MQTTThreadedClient.h"
#include "mbedtls/platform.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"

#define QUEUE_SIZE 2
static MemoryPool<MQTT::PubMessage, 2* QUEUE_SIZE> mpool; // 2nd arg is the number of elements in pool
static Queue<MQTT::PubMessage, QUEUE_SIZE> mqueue;     // 2nd arg is the number of elements in queue

//https://os.mbed.com/docs/v5.8/mbed-os-api-doxy/classrtos_1_1_queue.html


// SSL/TLS variables
mbedtls_entropy_context _entropy;
mbedtls_ctr_drbg_context _ctr_drbg;
mbedtls_x509_crt _cacert;
mbedtls_x509_crt _clientcert;
mbedtls_pk_context _pkey;
mbedtls_ssl_context _ssl;
mbedtls_ssl_config _ssl_conf;    
mbedtls_ssl_session saved_session;

bool _debug = false;

namespace MQTT {
    
/**
 * Receive callback for mbed TLS
 */
static int ssl_recv(void *ctx, unsigned char *buf, size_t len)
{
    int recv = -1;
    TCPSocket *socket = static_cast<TCPSocket *>(ctx);
    socket->set_timeout(DEFAULT_SOCKET_TIMEOUT);
    recv = socket->recv(buf, len);

    if (NSAPI_ERROR_WOULD_BLOCK == recv) {
        return MBEDTLS_ERR_SSL_WANT_READ;
    } else if (recv < 0) {
        return -1;
    } else {
        return recv;
    }
}

/**
 * Send callback for mbed TLS
 */
static int ssl_send(void *ctx, const unsigned char *buf, size_t len)
{
    int sent = -1;
    TCPSocket *socket = static_cast<TCPSocket *>(ctx);
    socket->set_timeout(DEFAULT_SOCKET_TIMEOUT);    
    sent = socket->send(buf, len);

    if(NSAPI_ERROR_WOULD_BLOCK == sent) {
        return MBEDTLS_ERR_SSL_WANT_WRITE;
    } else if (sent < 0) {
        return -1;
    } else {
        return sent;
    }
}

#if DEBUG_LEVEL > 0
/**
 * Debug callback for mbed TLS
 * Just prints on the USB serial port
 */
static void my_debug(void *ctx, int level, const char *file, int line,
                     const char *str)
{
    const char *p, *basename;
    (void) ctx;

    /* Extract basename from file */
    for(p = basename = file; *p != '\0'; p++) {
        if(*p == '/' || *p == '\\') {
            basename = p + 1;
        }
    }

    if (_debug) {
        mbedtls_printf("%s:%04d: |%d| %s", basename, line, level, str);
    }
}

/**
 * Certificate verification callback for mbed TLS
 * Here we only use it to display information on each cert in the chain
 */
static int my_verify(void *data, mbedtls_x509_crt *crt, int depth, uint32_t *flags)
{
    const uint32_t buf_size = 1024;
    char *buf = new char[buf_size];
    (void) data;

    if (_debug) mbedtls_printf("\nVerifying certificate at depth %d:\r\n", depth);
    mbedtls_x509_crt_info(buf, buf_size - 1, "  ", crt);
    if (_debug) mbedtls_printf("%s", buf);

    if (*flags == 0) {
        if (_debug) mbedtls_printf("No verification issue for this certificate\r\n");
     }   else {
            mbedtls_x509_crt_verify_info(buf, buf_size, "  ! ", *flags);
            if (_debug) mbedtls_printf("%s\n", buf);
        }

    delete[] buf;
    return 0;
}
#endif


void MQTTThreadedClient::setupTLS()    
{
        if (useTLS)
        {
            mbedtls_entropy_init(&_entropy);
            mbedtls_ctr_drbg_init(&_ctr_drbg);
            mbedtls_x509_crt_init(&_cacert);
            mbedtls_x509_crt_init(&_clientcert); 
            mbedtls_pk_init(&_pkey ); 
            mbedtls_ssl_init(&_ssl);
            mbedtls_ssl_config_init(&_ssl_conf);        
            memset( &saved_session, 0, sizeof( mbedtls_ssl_session ) );            
        }    
}

void MQTTThreadedClient::freeTLS()
{
        if (useTLS)
        {
            mbedtls_entropy_free(&_entropy);
            mbedtls_ctr_drbg_free(&_ctr_drbg);
            mbedtls_x509_crt_free(&_cacert);
            mbedtls_x509_crt_free(&_clientcert);
            mbedtls_pk_free(&_pkey);
            mbedtls_ssl_free(&_ssl);
            mbedtls_ssl_config_free(&_ssl_conf);               
        }    
}

int MQTTThreadedClient::initTLS()
{
        int ret;
        size_t len;

        DBG("MQTTThreadedClient::initTLS() ...\r\n");
        DBG("1)-->  mbedtls_ctr_drdbg_seed ...\r\n");
        const char *DRBG_PERS = "mbed TLS MQTT client"; //TODO - is it better to use the MAC address here?
        // https://tls.mbed.org/api/ctr__drbg_8h.html#ad93d675f998550b4478c1fe6f4f34ebc
        if ((ret = mbedtls_ctr_drbg_seed(&_ctr_drbg, mbedtls_entropy_func, &_entropy,
                          (const unsigned char *) DRBG_PERS,
                          sizeof (DRBG_PERS))) != 0) {
            mbedtls_printf("mbedtls_crt_drbg_init returned [%x]\r\n", ret);
            _error = ret;
            return -1;
        }

        if (isDERformat) {
          DBG("2)-->DER mbedtls_x509_crt_parse ca cert ...\r\n");

          //len = strlen((const char*)ssl_ca);
          len = ssl_ca_len; // 569; //sizeof(ssl_ca);
          mbedtls_printf("len=%d \r\n", len);

          if ((ret = mbedtls_x509_crt_parse_der(&_cacert, (const unsigned char *) ssl_ca,
                           len + 1)) != 0) {
            mbedtls_printf("ERROR mbedtls_x509_crt_parse_der() ca cert returned [%x]\r\n", ret);
            mbedtls_printf("%s \r\n", (const unsigned char *) ssl_ca);
            _error = ret;
            return -1;
          }

          DBG("3)-->DER mbedtls_x509_crt_parse client cert ...\r\n");
          len = ssl_client_cert_len; // strlen((const char*)ssl_client_cert);
          if ((ret = mbedtls_x509_crt_parse_der(&_clientcert, (const unsigned char *) ssl_client_cert,
                           len + 1)) != 0) {
            mbedtls_printf("ERROR mbedtls_x509_crt_parse_der() client cert returned [%x]\r\n", ret);
            mbedtls_printf("%s \r\n", (const unsigned char *) ssl_client_cert);
            _error = ret;
            return -1;
          }

        } else {  // PEM

          DBG("2)-->PEM mbedtls_x509_crt_parse ca cert ...\r\n");
          len = strlen((const char*)ssl_ca);
          if ((ret = mbedtls_x509_crt_parse(&_cacert, (const unsigned char *) ssl_ca,
                           len + 1 )) != 0) {
            mbedtls_printf("mbedtls_x509_crt_parse ca cert returned [%x]\r\n", ret);
            mbedtls_printf("%s \r\n", (const unsigned char *) ssl_ca);
            _error = ret;
            return -1;
          }

          DBG("3)-->PEM mbedtls_x509_crt_parse client cert ...\r\n");
          len = strlen((const char*)ssl_client_cert);
          if ((ret = mbedtls_x509_crt_parse(&_clientcert, (const unsigned char *) ssl_client_cert,
                           len + 1)) != 0) {
            mbedtls_printf("mbedtls_x509_crt_parse client cert returned [%x]\r\n", ret);
            mbedtls_printf("%s \r\n", (const unsigned char *) ssl_client_cert);
            _error = ret;
            return -1;
          }

        }  // end of PEM

        DBG("4)-->PEM or DER  mbedtls_pk_parse_key ...\r\n");
        if (isDERformat)
            len = ssl_client_pkey_len;
        else
            len = strlen((const char*)ssl_client_pkey);

        if ((ret = mbedtls_pk_parse_key(&_pkey, (const unsigned char *) ssl_client_pkey,
                           len + 1, NULL, 0)) != 0) {
            mbedtls_printf("mbedtls_pk_parse_key returned [%x]\r\n", ret);
            mbedtls_printf("%s \r\n", (const unsigned char *) ssl_client_pkey);
            _error = ret;
            return -1;
        }


        DBG("5) --> mbedtls_ssl_config_defaults ...\r\n");
        if ((ret = mbedtls_ssl_config_defaults(&_ssl_conf,
                        MBEDTLS_SSL_IS_CLIENT,
                        MBEDTLS_SSL_TRANSPORT_STREAM,
                        MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
            mbedtls_printf("mbedtls_ssl_config_defaults returned [%x]\r\n", ret);
            _error = ret;
            return -1;
        }

        DBG("mbedtls_ssl_config_ca_chain ...\r\n");
        mbedtls_ssl_conf_ca_chain(&_ssl_conf, &_cacert, NULL);
        DBG("mbedtls_ssl_conf_rng ...\r\n");
        mbedtls_ssl_conf_rng(&_ssl_conf, mbedtls_ctr_drbg_random, &_ctr_drbg);


        mbedtls_ssl_conf_ca_chain(&_ssl_conf, &_cacert, NULL);
    
        ret = mbedtls_ssl_conf_own_cert( 
            &_ssl_conf,   //SSL conf
            &_clientcert,    //own public cert chain
            &_pkey        //own private key
        );
        if (ret != 0) {
            mbedtls_printf("mbedtls_ssl_conf_own_cert() returned -0x%04X\n", -ret);
            return ret;
        }

        /* It is possible to disable authentication by passing
         * MBEDTLS_SSL_VERIFY_NONE in the call to mbedtls_ssl_conf_authmode()
         */
        DBG("mbedtls_ssl_conf_authmode ...\r\n");         
        mbedtls_ssl_conf_authmode(&_ssl_conf, MBEDTLS_SSL_VERIFY_REQUIRED);

#if DEBUG_LEVEL > 0
        mbedtls_ssl_conf_verify(&_ssl_conf, my_verify, NULL);
        mbedtls_ssl_conf_dbg(&_ssl_conf, my_debug, NULL);
        mbedtls_debug_set_threshold(DEBUG_LEVEL);
#endif

        DBG("mbedtls_ssl_setup ...\r\n");         
        if ((ret = mbedtls_ssl_setup(&_ssl, &_ssl_conf)) != 0) {
            mbedtls_printf("mbedtls_ssl_setup returned [%x]\r\n", ret);
            _error = ret;
            return -1;
        }
        
        return 0;
}

int MQTTThreadedClient::doTLSHandshake()
{
        int ret;
        
        /* Start the handshake, the rest will be done in onReceive() */
        printf("Starting the TLS handshake...\r\n");
        ret = mbedtls_ssl_handshake(&_ssl);
        if (ret < 0) 
        {
            if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
                ret != MBEDTLS_ERR_SSL_WANT_WRITE) 
                    mbedtls_printf("mbedtls_ssl_handshake returned [%x]\r\n", ret);
            else 
            {
                // do not close the socket if timed out
                ret = TIMEOUT;
            }
            return ret;
        }

        /* Handshake done, time to print info */
        printf("TLS connection to %s:%d established\r\n", 
            host.c_str(), port);

        const uint32_t buf_size = 1024;
        char *buf = new char[buf_size];
        mbedtls_x509_crt_info(buf, buf_size, "\r    ",
                        mbedtls_ssl_get_peer_cert(&_ssl));
                        
        printf("Server certificate:\r\n%s\r", buf);
        // Verify server cert ...
        uint32_t flags = mbedtls_ssl_get_verify_result(&_ssl);
        if( flags != 0 )
        {
            mbedtls_x509_crt_verify_info(buf, buf_size, "\r  ! ", flags);
            printf("Certificate verification failed:\r\n%s\r\r\n", buf);
            // free server cert ... before error return
            delete [] buf;
            return -1;
        }
        
        printf("Certificate verification passed\r\n\r\n");
        // delete server cert after verification
        delete [] buf;
        
#if defined(MBEDTLS_SSL_CLI_C)        
				printf("Saving SSL/TLS session ...\r\n");
        // TODO: Save the session here for reconnect.
        if( ( ret = mbedtls_ssl_get_session( &_ssl, &saved_session ) ) != 0 )
        {
            mbedtls_printf( "mbedtls_ssl_get_session returned -0x%x\n\n", -ret );
            hasSavedSession = false;
            return -1;
        }  
        printf("Session saved for reconnect ...\r\n");				
#endif        
     
        hasSavedSession = true;
        
        return 0;
}

int MQTTThreadedClient::readBytesToBuffer(char * buffer, size_t size, int timeout)
{
    int rc;

    if (tcpSocket == NULL)
        return -1;

    if (useTLS) 
    {
        // Do SSL/TLS read
        rc = mbedtls_ssl_read(&_ssl, (unsigned char *) buffer, size);
        if (MBEDTLS_ERR_SSL_WANT_READ == rc)
            return TIMEOUT;
        else
            return rc;
    } else {
        // non-blocking socket ...
        tcpSocket->set_timeout(timeout);
        rc = tcpSocket->recv( (void *) buffer, size);

        // return 0 bytes if timeout ...
        if (NSAPI_ERROR_WOULD_BLOCK == rc)
            return TIMEOUT;
        else
            return rc; // return the number of bytes received or error
    }
}

int MQTTThreadedClient::sendBytesFromBuffer(char * buffer, size_t size, int timeout)
{
    int rc;
    
    if (tcpSocket == NULL)
        return -1;
    
    if (useTLS) {
        // Do SSL/TLS write
        rc =  mbedtls_ssl_write(&_ssl, (const unsigned char *) buffer, size);
        if (MBEDTLS_ERR_SSL_WANT_WRITE == rc)
            return TIMEOUT;
        else
            return rc;
    } else {

        // set the write timeout
        tcpSocket->set_timeout(timeout);
        rc = tcpSocket->send(buffer, size);

        if ( NSAPI_ERROR_WOULD_BLOCK == rc)
            return TIMEOUT;
        else
            return rc;
    }
}

int MQTTThreadedClient::readPacketLength(int* value)
{
    int rc = MQTTPACKET_READ_ERROR;
    unsigned char c;
    int multiplier = 1;
    int len = 0;
    const int MAX_NO_OF_REMAINING_LENGTH_BYTES = 4;

    *value = 0;
    do
    {
        if (++len > MAX_NO_OF_REMAINING_LENGTH_BYTES)
        {
            rc = MQTTPACKET_READ_ERROR; /* bad data */
            goto exit;
        }
        
        rc = readBytesToBuffer((char *) &c, 1, DEFAULT_SOCKET_TIMEOUT);
        if (rc != 1)
        {
            rc = MQTTPACKET_READ_ERROR;
            goto exit;
        }
            
        *value += (c & 127) * multiplier;
        multiplier *= 128;
    } while ((c & 128) != 0);
    
    rc = MQTTPACKET_READ_COMPLETE;
        
exit:
    if (rc == MQTTPACKET_READ_ERROR )
        len = -1;
    
    return len;
}

int MQTTThreadedClient::sendPacket(size_t length)
{
    int rc = FAILURE;
    size_t sent = 0;

    while (sent < length)
    {
        rc = sendBytesFromBuffer((char *) &sendbuf[sent], length - sent, DEFAULT_SOCKET_TIMEOUT);
        if (rc < 0)  // there was an error writing the data
            break;
        sent += rc;
    }
    
    if (sent == length)
        rc = SUCCESS;
    else
        rc = FAILURE;
        
    return rc;
}
/**
 * Reads the entire packet to readbuf and returns
 * the type of packet when successful, otherwise
 * a negative error code is returned.
 **/
int MQTTThreadedClient::readPacket()
{
    int rc = FAILURE;
    MQTTHeader header = {0};
    int len = 0;
    int rem_len = 0;

    /* 1. read the header byte.  This has the packet type in it */
    if ( (rc = readBytesToBuffer((char *) &readbuf[0], 1, DEFAULT_SOCKET_TIMEOUT)) != 1)
        goto exit;

    len = 1;
    /* 2. read the remaining length.  This is variable in itself */
    if ( readPacketLength(&rem_len) < 0 )
        goto exit;
        
    len += MQTTPacket_encode(readbuf + 1, rem_len); /* put the original remaining length into the buffer */

    if (rem_len > (MAX_MQTT_PACKET_SIZE - len))
    {
        rc = BUFFER_OVERFLOW;
        goto exit;
    }

    /* 3. read the rest of the buffer using a callback to supply the rest of the data */
    if (rem_len > 0 && (readBytesToBuffer((char *) (readbuf + len), rem_len, DEFAULT_SOCKET_TIMEOUT) != rem_len))
        goto exit;

    // Convert the header to type
    // and update rc
    header.byte = readbuf[0];
    rc = header.bits.type;
    
exit:

    return rc;    
}

/**
 * Read until a specified packet type is received, or untill the specified
 * timeout dropping packets along the way.
 **/
int MQTTThreadedClient::readUntil(int packetType, int timeout)
{
    int pType = FAILURE;
    Timer timer;
    
    timer.start();
    do {
        pType = readPacket();
        if (pType < 0)
            break;
            
        if (timer.read_ms() > timeout)
        {
            pType = FAILURE;
            break;
        }
    }while(pType != packetType);
    
    return pType;    
}


int MQTTThreadedClient::login()
{
    int rc = FAILURE;
    int len = 0;

    if (!isConnected)
    {
        DBG("Session not connected! \r\n");
        return rc;
    }
        
    // Copy the keepAliveInterval value to local
    // MQTT specifies in seconds, we have to multiply that
    // amount for our 32 bit timers which accepts ms.
    keepAliveInterval = (connect_options.keepAliveInterval * 1000);
    

    
    if ((len = MQTTSerialize_connect(sendbuf, MAX_MQTT_PACKET_SIZE, &connect_options)) <= 0)
    {
        DBG("Error serializing connect packet ...\r\n");
        return rc;
    }
    if ((rc = sendPacket((size_t) len)) != SUCCESS)  // send the connect packet
    {
        DBG("Error sending the connect request packet ...\r\n");
        return rc; 
    }
    
    // Wait for the CONNACK 
    if (readUntil(CONNACK, COMMAND_TIMEOUT) == CONNACK)
    {
        unsigned char connack_rc = 255;
        bool sessionPresent = false;
        DBG("Connection acknowledgement received ... deserializing respones ...\r\n");
        if (MQTTDeserialize_connack((unsigned char*)&sessionPresent, &connack_rc, readbuf, MAX_MQTT_PACKET_SIZE) == 1)
            rc = connack_rc;
        else
            rc = FAILURE;
    }
    else
        rc = FAILURE;

    if (rc == SUCCESS)
    {
        DBG("Connected!!! ... starting connection timers ...\r\n");
        resetConnectionTimer();
    }
    
    DBG("Returning with rc = %d\r\n", rc);
    
    return rc;    
}


void MQTTThreadedClient::disconnect()
{
    if (isConnected)
    {
        if( useTLS 
            && ( mbedtls_ssl_session_reset( &_ssl ) != 0 )
           )
        {
            DBG( "disconnect(): Session reset returned an error \r\n");
        }        
        
        isConnected = false;
        tcpSocket->close();      
    }
}

int MQTTThreadedClient::connect()
{
    int ret = FAILURE;

    if (network == NULL) 
    {
        DBG("ERROR Network=NULL \r\n");
        return ret;
    }

    if (tcpSocket == NULL) 
    {
        DBG("ERROR  tcpSocket == NULL \r\n");
        return ret;
    }

    if (host.empty())
    {
        DBG("ERROR  host.empty() \r\n");
        return ret;
    }
    
    if (useTLS) 
    {
        if( ( ret = mbedtls_ssl_session_reset( &_ssl ) ) != 0 ) {
            mbedtls_printf( "connect() failed\n  ! mbedtls_ssl_session_reset returned -0x%x\n\n", -ret );
            return ret;
        }
#if defined(MBEDTLS_SSL_CLI_C)
        if ( hasSavedSession && (( ret = mbedtls_ssl_set_session( &_ssl, &saved_session ) ) != 0 )) {
            mbedtls_printf( "connect() failed\n  ! mbedtls_ssl_conf_session returned %d\n\n", ret );
            return ret;
        }
#endif        
    }
        
    tcpSocket->open(network);
    if (useTLS)
    {
        DBG("connect() mbedtls_ssl_set_hostname ...\r\n");         
        mbedtls_ssl_set_hostname(&_ssl, host.c_str());
        DBG("connect() mbedtls_ssl_set_bio ...\r\n");         
        mbedtls_ssl_set_bio(&_ssl, static_cast<void *>(tcpSocket),
                                   ssl_send, ssl_recv, NULL );
    }
    
    DBG("connect() attempting socket connect ...\r\n");         
    
    if (( ret = tcpSocket->connect(host.c_str(), port)) < 0 )
    {
         DBG("connect() Error connecting to %s:%d with %d\r\n", host.c_str(), port, ret);
         return ret;
    } else
         isConnected = true;
    DBG("connect() done socket connect ...\r\n");         
    
    if (useTLS) 
    {
        
        if (doTLSHandshake() < 0)
        {
            DBG("connect() TLS Handshake failed! \r\n");
            return FAILURE;
        }else
            DBG("connect() TLS Handshake complete!! \r\n");
    }
    
    return login();
}

void MQTTThreadedClient::setConnectionParameters(const char * chost, uint16_t cport, MQTTPacket_connectData & options)
{
    // Copy the settings for reconnection
    host = chost;
    port = cport;
    connect_options = options;    
}

int MQTTThreadedClient::publish(PubMessage& msg)
{
#if 0
    int id = queue.call(mbed::callback(this, &MQTTThreadedClient::sendPublish), topic, message);
    // TODO: handle id values when the function is called later
    if (id == 0)
        return FAILURE;
    else
        return SUCCESS;
#endif
    PubMessage *message = mpool.alloc();
    if (message == NULL){
      printf("====ERROR===== cannot allocate the object on mpool !!! \r\n");
      return -100;
    }
    // Simple copy
    *message = msg;
    
    // Push the data to the thread, wait and retry if queue is full
    int counter=0;
    while (mqueue.full() && (counter < 10)){
        printf ("The message queue is full - let wait and retry %d \r\n", counter);
        Thread::wait(1000);
        counter++;
    }
    if (mqueue.full()) {
         printf ("The message queue is full - give up on publishing\r\n ");
         return -200;
    }

    //DBG("Pushing data to consumer thread ... %d\r\n", mqueue.full());
    int ret = mqueue.put(message);
    if (ret) printf("Return status from put: %d\r\n", ret);
    return ret;
}

int MQTTThreadedClient::sendPublish(PubMessage& message)
{
     MQTTString topicString = MQTTString_initializer;
     
     if (!isConnected) 
     {
        DBG("Not connected!!! ...\r\n");
        return FAILURE;
     }
        
     topicString.cstring = (char*) &message.topic[0];
     // printf("BEFORE MQTTSerialize_publish: strlen(sendbuf)=%d  MAX_MQTT_PACKET_SIZE=%d \r\n", strlen((const char*)sendbuf), MAX_MQTT_PACKET_SIZE);
     // printf("BEFORE MQTTSerialize_publish: sendbuf = %s \r\n", sendbuf);
     DBG("BEFORE MQTTSerialize_publish: msg.payload = %s \r\n", message.payload);
     DBG("BEFORE MQTTSerialize_publish: strlen(msg.payload) = %d \r\n", strlen((const char*)message.payload));

     int len = MQTTSerialize_publish(sendbuf, MAX_MQTT_PACKET_SIZE, 0, message.qos, false, message.id,
              topicString, (unsigned char*) &message.payload[0], (int) message.payloadlen);
     if (len <= 0)
     {
         DBG("ERROR after MQTTSerialize_publish: Failed serializing message ...\r\n");
         return FAILURE;
     }
     
     if (sendPacket(len) == SUCCESS)
     {
         //DBG("Successfully published: topic=%s message=%s \r\n", (char*) &message.topic[0], (char*) &message.payload[0]);
         return SUCCESS;
     }
    
    DBG("Failed to send publish packet to server ...\r\n");
    return FAILURE;
}

void MQTTThreadedClient::addTopicHandler(const char * topicstr, void (*function)(MessageData &))
{
    // Push the subscription into the map ...
    F_P<void,MessageData &> fp;
    fp.attach(function);
    
    topicCBMap.insert(std::pair<std::string, F_P<void,MessageData &> >(std::string(topicstr),fp));    
} 

int MQTTThreadedClient::handlePublishMsg()
{
    MQTTString topicName = MQTTString_initializer;
    Message msg;
    int intQoS;
    DBG("Deserializing publish message ...\r\n");
    if (MQTTDeserialize_publish((unsigned char*)&msg.dup, 
            &intQoS, 
            (unsigned char*)&msg.retained, 
            (unsigned short*)&msg.id, 
            &topicName,
            (unsigned char**)&msg.payload, 
            (int*)&msg.payloadlen, readbuf, MAX_MQTT_PACKET_SIZE) != 1)
    {
        DBG("Error deserializing published message ...\r\n");
        return -1;
    }

    std::string topic;
    if (topicName.lenstring.len > 0)
    {
        topic = std::string((const char *) topicName.lenstring.data, (size_t) topicName.lenstring.len);
    }else
        topic = (const char *) topicName.cstring;
    
    DBG("Got message for topic [%s], QoS [%d] ...\r\n", topic.c_str(), intQoS);
    
    msg.qos = (QoS) intQoS;

    
    // Call the handlers for each topic 
    if (topicCBMap.find(topic) != topicCBMap.end())
    {
        // Call the callback function 
        if (topicCBMap[topic].attached())
        {
            DBG("Invoking function handler for topic ...\r\n");
            MessageData md(topicName, msg);            
            topicCBMap[topic](md);
            
            return 1;
        }
    }
    
    // TODO: depending on the QoS
    // we send data to the server = PUBACK or PUBREC
    switch(intQoS)
    {
        case QOS0:
            // We send back nothing ...
            break;
        case QOS1:
            // TODO: implement
            break;
        case QOS2:
            // TODO: implement
            break;
        default:
            break;
    }
    
    return 0;
}

void MQTTThreadedClient::resetConnectionTimer()
{
    if (keepAliveInterval > 0)
    {
        comTimer.reset();
        comTimer.start();
    }
}

bool MQTTThreadedClient::hasConnectionTimedOut()
{
    if (keepAliveInterval > 0 ) {
        // Check connection timer
        if (  (unsigned int)(comTimer.read_ms()) > keepAliveInterval)
            return true;
        else
            return false;
    }

    return false;
}
        
void MQTTThreadedClient::sendPingRequest()
{
    int len = MQTTSerialize_pingreq(sendbuf, MAX_MQTT_PACKET_SIZE);
    if (len > 0 && (sendPacket(len) == SUCCESS)) // send the ping packet
    {
        DBG("Ping request sent successfully ...\r\n");
    }
}

void MQTTThreadedClient::startListener()
{



    mbedtls_printf(" startListener() \r\n ");

    int pType;

    // Continuesly listens for packets and dispatch
    // message handlers ...
    if (useTLS)
    {
        initTLS();
    }
     
     
    while(true)
    {

        mbedtls_printf("startListener(): Attempting connect \r\n ");

        // Attempt to reconnect and login
        if ( connect() < 0 )
        {
            disconnect();
            // Wait for a few secs and reconnect ...
            Thread::wait(6000); //6000 millisec = 6 sec
            tcpSocket = new TCPSocket(network);
            continue;
        }

        mbedtls_printf("startListener(): Done connect\r\n");
         
        // loop read    
        while(true) 
        {
            pType = readPacket();
            switch(pType) 
            {
                case TIMEOUT:
                    // No data available from the network ...
                    break;
                case FAILURE:
                    {
                        DBG("readPacket returned failure \r\n");
                        goto reconnect;
                    }
                case BUFFER_OVERFLOW: 
                    {
                        // TODO: Network error, do we disconnect and reconnect?
                        DBG("Failure or buffer overflow problem ... \r\n");
                        MBED_ASSERT(false);
                    }
                    break;
                /**
                *  The rest of the return codes below (all positive) is about MQTT
                 * response codes
                 **/
                case CONNACK:
                case PUBACK:
                case SUBACK:
                    break;
                case PUBLISH: 
                    {
                        DBG("Publish received!....\r\n");
                        // We receive data from the MQTT server ..
                        if (handlePublishMsg() < 0) {
                            DBG("Error handling PUBLISH message ... \r\n");
                            break;
                        }
                    }
                    break;
                case PINGRESP: 
                    {
                        DBG("Got ping response ...\r\n");
                        resetConnectionTimer();
                    }
                    break;
                default:
                    DBG("Unknown/Not handled message from server pType[%d]\r\n", pType);
            }

            // Check if its time to send a keepAlive packet
            if (hasConnectionTimedOut()) {
                // Queue the ping request so that other
                // pending operations queued above will go first
                queue.call(this, &MQTTThreadedClient::sendPingRequest);
            }

             // Check if we have messages on the message queue
             osEvent evt = mqueue.get(10);   //the argument is timeout in millisec
             if (evt.status == osEventMessage) {

                 DBG("Got message to publish! ... \r\n");

                 // Unpack the message
                 PubMessage * message = (PubMessage *)evt.value.p;

                 // Send the packet, do not queue the call
                 // like the ping above ..
                 if ( sendPublish(*message) == SUCCESS) {
                     // Reset timers if we have been able to send successfully
                     resetConnectionTimer();
                 } else {
                     // Disconnected?
                     goto reconnect;
                 }

                 // Free the message from mempool  after using
                 mpool.free(message);
             }

            // // Dispatch any queued events ...
             queue.dispatch(100); //TODO - what it is??



        } // end while loop

reconnect:
        // reconnect?
        DBG("startListener() Client disconnected!! ... retrying ...\r\n");
        disconnect();
        
    }; //end of while(true)
}

void MQTTThreadedClient::stopListener()
{
    // TODO: Set a signal/flag that the running thread 
    // will check if its ok to stop ...
}

}
