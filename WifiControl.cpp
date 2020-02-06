/*
   Basecamp - ESP32 library to simplify the basics of IoT projects
   Written by Merlin Schumacher (mls@ct.de) for c't magazin für computer technik (https://www.ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */

#include "WifiControl.hpp"
#ifdef BASECAMP_WIRED_NETWORK
#include <ETH.h>
#endif

namespace {
	// Minumum access point secret length to be generated (8 is min for ESP32)
	const constexpr unsigned minApSecretLength = 8;
	static bool eth_connected = false;
}

void WifiControl::begin(String essid, String password, String configured,
												String hostname, String apSecret)
{
#ifdef BASECAMP_WIRED_NETWORK
	DEBUG_PRINTLN("Connecting to Ethernet");
	operationMode_ = Mode::client;
	WiFi.onEvent(WiFiEvent);
	ETH.begin() ;
	ETH.setHostname(hostname.c_str());
	DEBUG_PRINTLN ("Ethernet initialized") ;
	DEBUG_PRINTLN ("Waiting for connection") ;
	while (!eth_connected) {
		DEBUG_PRINT (".") ;
		delay(100) ;
	}
#else
	DEBUG_PRINTLN("Connecting to Wifi");
	String _wifiConfigured = std::move(configured);
	_wifiEssid = std::move(essid);
	_wifiPassword = std::move(password);
	if (_wifiAPName.length() == 0) {
		_wifiAPName = "ESP32_" + getHardwareMacAddress();
	}

	WiFi.onEvent(WiFiEvent);
	if (_wifiConfigured.equalsIgnoreCase("true")) {
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
#endif

}


bool WifiControl::isConnected()
{
#ifdef BASECAMP_WIRED_NETWORK
	return eth_connected ;
#else
	return WiFi.isConnected() ;
#endif
}

WifiControl::Mode WifiControl::getOperationMode() const
{
	return operationMode_;
}

int WifiControl::status() {
	return WiFi.status();

}
IPAddress WifiControl::getIP() {
#ifdef BASECAMP_WIRED_NETWORK
	return ETH.localIP() ;
#else
	return WiFi.localIP();
#endif
}
IPAddress WifiControl::getSoftAPIP() {
	return WiFi.softAPIP();
}

void WifiControl::setAPName(const String &name) {
	_wifiAPName = name;
}

String WifiControl::getAPName() {
	return _wifiAPName;
}

void WifiControl::WiFiEvent(WiFiEvent_t event)
{
	Preferences preferences;
	preferences.begin("basecamp", false);
	unsigned int bootCounter = preferences.getUInt("bootcounter", 0);
	// In case somebody wants to know this..
	DEBUG_PRINTF("[WiFi-event] event. Bootcounter is %d\n", bootCounter);
	DEBUG_PRINTF("[WiFi-event] event: %d\n", event);
#ifdef BASECAMP_WIRED_NETWORK
	switch (event) {
    case SYSTEM_EVENT_ETH_START:
      DEBUG_PRINTLN("ETH Started");
      break;
    case SYSTEM_EVENT_ETH_CONNECTED:
      DEBUG_PRINTLN("ETH Connected");
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
      DEBUG_PRINT("ETH MAC: ");
      DEBUG_PRINT(ETH.macAddress());
      DEBUG_PRINT(", IPv4: ");
      DEBUG_PRINT(ETH.localIP());
      if (ETH.fullDuplex()) {
        DEBUG_PRINT(", FULL_DUPLEX");
      }
      DEBUG_PRINT(", ");
      DEBUG_PRINT(ETH.linkSpeed());
      DEBUG_PRINTLN("Mbps");
      eth_connected = true;
      break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      DEBUG_PRINTLN("ETH Disconnected");
      eth_connected = false;
      break;
    case SYSTEM_EVENT_ETH_STOP:
      DEBUG_PRINTLN("ETH Stopped");
      eth_connected = false;
      break;
    default:
      break;
  }
#else
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
#endif
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
#ifdef BASECAMP_WIRED_NETWORK
	return ETH.macAddress() ;
#else
	uint8_t rawMac[6];
	esp_efuse_mac_get_default(rawMac);
	return format6Bytes(rawMac, delimiter);
#endif
}

String WifiControl::getSoftwareMacAddress(const String& delimiter)
{
#ifdef BASECAMP_WIRED_NETWORK
	return ETH.macAddress() ;
#else
	uint8_t rawMac[6];
	WiFi.macAddress(rawMac);
	return format6Bytes(rawMac, delimiter);
#endif
	
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
