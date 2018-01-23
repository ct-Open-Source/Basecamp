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
}

Basecamp::Basecamp()
	: configuration(String{"/basecamp.json"})
{

}

/**
 * This function generates a cleaned string from the device name set by the user.
 */
String Basecamp::_generateHostname()
{
	String clean_hostname =	configuration.get("DeviceName"); // Get device name from configuration

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
 * This is the initialisation function for the Basecamp class.
 */

bool Basecamp::begin()
{
	// Enable serial output
	Serial.begin(115200);
	// Display a simple lifesign
	Serial.println("Basecamp V.0.1.6");

	// Load configuration from internal flash storage.
	// If configuration.load() fails, reset the configuration
	if (!configuration.load()) {
		DEBUG_PRINTLN("Configuration is broken. Resetting.");
		configuration.reset();
	};

	// Get a cleaned version of the device name.
	// It is used as a hostname for DHCP and ArduinoOTA.
	hostname = _generateHostname();
	DEBUG_PRINTLN(hostname);

	// Have checkResetReason() control if the device configuration
	// should be reset or not.
	checkResetReason();

#ifndef BASECAMP_NOWIFI

	// Initialize Wifi with the stored configuration data.
	wifi.begin(
			configuration.get("WifiEssid"), // The (E)SSID or WiFi-Name
			configuration.get("WifiPassword"), // The WiFi password
			configuration.get("WifiConfigured"), // Has the WiFi been configured
			hostname // The system hostname to use for DHCP
		  );

	// Get Mac-Address in form aa::bb::cc:...
	mac = wifi.getSoftwareMacAddress(":");
	macHardware = wifi.getHardwareMacAddress(":");

	DEBUG_PRINTLN(showSystemInfo());
#endif
#ifndef BASECAMP_NOMQTT
	// Check if MQTT has been disabled by the user
	if (configuration.get("MQTTActive") != "false") {
		// Setting up variables for the MQTT client. This is necessary due to
		// the nature of the library. It won't work properly with Arduino Strings.
		const auto &mqtthost = configuration.get("MQTTHost");
		const auto &mqttuser = configuration.get("MQTTUser");
		const auto &mqttpass = configuration.get("MQTTPass");
		// INFO: that library just copies the pointer to the hostname. As long as nobody
		// modifies the config, this may work.
		mqtt.setClientId(hostname.c_str());
		// FIXME: It this is empty -> defaults?
		auto mqttport = configuration.get("MQTTPort").toInt();
		// INFO: that library just copies the pointer to the hostname. As long as nobody
		// modifies the config, this may work.
		// Define the hostname and port of the MQTT broker.
		mqtt.setServer(mqtthost.c_str(), mqttport);
		// If MQTT credentials are stored, set them.
		if (mqttuser.length() != 0) {
			mqtt.setCredentials(mqttuser.c_str(), mqttpass.c_str());
		};
		// Start a task that manages the (re)connection of the MQTT client
		// It's pinned to the same core (0) as FreeRTOS so the Arduino code inside setup()
		// and loop() will not be interrupted, as they are pinned to core 1.
		xTaskCreatePinnedToCore(&MqttHandling, "MqttTask", defaultThreadStackSize,
				(void *)&mqtt, defaultThreadPriority, NULL, 0);
	};
#endif

#ifndef BASECAMP_NOOTA
	// Set up Over-the-Air-Updates (OTA) if it hasn't been disabled.
	if(configuration.get("OTAActive") != "false") {
		// Create struct that stores the parameters for the OTA task
		struct taskParms OTAParams[1];
		// TODO: How long do these params have to be living?
		// Set OTA password
		OTAParams[0].parm1 = configuration.get("OTAPass").c_str();
		// Set OTA hostname
		OTAParams[0].parm2 = hostname.c_str();

		// Create a task that takes care of OTA update handling.
		// It's pinned to the same core (0) as FreeRTOS so the Arduino code inside setup()
		// and loop() will not be interrupted, as they are pinned to core 1.
		xTaskCreatePinnedToCore(&OTAHandling, "ArduinoOTATask", defaultThreadStackSize,
				(void *)&OTAParams[0], defaultThreadPriority, NULL, 0);
	}
#endif

#ifndef BASECAMP_NOWEB
	// Start webserver and pass the configuration object to it
	web.begin(configuration);

	// Add a webinterface element for the h1 that contains the device name. It is a child of the #wrapper-element.
	web.addInterfaceElement("heading", "h1", configuration.get("DeviceName"),"#wrapper");
	// Set the class attribute of the element to fat-border.
	web.setInterfaceElementAttribute("heading", "class", "fat-border");

	// Add a paragraph with some basic information
	web.addInterfaceElement("infotext1", "p", "Configure your device with the following options:","#wrapper");

	// Add the configuration form, that will include all inputs for config data
	web.addInterfaceElement("configform", "form", "","#wrapper");
	web.setInterfaceElementAttribute("configform", "action", "saveConfig");

	web.addInterfaceElement("DeviceName", "input", "Device name","#configform" , "DeviceName");

	// Add an input field for the WIFI data and link it to the corresponding configuration data
	web.addInterfaceElement("WifiEssid", "input", "WIFI SSID:","#configform" , "WifiEssid");
	web.addInterfaceElement("WifiPassword", "input", "WIFI Password:", "#configform", "WifiPassword");
	web.setInterfaceElementAttribute("WifiPassword", "type", "password");
	web.addInterfaceElement("WifiConfigured", "input", "", "#configform", "WifiConfigured");
	web.setInterfaceElementAttribute("WifiConfigured", "type", "hidden");
	web.setInterfaceElementAttribute("WifiConfigured", "value", "true");

	// Add input fields for MQTT configurations if it hasn't been disabled
	if (configuration.get("MQTTActive") != "false") {
		web.addInterfaceElement("MQTTHost", "input", "MQTT Host:","#configform" , "MQTTHost");
		web.addInterfaceElement("MQTTPort", "input", "MQTT Port:","#configform" , "MQTTPort");
		web.setInterfaceElementAttribute("MQTTPort", "type", "number");
		web.addInterfaceElement("MQTTUser", "input", "MQTT Username:","#configform" , "MQTTUser");
		web.addInterfaceElement("MQTTPass", "input", "MQTT Password:","#configform" , "MQTTPass");
		web.setInterfaceElementAttribute("MQTTPass", "type", "password");
	}
	// Add a save button that calls the JavaScript function collectConfiguration() on click
	web.addInterfaceElement("saveform", "input", " ","#configform");
	web.setInterfaceElementAttribute("saveform", "type", "button");
	web.setInterfaceElementAttribute("saveform", "value", "Save");
	web.setInterfaceElementAttribute("saveform", "onclick", "collectConfiguration()");

	// Show the devices MAC in the Webinterface
	String infotext2 = "This device has the MAC-Address: " + mac;
	web.addInterfaceElement("infotext2", "p", infotext2,"#wrapper");
#ifdef DNSServer_h
	if(configuration.get("WifiConfigured") != "True"){
    		dnsServer.start(53, "*", wifi.getSoftAPIP());
		xTaskCreatePinnedToCore(&DnsHandling, "DNSTask", 4096, (void*) &dnsServer, 5, NULL,0);
	}
	#endif
#endif

	Serial.println(showSystemInfo());

	// TODO: only return true if everything setup up correctly
	return true;
}

#ifndef BASECAMP_NOMQTT

//This is a task that checks if the MQTT client is still connected or not. If not it automatically reconnect.
// TODO: Think about making void* the real corresponding type
void Basecamp::MqttHandling(void *mqttPointer)
{
		// is set to true, when a connection attempt is already running. Paralell connection attempts
		// seem to mess up the async-mqtt-client library.
		bool mqttIsConnecting = false;
		AsyncMqttClient *mqtt = (AsyncMqttClient *)mqttPointer;
		while(1) {
			// TODO: What is the sense behind these magics?
			// If the MQTT client is not connected force a disconnect.
			if (mqtt->connected() != 1) {
				mqttIsConnecting = false;
				mqtt->disconnect(true);
			}
			// If the MQTT client is not connecting, not already connected and the WiFi has a
			// connection, try to connect
			if (!mqttIsConnecting) {
				if(mqtt->connected() != 1) {
					if (WiFi.status() == WL_CONNECTED) {
						mqtt->connect();
						mqttIsConnecting = true;
					} else {
						mqtt->disconnect();
					}
				}
			}
			vTaskDelay(100);
		}
};
#endif

#ifdef DNSServer_h
// This is a task that handles DNS requests from clients
void Basecamp::DnsHandling(void * dnsServerPointer) {

		DNSServer * dnsServer = (DNSServer *) dnsServerPointer;
		while(1) {
			// handle each request
			dnsServer->processNextRequest();
			vTaskDelay(100);
		}
};
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
			configuration.set("WifiConfigured", "False");
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
		} else if (bootCounter > 2 && configuration.get("WifiConfigured") == "False") {
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

#ifndef BASECAMP_NOOTA
// This tasks takes care of the ArduinoOTA function provided by Basecamp
void Basecamp::OTAHandling(void * OTAParams) {

	// Create a struct to store the given parameters
	struct taskParms *params;
	// Cast the void type pointer given to the task into a struct
	params = (struct taskParms *) OTAParams;

	// The first parameter is assumed to be the password for the OTA process
	// If it's set, require a password for upgrades
	if (strlen(params->parm1) != 0) {
		ArduinoOTA.setPassword(params->parm1);
	}
	// The second parameter is assumed to be the hostname of the esp
	// It is set to be distinctive in the Arduino IDE
	ArduinoOTA.setHostname(params->parm2);
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
				// NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
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
	// The while loop checks if OTA requests are received and sleeps for a bit if not
	while (1) {
		ArduinoOTA.handle();

		vTaskDelay(100);

	}
};
#endif

String Basecamp::showSystemInfo() {
	std::ostringstream info;
	info << "MAC-Address: " << mac.c_str();
	info << ", Hardware MAC: " << macHardware.c_str();

	return {info.str().c_str()};
}
