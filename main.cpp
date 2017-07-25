// ****************************************************************************
//  Firmware Over The Air (FOTA) demo
//
//  This application demonstrates how to perform fota using mbed cloud 1.2.
//
//  By the ARM Reference Design (Red) Team
// ****************************************************************************
#include "m2mclient.h"

#include "commander.h"
#include "DHT.h"
#include "displayman.h"
#include "GL5528.h"
#include "keystore.h"
#include "lcdprogress.h"

#include <SDBlockDevice.h>
#include <errno.h>
#include <factory_configurator_client.h>
#include <mbed-trace-helper.h>
#include <mbed-trace/mbed_trace.h>

#if MBED_CONF_APP_WIFI
#include <ESP8266Interface.h>
#else
#include <EthernetInterface.h>
#endif

#define TRACE_GROUP "main"

// Convert the value of a C macro to a string that can be printed.  This trick
// is straight out of the GNU C documentation.
// (https://gcc.gnu.org/onlinedocs/gcc-4.9.0/cpp/Stringification.html)
#define xstr(s) str(s)
#define str(s) #s

#ifndef DEVTAG
#error "No dev tag created"
#endif

// ****************************************************************************
// DEFINEs and type definitions
// ****************************************************************************
#define MACADDR_STRLEN 18

#define SSID_KEY "wifi.ssid"
#define PASSWORD_KEY "wifi.key"
#define SECURITY_KEY "wifi.encryption"

enum FOTA_THREADS {
    FOTA_THREAD_DISPLAY = 0,
    FOTA_THREAD_SENSOR_LIGHT,
    FOTA_THREAD_DHT,
    FOTA_THREAD_COUNT
};

struct dht_sensor {
    uint8_t h_id;
    uint8_t t_id;

    DHT *dev;

    M2MObject *h_obj;
    M2MObject *t_obj;

    M2MResource *h_res;
    M2MResource *t_res;

    M2MObjectInstance *h_inst;
    M2MObjectInstance *t_inst;
};

struct light_sensor {
    uint8_t id;

    AnalogIn *dev;

    M2MObject *obj;
    M2MObjectInstance *inst;
    M2MResource *res;
};

struct sensors {
    int event_queue_id;
    struct dht_sensor dht;
    struct light_sensor light;
};

// ****************************************************************************
// Globals
// ****************************************************************************
/* declared in pal_plat_fileSystem.cpp, which is included because COMMON_PAL
 * is defined in mbed_app.json */
extern SDBlockDevice sd;
static DisplayMan display;
static M2MClient *m2mclient;
static NetworkInterface *net;
static EventQueue evq;
static struct sensors sensors;
/* used to stop auto display refresh during firmware downloads */
static int display_evq_id;

// ****************************************************************************
// Generic Helpers
// ****************************************************************************
/**
 * Processes an event queue callback for updating the display
 */
static void display_refresh(DisplayMan *display)
{
    display->refresh();
}

// ****************************************************************************
// Sensors
// ****************************************************************************
/**
 * Inits the light sensor object
 */
static void light_init(struct light_sensor *s, M2MClient *mbed_client)
{
    /* add to the display */
    s->id = display.register_sensor("Light");

    /* init the driver */
    s->dev = new AnalogIn(A0);

    /* register the m2m object */
    s->obj = M2MInterfaceFactory::create_object("3301");
    s->inst = s->obj->create_object_instance();

    s->res = s->inst->create_dynamic_resource("1", "light_resource",
                                              M2MResourceInstance::FLOAT,
                                              true /* observable */);
    s->res->set_operation(M2MBase::GET_ALLOWED);
    s->res->set_value((uint8_t *)"0", 1);

    mbed_client->add_resource(s->obj);
}

/**
 * Reads a value from the light sensor and publishes to the display
 */
static void light_read(struct light_sensor *s)
{
    int size = 0;
    uint8_t res_buffer[33] = {0};

    float flux = s->dev->read();

    size = sprintf((char *)res_buffer, "%2.2f", flux);

    display.set_sensor_status(s->id, (char *)res_buffer);
    s->res->set_value(res_buffer, size);
}

/**
 * Inits the temp/humidity combo sensor
 */
static void dht_init(struct dht_sensor *s, M2MClient *mbed_client)
{
    /* add to the display */
    s->t_id = display.register_sensor("Temp");
    s->h_id = display.register_sensor("Humidity");

    /* init the driver */
    s->dev = new DHT(D4, AM2302);

    /* register the m2m temperature object */
    s->t_obj = M2MInterfaceFactory::create_object("3303");
    s->t_inst = s->t_obj->create_object_instance();

    s->t_res = s->t_inst->create_dynamic_resource("1", "temperature_resource",
                                                  M2MResourceInstance::FLOAT,
                                                  true /* observable */);
    s->t_res->set_operation(M2MBase::GET_ALLOWED);
    s->t_res->set_value((uint8_t *)"0", 1);

    mbed_client->add_resource(s->t_obj);

    /* register the m2m humidity object */
    s->h_obj = M2MInterfaceFactory::create_object("3304");
    s->h_inst = s->h_obj->create_object_instance();

    s->h_res = s->h_inst->create_dynamic_resource("1", "humidity_resource",
                                                  M2MResourceInstance::FLOAT,
                                                  true /* observable */);
    s->h_res->set_operation(M2MBase::GET_ALLOWED);
    s->h_res->set_value((uint8_t *)"0", 1);

    mbed_client->add_resource(s->h_obj);
}

/**
 * Reads temp and humidity values publishes to the display
 */
static void dht_read(struct dht_sensor *dht)
{
    int size = 0;
    eError readError;
    float temperature, humidity;
    uint8_t res_buffer[33] = {0};

    readError = dht->dev->readData();
    if (readError == ERROR_NONE) {
        temperature = dht->dev->ReadTemperature(CELCIUS);
        humidity = dht->dev->ReadHumidity();
        tr_debug("DHT: temp = %fC, humi = %f%%\n", temperature, humidity);

        size = sprintf((char *)res_buffer, "%.1f", temperature);
        dht->t_res->set_value(res_buffer, size);
        display.set_sensor_status(dht->t_id, (char *)res_buffer);

        size = sprintf((char *)res_buffer, "%.0f", humidity);
        dht->h_res->set_value(res_buffer, size);
        display.set_sensor_status(dht->h_id, (char *)res_buffer);
    } else {
        tr_error("DHT: readData() failed with %d\n", readError);
    }
}

/**
 * Processes an event queue callback for reading sensor data
 */
static void sensors_read(void *context)
{
    struct sensors *s = (struct sensors *)context;
    dht_read(&s->dht);
    light_read(&s->light);
}

/**
 * Inits all sensors, making them ready to read
 */
static void sensors_init(struct sensors *sensors, M2MClient *mbed_client)
{
    dht_init(&sensors->dht, mbed_client);
    light_init(&sensors->light, mbed_client);
}

/**
 * Starts the periodic sampling of sensor data
 */
static void sensors_start(struct sensors *s, EventQueue *q)
{
    printf("starting all sensors\n");
    s->event_queue_id = q->call_every(5000, sensors_read, s);
}

/**
 * Stops the periodic sampling of sensor data
 */
static void sensors_stop(struct sensors *s, EventQueue *q)
{
    printf("stopping all sensors\n");
    q->cancel(s->event_queue_id);
    s->event_queue_id = 0;
}

// ****************************************************************************
// Network
// ****************************************************************************
static void network_disconnect(NetworkInterface *net) { net->disconnect(); }

static char *network_get_macaddr(NetworkInterface *net, char *macstr)
{
    memcpy(macstr, net->get_mac_address(), MACADDR_STRLEN);
    return macstr;
}

#if MBED_CONF_APP_WIFI
static nsapi_security_t wifi_security_str2sec(const char *security)
{
    if (0 == strcmp("WPA/WPA2", security)) {
        return NSAPI_SECURITY_WPA_WPA2;

    } else if (0 == strcmp("WPA2", security)) {
        return NSAPI_SECURITY_WPA2;

    } else if (0 == strcmp("WPA", security)) {
        return NSAPI_SECURITY_WPA;

    } else if (0 == strcmp("WEP", security)) {
        return NSAPI_SECURITY_WEP;

    } else if (0 == strcmp("NONE", security)) {
        return NSAPI_SECURITY_NONE;

    } else if (0 == strcmp("OPEN", security)) {
        return NSAPI_SECURITY_NONE;
    }

    printf("warning: unknown wifi security type (%s), assuming NONE\n",
           security);
    return NSAPI_SECURITY_NONE;
}

/**
 * brings up wifi
 * */
static NetworkInterface *network_create(void)
{
    Keystore k;
    string ssid;

    ssid = MBED_CONF_APP_WIFI_SSID;

    k.open();
    if (k.exists("wifi.ssid")) {
        ssid = k.get("wifi.ssid");
    }
    k.close();

    display.init_network("WiFi");
    display.set_network_ssid(ssid);

    return new ESP8266Interface(MBED_CONF_APP_WIFI_TX, MBED_CONF_APP_WIFI_RX,
                                MBED_CONF_APP_WIFI_DEBUG);
}

static int network_connect(NetworkInterface *net)
{
    int ret;
    char macaddr[MACADDR_STRLEN];
    ESP8266Interface *wifi;

    /* code is compiled -fno-rtti so we have to use C cast */
    wifi = (ESP8266Interface *)net;

    //wifi login info set to default values
    string ssid     = MBED_CONF_APP_WIFI_SSID;
    string pass     = MBED_CONF_APP_WIFI_PASSWORD;
    string security = MBED_CONF_APP_WIFI_SECURITY;

    //keystore db access
    Keystore k;

    //read the current state
    k.open();

    //use the keystore for ssid?
    if (k.exists(SSID_KEY)) {
        printf("Using SSID from keystore.\r\n");

        ssid = k.get(SSID_KEY);
    } else {
        printf("Using default %s.\r\n", SSID_KEY);
    }

    //use the keystore for pass?
    if (k.exists(PASSWORD_KEY)) {
        printf("Using pass from keystore.\r\n");

        pass = k.get(PASSWORD_KEY);
    } else {
        printf("Using default %s.\r\n", PASSWORD_KEY);
    }

    //use the keystor for security?
    if (k.exists(SECURITY_KEY)) {
        printf("Using security from keystore.\r\n");

        security = k.get(SECURITY_KEY);
    } else {
        printf("Using default %s.\r\n", SECURITY_KEY);
    }

    display.set_network_ssid(ssid);
    printf("[WIFI] connecting: ssid=%s, mac=%s\n",
           ssid.c_str(), network_get_macaddr(wifi, macaddr));

    ret = wifi->connect(ssid.c_str(),
                        pass.c_str(),
                        wifi_security_str2sec(security.c_str()));
    if (0 != ret) {
        printf("[WIFI] Failed to connect to: %s (%d)\n",
               ssid.c_str(), ret);
        return ret;
    }

    printf("[WIFI] connected: ssid=%s, mac=%s, ip=%s, netmask=%s, gateway=%s\n",
           ssid.c_str(),
           network_get_macaddr(net, macaddr),
           net->get_ip_address(),
           net->get_netmask(),
           net->get_gateway());

    return 0;
}
#else
/**
 * brings up Ethernet
 * */
static NetworkInterface *network_create(void)
{
    display.init_network("Eth");
    return new EthernetInterface();
}

static int network_connect(NetworkInterface *net)
{
    int ret;
    char macaddr[MACADDR_STRLEN];

    /* note: Ethernet MAC isn't available until *after* a call to
     * EthernetInterface::connect(), so the first time we attempt to
     * connect this will print a NULL mac, but will work after a retry */
    printf("[ETH] obtaining IP address: mac=%s\n",
           network_get_macaddr(net, macaddr));
    ret = net->connect();
    if (0 != ret) {
        printf("ERROR: [ETH] Failed to connect! %d\n", ret);
        return ret;
    }
    printf("[ETH] connected: mac%s, ip=%s, netmask=%s, gateway=%s\n",
           network_get_macaddr(net, macaddr), net->get_ip_address(),
           net->get_netmask(), net->get_gateway());

    return ret;
}
#endif

// ****************************************************************************
// Cloud
// ****************************************************************************
/**
 * Readies the app for a firmware download
 */
void fota_auth_download(M2MClient *mbed_client)
{
    printf("Firmware download requested\r\n");

    sensors_stop(&sensors, &evq);
    /* we'll need to manually refresh the display until the firmware
     * update is complete.  it seems that doing *anything* outside of
     * the firmware download's thread context will result in a failed
     * download. */
    evq.cancel(display_evq_id);
    display_evq_id = 0;
    display.set_downloading();
    display.refresh();
    mbed_client->update_authorize(MbedCloudClient::UpdateRequestDownload);

    printf("Authorization granted\r\n");
}

/**
 * Readies the app for a firmware install
 */
void fota_auth_install(M2MClient *mbed_client)
{
    printf("Firmware install requested\r\n");

    display.set_installing();
    /* firmware download is complete, restart the auto display updates */
    display_evq_id = evq.call_every(250, display_refresh, &display);

    printf("Disconnecting network...\n");
    network_disconnect(net);

    mbed_client->update_authorize(MbedCloudClient::UpdateRequestInstall);
    printf("Authorization granted\r\n");
}

/**
 * Handles authorization requests from the mbed firmware updater
 */
void mbed_client_on_update_authorize(int32_t request)
{
    switch (request) {
        /* Cloud Client wishes to download new firmware. This can have a
         * negative impact on the performance of the rest of the system.
         *
         * The user application is supposed to pause performance sensitive tasks
         * before authorizing the download.
         *
         * Note: the authorization call can be postponed and called later.
         * This doesn't affect the performance of the Cloud Client.
         * */
        case MbedCloudClient::UpdateRequestDownload:
            evq.call(fota_auth_download, m2mclient);
            break;

        /* Cloud Client wishes to reboot and apply the new firmware.
         *
         * The user application is supposed to save all current work before
         * rebooting.
         *
         * Note: the authorization call can be postponed and called later.
         * This doesn't affect the performance of the Cloud Client.
         * */
        case MbedCloudClient::UpdateRequestInstall:
            evq.call(fota_auth_install, m2mclient);
            break;

        default:
            printf("ERROR: unknown request\r\n");
            led_set_color(IND_FWUP, IND_COLOR_FAILED);
            led_post();
            break;
    }
}

/**
 * Handles progress updates from the mbed firmware updater
 */
void mbed_client_on_update_progress(uint32_t progress, uint32_t total)
{
    uint32_t percent = progress * 100 / total;
    static uint32_t last_percent = 0;
    const char dl_message[] = "Downloading...";
    const char done_message[] = "Saving...";

    display.set_progress(dl_message, progress, total);

    if (last_percent < percent) {
        printf("Downloading: %lu\n", percent);
    }

    if (progress == total) {
        printf("\r\nDownload completed\r\n");
        display.set_progress(done_message, 0, 100);
        display.set_download_complete();
    }

    display.refresh();
    last_percent = percent;
}

static void mbed_client_on_registered(void *context)
{
    printf("mbed client registered\n");
    display.set_cloud_registered();
}

static void mbed_client_on_unregistered(void *context)
{
    printf("mbed client unregistered\n");
    display.set_cloud_unregistered();
}

static void mbed_client_on_error(void *context, int err_code,
                                 const char *err_name, const char *err_desc)
{
    printf("ERROR: mbed client (%d) %s\n", err_code, err_name);
    printf("    Error details : %s\n", err_desc);
    display.set_cloud_error();
}

static int register_mbed_client(NetworkInterface *iface, M2MClient *mbed_client)
{
    mbed_client->on_registered(NULL, mbed_client_on_registered);
    mbed_client->on_unregistered(NULL, mbed_client_on_unregistered);
    mbed_client->on_error(mbed_client, mbed_client_on_error);
    mbed_client->on_update_authorize(mbed_client_on_update_authorize);
    mbed_client->on_update_progress(mbed_client_on_update_progress);

    display.set_cloud_in_progress();
    mbed_client->call_register(iface);

    return 0;
}

static int init_fcc(void)
{
    fcc_status_e ret;

#if MBED_CONF_APP_FCC_WIPE
    ret = fcc_storage_delete();
    if (ret != FCC_STATUS_SUCCESS) {
        printf("ERROR: fcc delete failed: %d\n", ret);
    }
#endif

    ret = fcc_init();
    if (ret != FCC_STATUS_SUCCESS) {
        printf("ERROR: fcc init failed: %d\n", ret);
        return ret;
    }

    ret = fcc_developer_flow();
    if (ret == FCC_STATUS_KCM_FILE_EXIST_ERROR) {
        printf("fcc: developer credentials already exists\n");
    } else if (ret != FCC_STATUS_SUCCESS) {
        printf("ERROR: fcc failed to load developer credentials\n");
        return ret;
    }

    ret = fcc_verify_device_configured_4mbed_cloud();
    if (ret != FCC_STATUS_SUCCESS) {
        printf("ERROR: fcc device not configured for mbed cloud\n");
        return ret;
    }

    return 0;
}

// ****************************************************************************
// Generic Helpers
// ****************************************************************************
static int platform_init(void)
{
    int ret;

    /* setup the display */
    display.init(MBED_CONF_APP_VERSION);
    display.refresh();

#if MBED_CONF_MBED_TRACE_ENABLE
    /* Create mutex for tracing to avoid broken lines in logs */
    if (!mbed_trace_helper_create_mutex()) {
        printf("ERROR: Mutex creation for mbed_trace failed!\n");
        return -EACCES;
    }

    /* Initialize mbed trace */
    mbed_trace_init();
    mbed_trace_mutex_wait_function_set(mbed_trace_helper_mutex_wait);
    mbed_trace_mutex_release_function_set(mbed_trace_helper_mutex_release);
#endif

    /* init the sd card */
    ret = sd.init();
    if (ret != BD_ERROR_OK) {
        printf("ERROR: sd init failed: %d\n", ret);
        return ret;
    }
    printf("sd init OK\n");

    return 0;
}

static void platform_shutdown()
{
    /* stop the EventQueue */
    evq.break_dispatch();
}

// ****************************************************************************
// call back handlers for commandline interface
// ****************************************************************************
static void cmd_cb_del(vector<string>& params)
{
    //check params
    if (params.size() >= 2) {
        //the db
        Keystore k;

        //read the file
        k.open();

        //delete the given key
        k.del(params[1]);

        //write the changes back out
        k.close();

        //let user know
        cmd.printf("Deleted key %s\r\n",
                   params[1].c_str());
    } else {
        cmd.printf("Not enough arguments!\r\n");
    }
}

static void cmd_cb_get(vector<string>& params)
{
    //check params
    if (params.size() >= 1) {
        //database
        Keystore k;

        //don't show all keys by default
        bool ball = false;

        //read current values
        k.open();

        //if no param set to *
        if (params.size() == 1) {
            ball = true;
        } else if (params[1] == "*") {
            ball = true;
        }

        //show all keys?
        if (ball) {
            //get all keys
            vector<string> keys = k.keys();

            //walk the keys
            for (unsigned int n = 0; n < keys.size(); n++) {
                //get value
                string val = k.get(keys[n]);

                //format for display
                cmd.printf("%s=%s\r\n",
                           keys[n].c_str(),
                           val.c_str());
            }
        } else {

            // if not get one key
            string val = k.get(params[1]);

            //return just the value
            cmd.printf("%s\r\n",
                       val.c_str());
        }
    } else {
        cmd.printf("Not enough arguments!\r\n");
    }
}

static void cmd_cb_set(vector<string>& params)
{
    //check params
    if (params.size() >= 3) {

        //db
        Keystore k;

        //read the file into db
        k.open();

        //make the change
        k.set(params[1],params[2]);

        //write the file back out
        k.close();

        //return just the value
        cmd.printf("%s=%s\r\n",
                   params[1].c_str(),
                   params[2].c_str());

    } else {
        cmd.printf("Not enough arguments!\r\n");
    }
}

static void cmd_cb_reboot(vector<string>& params)
{
    cmd.printf("\r\nRebooting...");
    NVIC_SystemReset();
}

static void cmd_cb_reset(vector<string>& params)
{
    Keystore k;

    k.kill_all();
}

static void cmd_pump(Commander *cmd)
{
    cmd->pump();
}

void cmd_on_ready(void)
{
    evq.call(cmd_pump, &cmd);
}

/**
 * Sets up the command shell
 */
void init_commander(void)
{
    cmd.on_ready(cmd_on_ready);

    // add our callbacks
    cmd.add("get",
            "Get the value for the given configuration option. Usage: get <option> defaults to *=all",
            cmd_cb_get);

    cmd.add("set",
            "Set a configuration option to a the given value. Usage: set <option> <value>",
            cmd_cb_set);

    cmd.add("del",
            "Delete a configuration option from the store. Usage: del <option> <value>",
            cmd_cb_del);

    cmd.add("reboot",
            "Reboot the device. Usage: reboot",
            cmd_cb_reboot);

    cmd.add("reset",
            "Reset configuration options and/or certificates. Usage: reset <options|certs|all> defaults to options",
            cmd_cb_reset);

    //display the banner
    cmd.banner();

    //prime the serial
    cmd.init();
}

static void init_app(EventQueue *queue)
{
    int ret;

    m2mclient = new M2MClient();

    init_commander();

    /* create the network */
    printf("init network\n");
    net = network_create();
    if (NULL == net) {
        printf("ERROR: failed to create network stack\n");
        display.set_network_fail();
        return;
    }

    /* workaround: go ahead and connect the network.  it doesn't like being
     * polled for status before a connect() is attempted.
     * in addition, the fcc code requires a connected network when generating
     * creds the first time, so we need to spin here until we have an active
     * network. */
    do {
        display.set_network_in_progress();
        ret = network_connect(net);
        if (0 != ret) {
            display.set_network_fail();
            printf("WARN: failed to init network, retrying...\n");
            Thread::wait(2000);
        }
    } while (0 != ret);
    display.set_network_success();
    printf("init network: OK\n");

    /* initialize the factory configuration client
     * WARNING: the network must be connected first, otherwise this
     * will not return if creds haven't been provisioned for the first time.
     * */
    printf("init factory configuration client\n");
    ret = init_fcc();
    if (0 != ret) {
        printf("ERROR: failed to init factory configuration client: %d\n", ret);
        return;
    }
    printf("init factory configuration client: OK\n");

    printf("init sensors\n");
    sensors_init(&sensors, m2mclient);
    sensors_start(&sensors, &evq);

    /* connect to mbed cloud */
    printf("init mbed client\n");
    /* WARNING: the sensor resources must be added to the mbed client
     * before the mbed client connects to the cloud, otherwise the
     * sensor resources will not exist in the portal. */
    register_mbed_client(net, m2mclient);
}

// ****************************************************************************
// Main
// main() runs in its own thread in the OS
//
// Be aware of 3 threads of execution.
// 1. The init thread is kicked off when the app first starts and is
// responsible for bringing up the mbed client, the network, the sensors,
// etc., and exits as soon as initialization is complete.
// 2. The main thread dispatches the event queue and is where all normal
// runtime operations are processed.
// 3. The firmware update thread runs in the context of the mbed client and
// executes callbacks in our app.  When a firmware update begins and a
// download started, the event queue in the main thread must be halted until
// the download completes.  Through a good deal of testing, it seems that
// any work performed outside of the mbed client's context while a download
// is in progress will cause the downloaded file to become corrupt and
// therefore cause the firmware update to fail.
// ****************************************************************************
int main()
{
    int ret;

    /* stack size 2048 is too small for fcc_developer_flow() */
    Thread thread(osPriorityNormal, 4096);

    printf("FOTA demo version: %s\n", MBED_CONF_APP_VERSION);
    printf("     code version: " xstr(DEVTAG) "\n");

    /* minimal init sequence */
    printf("init platform\n");
    ret = platform_init();
    if (0 != ret) {
        return ret;
    }
    printf("init platform: OK\n");

    /* set the refresh rate of the display. */
    display_evq_id = evq.call_every(250, display_refresh, &display);

    /* use a separate thread to init the remaining components so that we
     * can continue to refresh the display */
    thread.start(callback(init_app, &evq));

    printf("entering run loop\n");
    evq.dispatch();
    printf("exited run loop\n");

    platform_shutdown();
    printf("exiting main\n");
    return 0;
}
