/*
   Basecamp - ESP32 library to simplify the basics of IoT projects
   Written by Merlin Schumacher (mls@ct.de) for c't magazin für computer technik (https://www.ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */
#include "WifiControl.hpp"
#include "debug.hpp"
#include "Basecamp.hpp"
void WifiControl::begin(String essid, String password, String configured, String hostname)
{
	DEBUG_PRINTLN("Connecting to Wifi");

	String _wifiConfigured = configured;
	String _wifiEssid = essid;
	String _wifiPassword = password;

	WiFi.onEvent(WiFiEvent);
	if (_wifiConfigured == "True") {
		DEBUG_PRINTLN("Wifi is configured");
		DEBUG_PRINT("Connecting to ");
		DEBUG_PRINTLN(_wifiEssid);

		WiFi.begin (_wifiEssid.c_str(), _wifiPassword.c_str());
		WiFi.setHostname(hostname.c_str());
		//WiFi.setAutoConnect ( true );                                  
		//WiFi.setAutoReconnect ( true );
	} else {

		DEBUG_PRINTLN("Wifi is NOT configured");
		DEBUG_PRINTLN("Starting Wifi AP");

		WiFi.mode(WIFI_AP_STA);
		WiFi.softAP("ESP32");


	}
}

int WifiControl::status() {
	return WiFi.status();

}
IPAddress WifiControl::getIP() {
	return WiFi.localIP();
}

void WifiControl::WiFiEvent(WiFiEvent_t event)
{
	Preferences preferences;
	preferences.begin("basecamp", false);
	unsigned int bootCounter = preferences.getUInt("bootcounter", 0);
	DEBUG_PRINTF("[WiFi-event] event: %d\n", event);
	switch(event) {
		case SYSTEM_EVENT_STA_GOT_IP:
			DEBUG_PRINT("Wifi IP address: ");
			DEBUG_PRINTLN(WiFi.localIP());
			preferences.putUInt("bootcounter", 0);
			break;
		case SYSTEM_EVENT_STA_DISCONNECTED:
			DEBUG_PRINTLN("WiFi lost connection");
			WiFi.reconnect();
			break;
	}
}



