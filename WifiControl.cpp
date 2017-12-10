/*
   Basecamp - ESP32 library to simplify the basics of IoT projects
   Written by Merlin Schumacher (mls@ct.de) for c't magazin für computer technik (https://www.ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */

#include "WifiControl.hpp"
#include "debug.hpp"
void WifiControl::begin(String essid, String password, String configured)
{
	DEBUG_PRINTLN("Connecting to Wifi");

	String _wifiConfigured = configured;
	String _wifiEssid = essid;
	String _wifiPassword = password;

	if (_wifiConfigured == "True") {
		DEBUG_PRINTLN("Wifi is configured");
		DEBUG_PRINT("Connecting to ");
		DEBUG_PRINTLN(_wifiEssid);

		WiFi.begin ( _wifiEssid.c_str(), _wifiPassword.c_str());
		WiFi.setAutoConnect ( true );                                             // Autoconnect to last known Wifi on startup
		WiFi.setAutoReconnect ( true );
		//xTaskCreate(&WifiConnector, "WifiConnectorTask", 4096, NULL, 5, NULL);
	} else {

		DEBUG_PRINTLN("Wifi is NOT configured");
		DEBUG_PRINTLN("Starting Wifi AP");

		WiFi.mode(WIFI_AP_STA);
		WiFi.softAP("ESP32");
		//xTaskCreate(&DNSTask, "DNSTask", 4096, NULL, 5, NULL);

	}
}

int WifiControl::status() {
	return WiFi.status();

}
IPAddress WifiControl::getIP() {
	return WiFi.localIP();
}




void WifiControl::DNSTask(void *) {
	//DNSServer dnsServer;
	//IPAddress apIP(192, 168, 4, 1);
	//dnsServer.start(53, "*", apIP);
	while (1) {
		//dnsServer.processNextRequest();
		vTaskDelay(100);
	}
}

//void WifiControl::WifiConnector(void *) {
	//while (WiFi.status() != WL_CONNECTED) {
		//DEBUG_PRINT(".");
		//delay(1000);
	//}
	//Serial.println(WiFi.localIP());
	
	//Preferences preferences;
	//preferences.begin("basecamp", false);
	//unsigned int bootCounter = preferences.putUInt("bootcounter", 0);
	//preferences.putUInt("bootcounter", 0);
	//preferences.end();
	//vTaskDelete( NULL );
//}


void WifiControl::WiFiEvent(WiFiEvent_t event)
{
    Serial.printf("[WiFi-event] event: %d\n", event);

    switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        Serial.println("WiFi lost connection");
	delay(1000);
	//WiFi.connect();
        break;
    }
}


