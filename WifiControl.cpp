/*
   Basecamp - ESP32 library to simplify the basics of IoT projects
   Written by Merlin Schumacher (mls@ct.de) for c't magazin für computer technik (https://www.ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */

#include "WifiControl.hpp"

namespace {
	// Minumum access point secret length to be generated (8 is min for ESP32)
	const constexpr unsigned minApSecretLength = 8;
}

void WifiControl::begin(String essid, String password, String configured,
												String hostname, String apSecret)
{
	DEBUG_PRINTLN("Connecting to Wifi");

	String _wifiConfigured = std::move(configured);
	_wifiEssid = std::move(essid);
	_wifiPassword = std::move(password);
	_wifiAPName = "ESP32_" + getHardwareMacAddress();

	WiFi.onEvent(WiFiEvent);
	if (_wifiConfigured == "True") {
		operationMode_ = Mode::client;
		DEBUG_PRINTLN("Wifi is configured");
		DEBUG_PRINT("Connecting to ");
		DEBUG_PRINTLN(_wifiEssid);

		WiFi.begin(_wifiEssid.c_str(), _wifiPassword.c_str());
		WiFi.setHostname(hostname.c_str());
		//WiFi.setAutoConnect ( true );
		//WiFi.setAutoReconnect ( true );
	} else {
		operationMode_ = Mode::accessPoint;
		DEBUG_PRINTLN("Wifi is NOT configured");
		DEBUG_PRINTF("Starting Wifi AP '%s'", _wifiAPName);

		WiFi.mode(WIFI_AP_STA);
		if (apSecret.length() > 0) {
			// Start with password protection
			DEBUG_PRINTF("Starting AP with password %s\n", apSecret.c_str());
			WiFi.softAP(_wifiAPName.c_str(), apSecret.c_str());
		} else {
			// Start without password protection
			WiFi.softAP(_wifiAPName.c_str());
		}
	}
}

WifiControl::Mode WifiControl::getOperationMode() const
{
	return operationMode_;
}

int WifiControl::status() {
	return WiFi.status();

}
IPAddress WifiControl::getIP() {
	return WiFi.localIP();
}
IPAddress WifiControl::getSoftAPIP() {
	return WiFi.softAPIP();
}

void WifiControl::WiFiEvent(WiFiEvent_t event)
{
	Preferences preferences;
	preferences.begin("basecamp", false);
	unsigned int bootCounter = preferences.getUInt("bootcounter", 0);
	// In case somebody wants to know this..
	DEBUG_PRINTF("[WiFi-event] event. Bootcounter is %d\n", bootCounter);
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
		default:
			// INFO: Default = do nothing
			break;
	}
}

namespace {
	template <typename BYTES>
	String format6Bytes(const BYTES &bytes, const String& delimiter)
	{
		std::ostringstream stream;
		for (unsigned int i = 0; i < 6; i++) {
			if (i != 0 && delimiter.length() > 0) {
				stream << delimiter.c_str();
			}
			stream << std::setfill('0') << std::setw(2) << std::hex << static_cast<unsigned int>(bytes[i]);
		}

		String mac{stream.str().c_str()};
		return mac;
	}
}

// TODO: This will return the default mac, not a manually set one
// See https://github.com/espressif/esp-idf/blob/master/components/esp32/include/esp_system.h
String WifiControl::getHardwareMacAddress(const String& delimiter)
{
	uint8_t rawMac[6];
	esp_efuse_mac_get_default(rawMac);
	return format6Bytes(rawMac, delimiter);
}

String WifiControl::getSoftwareMacAddress(const String& delimiter)
{
	uint8_t rawMac[6];
	WiFi.macAddress(rawMac);
	return format6Bytes(rawMac, delimiter);
}

unsigned WifiControl::getMinimumSecretLength() const
{
	return minApSecretLength;
}

String WifiControl::generateRandomSecret(unsigned length) const
{
	// There is no "O" (Oh) to reduce confusion
	const String validChars{"abcdefghjkmnopqrstuvwxyzABCDEFGHJKMNPQRSTUVWXYZ23456789.-,:$/"};
	String returnValue;

	unsigned useLength = (length < minApSecretLength)?minApSecretLength:length;
	returnValue.reserve(useLength);

	for (unsigned i = 0; i < useLength; i++)
	{
		auto randomValue = validChars[(esp_random() % validChars.length())];
		returnValue += randomValue;
	}

	return returnValue;
}
