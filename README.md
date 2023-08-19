# ESP-QSolarLamp-ESP8266

### Client side code for QSolarLamp
### This code currently supports ESP8266/ESP8285 only

### Features:
* Autoconnect to AP (no credential hardcoding)
* Handles WiFi AP disconnections
* AP mode has a good user inteface to enter device details and AP details
* WiFi connection is event based - no blocking code for WiFi operations
* AP mode server is Async in nature as it is based on [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)

### Downsides:
* HTTPS connection via HTTPClient is synchronous hence leads to blocking code during HTTPS connection
* NTP Sync is needed to verify the server certificate's validity which is blocking
