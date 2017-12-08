/*
   Basecamp - ESP32 library to simplify the basics of IoT projects
   Written by Merlin Schumacher (mls@ct.de) for c't magazin für computer technik (https://www.ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */

#include "Basecamp.hpp"
#include "debug.hpp"

bool Basecamp::begin() {
	Serial.begin(115200);
	Serial.println("Basecamp V.0.0.1");

	configuration.begin("/basecamp.json");

	if (!configuration.load()) {
		DEBUG_PRINTLN("Configuration is broken. Resetting.");
		configuration.reset();
	};


	checkResetReason();

#ifndef BASECAMP_NOWIFI
	wifi.begin(
			configuration.get("WifiEssid"),
			configuration.get("WifiPassword"),
			configuration.get("WifiConfigured")
		  );
#endif

#ifndef BASECAMP_NOMQTT
	mqtt.setServer(
			configuration.get("MQTTHost").c_str(),
			configuration.get("MQTTPort").toInt()
		      );

	mqtt.connect();
#endif

#ifndef BASECAMP_NOOTA
	xTaskCreate(&OTAHandling, "ArduinoOTATask", 4096, NULL, 5, NULL);
#endif

#ifndef BASECAMP_NOWEB
	web.begin(configuration);

	if (configuration.get("WifiConfigured")) {
		web.addInterfaceElement("WifiEssid", "input", "WIFI SSID:","#configform" , "WifiEssid");
		web.addInterfaceElement("WifiPassword", "input", "WIFI Password:", "#configform", "WifiPassword");
		web.setInterfaceElementAttribute("WifiPassword", "type", "password");
		web.addInterfaceElement("heading", "h1", "IoT Door Sensor","#wrapper");
		web.setInterfaceElementAttribute("heading", "class", "fat-border");
		web.addInterfaceElement("infotext1", "p", "Please finalize your configuration","#wrapper");
		web.addInterfaceElement("configform", "form", "","#wrapper");
		web.setInterfaceElementAttribute("configform", "action", "saveConfig");
	}
#endif

};

bool Basecamp::checkResetReason() {

	preferences.begin("basecamp", false);
	int reason = rtc_get_reset_reason(0);
	if (reason == 1 || reason == 16) {
		unsigned int bootCounter = preferences.getUInt("bootcounter", 0);

		DEBUG_PRINT("Times booted: ");
		DEBUG_PRINTLN(bootCounter);
		bootCounter++;
		preferences.putUInt("bootcounter", bootCounter);

		if (bootCounter > 4){
			DEBUG_PRINTLN("Configuration forcibly reset.");
			//configuration.reset();
			configuration.set("WifiConfigured", "False");
			configuration.save();
			preferences.putUInt("bootcounter", 0);
			preferences.end();
			ESP.restart();
		};
	} else {
		preferences.putUInt("bootcounter", 0);
	};
};

#ifndef BASECAMP_NOOTA
void Basecamp::OTAHandling(void *) {
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
	.onEnd([]() {
			Serial.println("\nEnd");
			})
	.onProgress([](unsigned int progress, unsigned int total) {
			Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
			})
	.onError([](ota_error_t error) {
			Serial.printf("Error[%u]: ", error);
			if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
			else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
			else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
			else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
			else if (error == OTA_END_ERROR) Serial.println("End Failed");
			});
	ArduinoOTA.begin();
	Serial.println("OTA");
	while (1) {
		ArduinoOTA.handle();

		vTaskDelay(100);

	}
};
#endif


