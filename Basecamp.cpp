/*
   Basecamp - ESP32 library to simplify the basics of IoT projects
   Written by Merlin Schumacher (mls@ct.de) for c't magazin für computer technik (https://www.ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */

#include <iomanip>
#include "Basecamp.hpp"
#include "debug.hpp"

namespace {
	const constexpr uint16_t defaultThreadStackSize = 3072;
	const constexpr UBaseType_t defaultThreadPriority = 0;
	// Default length for access point mode password
	const constexpr unsigned defaultApSecretLength = 8;
}

Basecamp::Basecamp(SetupModeWifiEncryption setupModeWifiEncryption, ConfigurationUI configurationUi)
	: MqttGuardInterface(mqtt)
	, configuration(String{"/basecamp.json"})
	, setupModeWifiEncryption_(setupModeWifiEncryption)
	, configurationUi_(configurationUi)
{
}

/**
 * This function generates a cleaned string from the device name set by the user.
 */
String Basecamp::_cleanHostname()
{
	String clean_hostname =	configuration.get(ConfigurationKey::deviceName); // Get device name from configuration

	// If hostname is not set, return default
	if (clean_hostname == "") {
		return "basecamp-device";
	}

	// Transform device name to lower case
	clean_hostname.toLowerCase();

	// Replace all non-alphanumeric characters in hostname to minus symbols
	for (int i = 0; i <= clean_hostname.length(); i++) {
		if (!isalnum(clean_hostname.charAt(i))) {
			clean_hostname.setCharAt(i,'-');
		};
	};
	DEBUG_PRINTLN(clean_hostname);

	// return cleaned hostname String
	return clean_hostname;
};

/**
 * Returns true if a secret for the setup WiFi AP is set
 */
bool Basecamp::isSetupModeWifiEncrypted(){
	return (setupModeWifiEncryption_ == SetupModeWifiEncryption::secured);
}

/**
 * Returns the SSID of the setup WiFi network
 */
String Basecamp::getSetupModeWifiName(){
	return wifi.getAPName();
}

/**
 * Returns the secret of the setup WiFi network
 */
String Basecamp::getSetupModeWifiSecret(){
	return configuration.get(ConfigurationKey::accessPointSecret);
}

/**
 * This is the initialisation function for the Basecamp class.
 */
bool Basecamp::begin(String fixedWiFiApEncryptionPassword)
{
	// Make sure we only accept valid passwords for ap
	if (fixedWiFiApEncryptionPassword.length() != 0) {
		if (fixedWiFiApEncryptionPassword.length() >= wifi.getMinimumSecretLength()) {
			setupModeWifiEncryption_ = SetupModeWifiEncryption::secured;
		} else {
			Serial.println("Error: Given fixed ap secret is too short. Refusing.");
		}
	}

	// Enable serial output
	Serial.begin(115200);
	// Display a simple lifesign
	Serial.println("");
	Serial.println("Basecamp Startup");

	// Load configuration from internal flash storage.
	// If configuration.load() fails, reset the configuration
	if (!configuration.load()) {
		DEBUG_PRINTLN("Configuration is broken. Resetting.");
		configuration.reset();
	};

	// Get a cleaned version of the device name.
	// It is used as a hostname for DHCP and ArduinoOTA.
	hostname = _cleanHostname();
	DEBUG_PRINTLN(hostname);

	// Have checkResetReason() control if the device configuration
	// should be reset or not.
	checkResetReason();

#ifndef BASECAMP_NOWIFI

	// If there is no access point secret set yet, generate one and save it.
	// It will survive the default config reset.
	if (!configuration.isKeySet(ConfigurationKey::accessPointSecret) ||
		fixedWiFiApEncryptionPassword.length() >= wifi.getMinimumSecretLength())
	{
		String apSecret = fixedWiFiApEncryptionPassword;
		if (apSecret.length() < wifi.getMinimumSecretLength()) {
			// Not set or too short. Generate a random one.
			Serial.println("Generating access point secret.");
			apSecret = wifi.generateRandomSecret(defaultApSecretLength);
		} else {
			Serial.println("Using fixed access point secret.");
		}
		configuration.set(ConfigurationKey::accessPointSecret, apSecret);
		configuration.save();
	}

	DEBUG_PRINTF("Secret: %s\n", configuration.get(ConfigurationKey::accessPointSecret).c_str());

	// Initialize Wifi with the stored configuration data.
	wifi.begin(
			configuration.get(ConfigurationKey::wifiEssid), // The (E)SSID or WiFi-Name
			configuration.get(ConfigurationKey::wifiPassword), // The WiFi password
			configuration.get(ConfigurationKey::wifiConfigured), // Has the WiFi been configured
			hostname, // The system hostname to use for DHCP
			(setupModeWifiEncryption_ == SetupModeWifiEncryption::none)?"":configuration.get(ConfigurationKey::accessPointSecret)
	);

	// Get WiFi MAC
	mac = wifi.getSoftwareMacAddress(":");
#endif
#ifndef BASECAMP_NOMQTT
	// Check if MQTT has been disabled by the user
	if (!configuration.get(ConfigurationKey::mqttActive).equalsIgnoreCase("false")) {
		// Setting up variables for the MQTT client. This is necessary due to
		// the nature of the library. It won't work properly with Arduino Strings.
		const auto &mqtthost = configuration.get(ConfigurationKey::mqttHost);
		const auto &mqttuser = configuration.get(ConfigurationKey::mqttUser);
		const auto &mqttpass = configuration.get(ConfigurationKey::mqttPass);
		// INFO: that library just copies the pointer to the hostname. As long as nobody
		// modifies the config, this may work.
		mqtt.setClientId(hostname.c_str());
		auto mqttport = configuration.get(ConfigurationKey::mqttPort).toInt();
		if (mqttport == 0) mqttport = 1883;
		// INFO: that library just copies the pointer to the hostname. As long as nobody
		// modifies the config, this may work.
		// Define the hostname and port of the MQTT broker.
		mqtt.setServer(mqtthost.c_str(), mqttport);
		// If MQTT credentials are stored, set them.
		if (mqttuser.length() != 0) {
			mqtt.setCredentials(mqttuser.c_str(), mqttpass.c_str());
		};
		// Create a timer and register a "onDisconnect" callback function that manages the (re)connection of the MQTT client
		// It will be called by the Asyc-MQTT-Client KeepAlive function if a connection loss is detected
		// The timer is then started and will start a function to reconnect MQTT after 2 seconds 
		mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)&mqtt, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
		mqtt.onDisconnect(onMqttDisconnect);
		// Do not connect MQTT directly but only start the timer to give the main setup() time to register all MQTT callbacks before 
		// Especially a "onConnect" callback should be in place to get informed about a successful MQTT connection
		// setup() can optionally call mqtt.connect() by itself if MQTT is needed before timer elapses
		xTimerStart(mqttReconnectTimer, 0);
	};
#endif

#ifndef BASECAMP_NOOTA
	// Set up Over-the-Air-Updates (OTA) if it hasn't been disabled.
	if (!configuration.get(ConfigurationKey::otaActive).equalsIgnoreCase("false")) {

		// Set OTA password
		String otaPass = configuration.get(ConfigurationKey::otaPass);
		if (otaPass.length() != 0) {
			ArduinoOTA.setPassword(otaPass.c_str());
		}

		// Set OTA hostname
		ArduinoOTA.setHostname(hostname.c_str());

		// The following code is copied verbatim from the ESP32 BasicOTA.ino example
		// This is the callback for the beginning of the OTA process
		ArduinoOTA
			.onStart([]() {
					String type;
					if (ArduinoOTA.getCommand() == U_FLASH)
					type = "sketch";
					else // U_SPIFFS
					type = "filesystem";
					SPIFFS.end();

					Serial.println("Start updating " + type);
					})
		// When the update ends print it to serial
		.onEnd([]() {
				Serial.println("\nEnd");
				})
		// Show the progress of the update
		.onProgress([](unsigned int progress, unsigned int total) {
				Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
				})
		// Error handling for the update
		.onError([](ota_error_t error) {
				Serial.printf("Error[%u]: ", error);
				if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
				else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
				else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
				else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
				else if (error == OTA_END_ERROR) Serial.println("End Failed");
				});

		// Start the OTA service
		ArduinoOTA.begin();

	}
#endif

#ifndef BASECAMP_NOWEB
	if (shouldEnableConfigWebserver())
	{
		// Add a webinterface element for the h1 that contains the device name. It is a child of the #wrapper-element.
		web.addInterfaceElement("heading", "h1", "","#wrapper");
		web.setInterfaceElementAttribute("heading", "class", "fat-border");
		web.addInterfaceElement("logo", "img", "", "#heading");
		web.setInterfaceElementAttribute("logo", "src", "/logo.svg");
		String DeviceName = configuration.get(ConfigurationKey::deviceName);
		if (DeviceName == "") {
			DeviceName = "Unconfigured Basecamp Device";
		}
		web.addInterfaceElement("title", "title", DeviceName,"head");
		web.addInterfaceElement("devicename", "span", DeviceName,"#heading");
		// Set the class attribute of the element to fat-border.
		web.setInterfaceElementAttribute("heading", "class", "fat-border");
		// Add a paragraph with some basic information
		web.addInterfaceElement("infotext1", "p", "Configure your device with the following options:","#wrapper");

		// Add the configuration form, that will include all inputs for config data
		web.addInterfaceElement("configform", "form", "","#wrapper");
		web.setInterfaceElementAttribute("configform", "action", "#");
		web.setInterfaceElementAttribute("configform", "onsubmit", "collectConfiguration()");

		web.addInterfaceElement("DeviceName", "input", "Device name","#configform" , "DeviceName");

#ifdef BASECAMP_WIRED_NETWORK
		// Add an input field for the WIFI data and link it to the corresponding configuration data
		web.addInterfaceElement("WifiEssid", "input", "WIFI SSID:","#configform" , "WifiEssid");
		web.addInterfaceElement("WifiPassword", "input", "WIFI Password:", "#configform", "WifiPassword");
		web.setInterfaceElementAttribute("WifiPassword", "type", "password");
		web.addInterfaceElement("WifiConfigured", "input", "", "#configform", "WifiConfigured");
		web.setInterfaceElementAttribute("WifiConfigured", "type", "hidden");
		web.setInterfaceElementAttribute("WifiConfigured", "value", "true");
#endif
		// Add input fields for MQTT configurations if it hasn't been disabled
		if (!configuration.get(ConfigurationKey::mqttActive).equalsIgnoreCase("false")) {
			web.addInterfaceElement("MQTTHost", "input", "MQTT Host:","#configform" , "MQTTHost");
			web.addInterfaceElement("MQTTPort", "input", "MQTT Port:","#configform" , "MQTTPort");
			web.setInterfaceElementAttribute("MQTTPort", "type", "number");
			web.setInterfaceElementAttribute("MQTTPort", "min", "0");
			web.setInterfaceElementAttribute("MQTTPort", "max", "65535");
			web.addInterfaceElement("MQTTUser", "input", "MQTT Username:","#configform" , "MQTTUser");
			web.addInterfaceElement("MQTTPass", "input", "MQTT Password:","#configform" , "MQTTPass");
			web.setInterfaceElementAttribute("MQTTPass", "type", "password");
		}
		// Add a save button that calls the JavaScript function collectConfiguration() on click
		web.addInterfaceElement("saveform", "button", "Save","#configform");
		web.setInterfaceElementAttribute("saveform", "type", "submit");

		// Show the devices MAC in the Webinterface
		String infotext2 = "This device has the MAC-Address: " + mac;
		web.addInterfaceElement("infotext2", "p", infotext2,"#wrapper");

		web.addInterfaceElement("footer", "footer", "Powered by ", "body");
		web.addInterfaceElement("footerlink", "a", "Basecamp", "footer");
		web.setInterfaceElementAttribute("footerlink", "href", "https://github.com/merlinschumacher/Basecamp");
		web.setInterfaceElementAttribute("footerlink", "target", "_blank");
		#ifdef BASECAMP_USEDNS
		#ifdef DNSServer_h
		if (!configuration.get(ConfigurationKey::wifiConfigured).equalsIgnoreCase("true")) {
			dnsServer.start(53, "*", wifi.getSoftAPIP());
			xTaskCreatePinnedToCore(&DnsHandling, "DNSTask", 4096, (void*) &dnsServer, 5, NULL,0);
		}
		#endif
		#endif
		// Start webserver and pass the configuration object to it
		// Also pass a Lambda-function that restarts the device after the configuration has been saved.
		web.begin(configuration, [](){
			delay(2000);
			ESP.restart();
		});
	}
	#endif
	Serial.println(showSystemInfo());

	// TODO: only return true if everything setup up correctly
	return true;
}

/**
 * This is the background task function for the Basecamp class. To be called from Arduino loop.
 */
void Basecamp::handle (void)
{
	#ifndef BASECAMP_NOOTA
		// This call takes care of the ArduinoOTA function provided by Basecamp
		ArduinoOTA.handle();
	#endif
}


#ifndef BASECAMP_NOMQTT

bool Basecamp::shouldEnableConfigWebserver() const
{
	return (configurationUi_ == ConfigurationUI::always ||
	   (configurationUi_ == ConfigurationUI::accessPoint && wifi.getOperationMode() == WifiControl::Mode::accessPoint));
}

// This is a task that is called if MQTT client has lost connection. After 2 seconds it automatically trys to reconnect.

TimerHandle_t Basecamp::mqttReconnectTimer;
  
void Basecamp::onMqttDisconnect(AsyncMqttClientDisconnectReason reason) 
{
  Serial.print("MQTT Disconnected. Reason: "); Serial.println((int)reason, DEC); 
  xTimerStart(mqttReconnectTimer, 0);
}

void Basecamp::connectToMqtt(TimerHandle_t xTimer) 
{
  AsyncMqttClient *mqtt = (AsyncMqttClient *) pvTimerGetTimerID(xTimer);

  if (WifiControl::isConnected()) {
    Serial.println("Trying to connect ...");
    mqtt->connect();    // has no effect if already connected ( if (_connected) return;) 
  }
  else {
    Serial.println("Waiting for WiFi ...");
    xTimerStart(xTimer, 0);
  }  
}

#endif

#ifdef BASECAMP_USEDNS
#ifdef DNSServer_h
// This is a task that handles DNS requests from clients
void Basecamp::DnsHandling(void * dnsServerPointer)
{
		DNSServer * dnsServer = (DNSServer *) dnsServerPointer;
		while(1) {
			// handle each request
			dnsServer->processNextRequest();
			vTaskDelay(1000);
		}
};
#endif
#endif

// This function checks the reset reason returned by the ESP and resets the configuration if neccessary.
// It counts all system reboots that occured by power cycles or button resets.
// If the ESP32 receives an IP the boot counts as successful and the counter will be reset by Basecamps
// WiFi management.
void Basecamp::checkResetReason()
{
	// Instead of the internal flash it uses the somewhat limited, but sufficient preferences storage
	preferences.begin("basecamp", false);
	// Get the reset reason for the current boot
	int reason = rtc_get_reset_reason(0);
	DEBUG_PRINT("Reset reason: ");
	DEBUG_PRINTLN(reason);
	// If the reason is caused by a power cycle (1) or a RTC reset / button press(16) evaluate the current
	// bootcount and act accordingly.
	if (reason == 1 || reason == 16) {
		// Get the current number of unsuccessful boots stored
		unsigned int bootCounter = preferences.getUInt("bootcounter", 0);
		// increment it
		bootCounter++;
		DEBUG_PRINT("Unsuccessful boots: ");
		DEBUG_PRINTLN(bootCounter);

		// If the counter is bigger than 3 it will be the fifths consecutive unsucessful reboot.
		// This forces a reset of the WiFi configuration and the AP will be opened again
		if (bootCounter > 3){
			DEBUG_PRINTLN("Configuration forcibly reset.");
			// Mark the WiFi configuration as invalid
			configuration.set(ConfigurationKey::wifiConfigured, "False");
			// Save the configuration immediately
			configuration.save();
			// Reset the boot counter
			preferences.putUInt("bootcounter", 0);
			// Call the destructor for preferences so that all data is safely stored befor rebooting
			preferences.end();
			Serial.println("Resetting the WiFi configuration.");
			// Reboot
			ESP.restart();

			// If the WiFi is unconfigured and the device is rebooted twice format the internal flash storage
		} else if (bootCounter > 2 && configuration.get(ConfigurationKey::wifiConfigured).equalsIgnoreCase("false")) {
			Serial.println("Factory reset was forced.");
			// Format the flash storage
			SPIFFS.format();
			// Reset the boot counter
			preferences.putUInt("bootcounter", 0);
			// Call the destructor for preferences so that all data is safely stored befor rebooting
			preferences.end();
			Serial.println("Rebooting.");
			// Reboot
			ESP.restart();

		// In every other case: store the current boot count
		} else {
			preferences.putUInt("bootcounter", bootCounter);
		};

	// if the reset has been for any other cause, reset the counter
	} else {
		preferences.putUInt("bootcounter", 0);
	};
	// Call the destructor for preferences so that all data is safely stored
	preferences.end();
};

// This shows basic information about the system. Currently only the mac
// TODO: expand infos
String Basecamp::showSystemInfo() {
	std::ostringstream info;
	info << "MAC-Address: " << mac.c_str();
	info << ", Hardware MAC: " << wifi.getHardwareMacAddress(":").c_str() << std::endl;

	if (configuration.isKeySet(ConfigurationKey::accessPointSecret)) {
			info << "*******************************************" << std::endl;
			info << "* ACCESS POINT PASSWORD: ";
			info << configuration.get(ConfigurationKey::accessPointSecret).c_str() << std::endl;
			info << "*******************************************" << std::endl;
	}

	return {info.str().c_str()};
}

