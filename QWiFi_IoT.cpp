#include "QWiFi_IoT.h"

//Debug functions
//Don't use in PROD
void QWiFi_IoT::__displayCreds(void){    
    Serial.printf("mode : %s\nAP_SSID : %s\nAP_PASS : %s\nSTA_SSID : %s\nSTA_PASS : %s\n"
                ,wc.mode
                ,wc.ap_ssid
                ,wc.ap_pass
                ,wc.sta_ssid
                ,wc.sta_pass
    );
}

//*******************[Private Member Functions]**********************//

bool QWiFi_IoT::_validateCreds(String dn, String dp, String sn, String sp, String *msg)
{
    if(dn.length() <8 || dn.length()>20 || dn.indexOf(" ") != -1){
        *msg = "Invalid Device Name";
        return false;
    }
    if(dp.length() <8 || dp.length()>20){
        *msg = "Invalid Device Password";
        return false;
    }
    if (sn.length() == 0 || sn.length() > 32)
    {
        *msg = "Invalid STA SSID";
        return false;
    }
    if (sp.length() < 8 || sp.length() > 63)
    {
        *msg = "Invalid STA Password";
        return false;
    }
    return true;
}

bool QWiFi_IoT::_getCreds(void)
{
    if (!LittleFS.begin())
    {
        Serial.println("[_getCreds] : LittleFS initialization unsuccessful");
        Serial.println("[_getCreds] : Resorting to default AP creds");
        return true;
    }
    // LittleFS.format(); // for testing only
    if (!LittleFS.exists(CREDS_PATH))
    {
        Serial.printf("[_getCreds] : No file found at %s\n", CREDS_PATH);
        Serial.println("[_getCreds] : Resorting to default AP credentials");
        return true;
    }

    File credsFile = LittleFS.open(CREDS_PATH, "r");
    if (!credsFile)
    {
        Serial.printf("[_getCreds] : Failed to read credentials file at %s\n", CREDS_PATH);
        Serial.println("[_getCreds] : Resorting to default AP creds");
        return true;
    }
    wc.mode = credsFile.readStringUntil('\n');
    wc.ap_ssid = credsFile.readStringUntil('\n');
    wc.ap_pass = credsFile.readStringUntil('\n');
    wc.sta_ssid = credsFile.readStringUntil('\n');
    wc.sta_pass = credsFile.readStringUntil('\n');

    credsFile.close();
    return wc.mode == "AP"; // AP->true, STA->false
}

bool QWiFi_IoT::_saveCreds(void)
{
    if (!LittleFS.begin())
    {
        Serial.println("[_saveCreds] : LittleFS check unsuccessful");
        Serial.println("[_saveCreds] : Try restarting the device");
        return false;
    }
    File credsFile = LittleFS.open(CREDS_PATH, "w");
    if (!credsFile)
    {
        Serial.printf("[_saveCreds] : Failed to create file at %s\n", CREDS_PATH);
        return false;
    }
    credsFile.printf("%s\n%s\n%s\n%s\n%s\n", wc.mode.c_str(), wc.ap_ssid.c_str(), wc.ap_pass.c_str(), wc.sta_ssid.c_str(), wc.sta_pass.c_str());
    credsFile.close();
    return true;
}

bool QWiFi_IoT::_parseCreds(uint8_t *data,String *d_name, String *d_pass, String *ssid, String *pass)
{
    String body = (char *)data;

    int si = body.indexOf("\"dn\":\"");
    if(si == -1){
        return false;
    }
    si+=6;
    int ei = body.indexOf("\",", si);
    if(ei == -1){
        return false;
    }
    *d_name = body.substring(si, ei);

    si = body.indexOf("\"dp\":\"");
    if(si == -1){
        return false;
    }
    si+=6;
    ei = body.indexOf("\",", si);
    if(ei == -1){
        return false;
    }
    *d_pass = body.substring(si, ei);

    si = body.indexOf("\"sn\":\"");
    if(si == -1){
        return false;
    }
    si+=6;
    ei = body.indexOf("\",", si);
    if(ei == -1){
        return false;
    }
    *ssid = body.substring(si, ei);

    si = body.indexOf("\"sp\":\"");
    if(si == -1){
        return false;
    }
    si+=6;
    ei = body.indexOf("\"", si);
    if(ei == -1){
        return false;
    }
    *pass = body.substring(si, ei);
    return true;
}

void QWiFi_IoT::_StartWiFiEvents(void)
{
    staGotIPHandler = WiFi.onStationModeGotIP([this](const WiFiEventStationModeGotIP &event)
                                              {
        _boot_wifi_conn_f = false;
        Serial.println("[staGotIPHandler] : Connected to Wi-Fi sucessfully.");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        
        _retr = 0; });

    staDisconnectedHandler = WiFi.onStationModeDisconnected([this](const WiFiEventStationModeDisconnected &event)
                                                         {
        if(_status_received_f){
            return;
        }
        if(_retr < 5){
            Serial.printf("[staDisconnectedHandler] : Disconnected from Wi-Fi Attempting reconnect : %d\n", _retr);
            _connectToAccessPoint();
            _retr++;
            return;
        }
        if(_boot_wifi_conn_f)
            wc.mode="AP";
        
        _saveCreds();
        ESP.restart(); });
}

void QWiFi_IoT::_APServerDefinition(void)
{
    _ap_server = new AsyncWebServer(_port);
    _ap_server->on("/", HTTP_GET, [this](AsyncWebServerRequest *req)
                { 
                    wc.mode = "STA";
                    _saveCreds();
                    req->send_P(200, "text/html", _index); 
                });
    _ap_server->on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *req)
                   { req->send_P(200, "text/css", _styles); });
    _ap_server->on("/script.js", HTTP_GET, [](AsyncWebServerRequest *req)
                   { req->send_P(200, "application/javascript", _script); });
    _ap_server->on("/reset", HTTP_GET, [this](AsyncWebServerRequest *req)
                   {
                        if(!LittleFS.begin()){
                            req->send(500, "text/plain", "FS Init : Please restart the device");
                            return;
                        }
                        if(!LittleFS.remove(CREDS_PATH)){
                            req->send(500, "text/plain", "Failed to delete the saved credentials");
                            return;
                        }
                        if(!LittleFS.remove(JWT_PATH)){
                            req->send(500, "text/plain", "Failed to delete JWT");
                        }
                        req->send(200, "text/plain", "ok");
                        ESP.restart();
                    });
    _ap_server->onRequestBody([this](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t index, size_t total)
                              {  
        if (len > 250){
            req->send(400, "text/plain", "body too large");
            return;
        }
        if (req->url() != "/update"){
            req->send(400, "text/plain", "bad request");
            return;
        }
        
        String dn,dp,sn,sp;
        
        if (!_parseCreds(data, &dn,&dp,&sn,&sp)){
            req->send(400, "text/plain", "error parsing json body");
            return;
        }
        String msg;
        if(!_validateCreds(dn,dp,sn,sp, &msg)){
            req->send(400, "text/plain", msg.c_str());
            return;
        }

        wc.ap_ssid = dn;
        wc.ap_pass = dp;
        wc.sta_ssid = sn;
        wc.sta_pass = sp;
        wc.mode = "STA";
        
        if(!_saveCreds()){
                req->send(500, "text/plain", "Failed to save details");
                return;
        }
        req->send(200, "text/plain", "Please restart the device to connect to AP"); });
}

void QWiFi_IoT::_startAP(void)
{
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);
    WiFi.softAP(wc.ap_ssid, wc.ap_pass);
    delay(50);
    Serial.println("[_startAP] : started Access point");
    Serial.println(WiFi.softAPIP());
    _ap_server->begin();
}

void QWiFi_IoT::_connectToAccessPoint(void)
{
    if(wc.sta_ssid == "" || wc.ap_ssid == Q_WIFI_DEFAULT_AP_SSID){
        wc.mode = "AP";
        _saveCreds();
        ESP.restart();
    }
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin(wc.sta_ssid, wc.sta_pass);
    Serial.printf("[_connectToAccessPoint] : Attempting to connect to %s\n", wc.sta_ssid.c_str());
}

void QWiFi_IoT::_initToken()
{
    if (!LittleFS.begin())
    {
        Serial.println("[LittleFS] : LittleFS Initialization Failed");
        return;
    }
    if (!LittleFS.exists(JWT_PATH))
    {
        Serial.printf("[LittleFS] : JWT file does not exist at %s\n", JWT_PATH);
        return;
    }
    File jwtFile = LittleFS.open(JWT_PATH, "r");
    if (!jwtFile)
    {
        Serial.printf("[LittleFS] : Failed to read file at %s\n", JWT_PATH);
        return;
    }
    String jwt_data = jwtFile.readString();
    jwtFile.close();
    _token_exists_f =_validateToken(jwt_data);
    if (!_token_exists_f)
    {
        Serial.printf("[LittleFS] : Invalid JWT %s\n", JWT_PATH);
        return;
    }
    _jwt = jwt_data;
    Serial.printf("[LittleFS] : Existing Jwt found : ");
    Serial.println(_jwt);
}

bool QWiFi_IoT::_saveToken(String *token)
{
    if (!LittleFS.begin())
    {
        Serial.println("[LittleFS] : LittleFS Initialization Failed");
        return false;
    }
    _token_exists_f = _validateToken(*token);
    if(!_token_exists_f){
        Serial.printf("[_validateToken] : Invalid Token\n");
        return false;
    }
    File jwtFile = LittleFS.open(JWT_PATH, "w");
    if (!jwtFile)
    {
        Serial.printf("[LittleFS] : Failed to create file at %s\n", JWT_PATH);
        return false;
    }
    jwtFile.print(*token);
    jwtFile.close();
    Serial.println(*token);
    _jwt = *token;
    return true;
}

bool QWiFi_IoT::_validateToken(String payload)
{
    uint16_t indexDot1 = payload.indexOf('.');
    if (indexDot1 == -1)
    {
        return false;
    }
    String subPayload = payload.substring(indexDot1 + 1);
    uint16_t indexDot2 = subPayload.indexOf('.');
    if (indexDot2 == -1)
    {
        return false;
    }
    return true;
}

bool QWiFi_IoT::_login(WiFiClientSecure *client)
{
    HTTPClient https;
    https.setTimeout(10000);
    Serial.print("[HTTPS] login...\n");
    String url = String(BASE_URL) + "/login";
    if (!https.begin(*client, url))
    {
        Serial.printf("[HTTPS] Unable to login\n");
    }
    https.addHeader("Content-Type", "application/json");
    //create req_body
    String req_body = "{\"lk\":\""+String(LICENSE_KEY)+"\",\"name\":\""+wc.ap_ssid+"\",\"pass\":\""+wc.ap_pass+"\"}";
    int httpCode = https.POST(req_body);
    if (httpCode <= 0)
    {
        Serial.printf("[HTTPS] POST - Login... failed, error: %s\n", https.errorToString(httpCode).c_str());
        https.end();
        return false;
    }
    Serial.printf("[HTTPS] POST - Login... code: %d\n", httpCode);
    String payload = https.getString();
    if (httpCode != HTTP_CODE_OK && httpCode != HTTP_CODE_MOVED_PERMANENTLY)
    {
        Serial.printf("[HTTPS] POST - Login... Error Payload: %s\n", payload.c_str());
        https.end();
        return false;
    }
    
    uint8_t payloadCheck = _validateToken(payload);
    if (!payloadCheck)
    {
        https.end();
        return false;
    }
    _token_exists_f = _saveToken(&payload);
    if (!_token_exists_f)
    {
        https.end();
        return false;
    }

    return true;
}

bool QWiFi_IoT::_parsePayload(String *payload){
    int si = payload->indexOf("\"power\":");
    if(si == -1){
        return false;
    }
    si+=8;
    int ei = payload->indexOf(",", si);
    if(ei == -1){
        return false;
    }
    String power = payload->substring(si,ei);
    if(power != "true" && power != "false"){
        return false;
    }
    _ld->power = power == "true";
    si = payload->indexOf("\"brightness\":");
    if(si == -1){
        return false;
    }
    si+=13;
    ei = payload->indexOf(",", si);
    if(ei == -1){
        return false;
    }
    String bright = payload->substring(si, ei);
    char *endptr;
    int brightness = (int)strtol(bright.c_str(), &endptr, 10);//string, end of conversion pointer, base of int conversion
    if(*endptr != '\0'){
        return false;
    }
    if(brightness < 0 || brightness >100){
        return false;
    }
    _ld->brightness = brightness;
    //conversion complete
    return true;
}

bool QWiFi_IoT::_getStatus(WiFiClientSecure * client){
  HTTPClient https;
  https.setTimeout(10000);
  Serial.print("[HTTPS] status...\n");
  String url = String(BASE_URL)+"/status";
  if (!https.begin(*client, url)) {
    Serial.printf("[HTTPS] Unable to login\n");
    return false;
  }
  https.addHeader("x-auth-token", _jwt);
  int httpCode = https.GET();
  if (httpCode <= 0) {
     Serial.printf("[HTTPS] GET... failed,errorCode: %d, error: %s\n",httpCode, https.errorToString(httpCode).c_str());
     https.end();
     return false;
  }
  Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
  if(httpCode != HTTP_CODE_OK && httpCode != HTTP_CODE_MOVED_PERMANENTLY){
     https.end();
     return false;
  }
  String payload = https.getString();
  https.end();
  if(!_parsePayload(&payload)){
    Serial.printf("[HTTPS] Error in parsing payload.. exiting _getStatus\n");
  }
  return true;
}

//*******************[Public Member Functions]**********************//

QWiFi_IoT::QWiFi_IoT(uint16_t port, lightData * ld)
{
    _ld = ld;
    _port = port;
}

QWiFi_IoT::QWiFi_IoT(lightData * ld)
{
    QWiFi_IoT(80, ld);
}

void QWiFi_IoT::begin()
{
    _StartWiFiEvents();
    if (_getCreds())
    {
        _APServerDefinition();
        _startAP();
        return;
    }
    _connectToAccessPoint();
    _initToken();
}

void QWiFi_IoT::autoNTPSync()
{
    if (!_ntp_time_synced && WiFi.status() == WL_CONNECTED)
    {
        Serial.println("Config Time");
        configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov"); // does this for every 3 hrs
        Serial.print("Waiting for NTP time sync: ");
        time_t now = time(nullptr);
        uint8_t count = 0;
        while ((now < 8 * 3600 * 2) && WiFi.status() == WL_CONNECTED && (count++) < 0x20)
        {
            delay(500);
            Serial.print(".");
            now = time(nullptr);
        }
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("NTP Sync Error : WiFi not connected");
            return;
        }
        if (count > 0x20)
        {
            Serial.println("NTP Timeout");
            return;
        }
        Serial.println("");
        struct tm timeinfo;
        gmtime_r(&now, &timeinfo);
        Serial.print("Current time: ");
        Serial.print(asctime(&timeinfo));
        _ntp_time_synced = true;
    }
}

void QWiFi_IoT::statusUpdated(bool recv_status){
    _status_received_f = recv_status;
    _get_status_t = millis();
    if(recv_status){ //recv_status == true i.e gotStatus so WiFi force sleep
        Serial.printf("[statusUpdated] : power : %d, brightness : %d\n", _ld->power, _ld->brightness);

        Serial.println("[statusUpdated] : WiFi entering sleep mode");
        WiFi.disconnect();
        WiFi.mode(WIFI_OFF);
        WiFi.forceSleepBegin();
    }else{
        Serial.println("[statusUpdated] : WiFi activating...");
        _connectToAccessPoint();
    }
}

void QWiFi_IoT::dataSync(X509List * _cert)
{
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }
    if(!_ntp_time_synced){
        return;
    }
    WiFiClientSecure client;
    client.setTrustAnchors(_cert);

    if(!_token_exists_f){
        _login(&client);
        return;
    }
    
    if(!_getStatus(&client)){
        _token_exists_f = false;
        return;
    }
    
    statusUpdated(true);
    
    
}

void QWiFi_IoT::handleStatus(X509List * cert, uint32_t _delay){

    if(!_status_received_f){
        autoNTPSync();
        dataSync(cert);
        return;
    } 
    if(millis() - _get_status_t > _delay){
        statusUpdated(false);
    }
}