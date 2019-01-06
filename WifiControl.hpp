/*
   Basecamp - ESP32 library to simplify the basics of IoT projects
   Written by Merlin Schumacher (mls@ct.de) for c't magazin für computer technik (https://www.ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */

#ifndef WifiControl_h
#define WifiControl_h

#include "debug.hpp"
#include <iomanip>
#include <sstream>
#include <WiFi.h>
#include <WiFiClient.h>
#include <Preferences.h>

class WifiControl {
	public:
		enum class Mode {
			unconfigured,
			accessPoint,
			client,
		};

		WifiControl(){};
		bool connect();
		bool disconnect();
		static bool isConnected() ;

		Mode getOperationMode() const;

		void begin(String essid, String password = "", String configured = "False",
							 String hostname = "BasecampDevice", String apSecret="");
		IPAddress getIP();
		IPAddress getSoftAPIP();
		void setAPName(const String &name);
		String getAPName();
		int status();
		static void WiFiEvent(WiFiEvent_t event);

		unsigned getMinimumSecretLength() const;
		String generateRandomSecret(unsigned length) const;

		/*
			Returns the MAC Address of the wifi adapter in hexadecimal form, optionally delimited
			by a given delimiter which is inserted between every hex-representation.
			e.G. getMacAddress(":") would return "aa:bb:cc:..."
		*/
		String getHardwareMacAddress(const String& delimiter = {});
		String getSoftwareMacAddress(const String& delimiter = {});
	private:
		String _wifiEssid;
		String _wifiPassword;
		String _ap;
		String _wifiAPName;

		Mode operationMode_ = Mode::unconfigured;
};

#endif
