/*
   Basecamp - ESP32 library to simplify the basics of IoT projects
   Written by Merlin Schumacher (mls@ct.de) for c't magazin für computer technik (https://www.ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */

#ifndef Basecamp_h
#define Basecamp_h
#include "debug.hpp"
#include "Configuration.hpp"
#include <Preferences.h>
#include <rom/rtc.h>
#include <esp_wifi.h>
#ifndef BASECAMP_NOWIFI
#include "WifiControl.hpp"
#endif

#ifndef BASECAMP_NOWEB
#include "WebServer.hpp"
#endif

#ifndef BASECAMP_NOMQTT
#include <AsyncMqttClient.h>
#endif

#ifndef BASECAMP_NOOTA
#include <ArduinoOTA.h>
#endif

class Basecamp {
	public:
		Basecamp() {};
		~Basecamp() {};
		Configuration configuration;
		Preferences preferences;
		bool begin();
		bool checkResetReason();
		String showSystemInfo();
		String hostname;
		struct taskParms {
			char* parm1;
			const char* parm2;
		};
#ifndef BASECAMP_NOWIFI
		String mac;
		WifiControl wifi;
#endif

#ifndef BASECAMP_NOMQTT
		AsyncMqttClient mqtt;
		static void MqttHandling(void *);
#endif

#ifndef BASECAMP_NOWEB
		WebServer web;
#endif

#ifndef BASECAMP_NOOTA
		static void OTAHandling(void *);
#endif
	private:
		String _generateHostname();
		String _generateMac();
};
#endif
