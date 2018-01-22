/*
   Basecamp - ESP32 library to simplify the basics of IoT projects
   Written by Merlin Schumacher (mls@ct.de) for c't magazin für computer technik (https://www.ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */

#ifndef WifiControl_h
#define WifiControl_h

#include <WiFi.h>
#include <WiFiClient.h>
#include <Preferences.h>

class WifiControl {
	public:
		WifiControl(){};
		bool connect();
		bool disconnect();
		void begin(String essid, String password = "", String configured = "False", String hostname = "BasecampDevice");
		IPAddress getIP();
		IPAddress getSoftAPIP();
		int status();
		static void WiFiEvent(WiFiEvent_t event);
	private:
		String _wifiEssid;
		String _wifiPassword;
		String _ap;
};

#endif
