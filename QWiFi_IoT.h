#ifndef _QWIFI_IOT_
#define _QWIFI_IOT_
#include <ESP8266WiFi.h>       
#include <ESPAsyncWebServer.h> 
#include <ESPAsyncTCP.h> 
#include "LittleFS.h"

#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>

#include "index.h"
#include "styles.h"
#include "script.h"

#include "config.h"

#define CREDS_PATH "/wificreds.bin"
#define JWT_PATH "/token.bin"

typedef struct {
    bool power = false;
    uint8_t brightness = 30;
} lightData;

class QWiFi_IoT
{
private:
    AsyncWebServer * _ap_server = NULL;
    WiFiEventHandler staDisconnectedHandler, staGotIPHandler;
    uint16_t _port = 80;
    struct wifiCreds
    {
        String mode = "AP";
        String ap_ssid = Q_WIFI_DEFAULT_AP_SSID;
        String ap_pass = Q_WIFI_DEFAULT_AP_PASS;
        String sta_ssid = "";
        String sta_pass = "";
    } wc;

    lightData * _ld = NULL;
    String _jwt = "";
   
    uint8_t _retr = 0;

    bool _boot_wifi_conn_f = true;
    bool _ntp_time_synced = false;
    bool _token_exists_f = false;
    bool _status_received_f = false;


    uint32_t _get_status_t = 0;

    // Debug functions
    void __displayCreds(void);
    // functions
    bool _validateCreds(String dn, String dp, String sn, String sp, String *msg);
    bool _getCreds(void);
    bool _saveCreds(void);
    bool _parseCreds(uint8_t *data, String *d_name, String *d_pass, String *ssid, String *pass);

    void _StartWiFiEvents(void);
    void _APServerDefinition(void);
    void _startAP(void);
    void _connectToAccessPoint(void);

    // After WiFi STA Connection - WiFi.status == WL_CONNECTED
    void _initToken(void);
    bool _saveToken(String *token);

    bool _validateToken(String payload);
    bool _login(WiFiClientSecure *client);
    bool _parsePayload(String *payload);
    bool _getStatus(WiFiClientSecure *client);
    
public:
    QWiFi_IoT(uint16_t port, lightData *ld);
    QWiFi_IoT(lightData *ld);
    void begin(void);
    void autoNTPSync(void);
    void statusUpdated(bool value);
    void dataSync(X509List *_cert);
    void handleStatus(X509List * cert,  uint32_t _delay);
};

#endif _QWIFI_IOT_