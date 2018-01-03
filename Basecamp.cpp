/*
   Basecamp - ESP32 library to simplify the basics of IoT projects
   Written by Merlin Schumacher (mls@ct.de) for c't magazin für computer technik (https://www.ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */
#define DEBUG 1
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
	delay(5000);
#ifndef BASECAMP_NOMQTT
	//mqtt.setSecure(configuration.get("MQTTHost").toInt());
	//mqtt.setServerFingerprint(configuration.get("MQTTFingerprint").toInt());

	uint16_t mqttport = configuration.get("MQTTPort").toInt();
	char* mqtthost = configuration.getCString("MQTTHost");
	char* mqttuser = configuration.get("MQTTPort");
	char* mqttpass = configuration.getCString("MQTTHost");
	mqtt.setServer(mqtthost, mqttport);
	if(mqttuser != "") {
		mqtt.setCredentials(mqttuser,mqttpass);
	};

	mqtt.connect();
	free(mqtthost);
	free(mqttport);
	free(mqttuser);
	free(mqttpass);
	mqtt.onDisconnect([this](AsyncMqttClientDisconnectReason reason) {
			MqttReconnect(&mqtt);
			});

#endif

#ifndef BASECAMP_NOOTA
	xTaskCreatePinnedToCore(&OTAHandling, "ArduinoOTATask", 4096, NULL, 5, NULL,0);
#endif

#ifndef BASECAMP_NOWEB
	web.begin(configuration);

	if (configuration.get("WifiConfigured")) {
		web.addInterfaceElement("heading", "h1", "IoT Door Sensor","#wrapper");
		web.setInterfaceElementAttribute("heading", "class", "fat-border");
		web.addInterfaceElement("infotext1", "p", "Please finalize your configuration","#wrapper");
		web.addInterfaceElement("configform", "form", "","#wrapper");
		web.setInterfaceElementAttribute("configform", "action", "saveConfig");
		web.addInterfaceElement("WifiEssid", "input", "WIFI SSID:","#configform" , "WifiEssid");
		web.addInterfaceElement("WifiPassword", "input", "WIFI Password:", "#configform", "WifiPassword");
		web.setInterfaceElementAttribute("WifiPassword", "type", "password");
		web.addInterfaceElement("saveform", "input", "","#configform");
		web.setInterfaceElementAttribute("saveform", "type", "button");
		web.setInterfaceElementAttribute("saveform", "value", "Save");
		web.setInterfaceElementAttribute("saveform", "onclick", "collectConfiguration()");
	}
#endif

}


#ifndef BASECAMP_NOMQTT
void Basecamp::MqttReconnect(AsyncMqttClient * mqtt) {
	while (1) {
		if (mqtt->connected() != 1 && WiFi.status() == WL_CONNECTED) {
			DEBUG_PRINTLN("Callback: MQTT disconnected, reconnecting");
			mqtt->connect();
		} else if (WiFi.status() != WL_CONNECTED) {
			mqtt->disconnect();
		} else {
			break;
		}
	}
}

#endif

bool Basecamp::checkResetReason() {

	preferences.begin("basecamp", false);
	int reason = rtc_get_reset_reason(0);
	DEBUG_PRINT("Reset reason: ");
	DEBUG_PRINTLN(reason);
	if (reason == 1 || reason == 16) {
		unsigned int bootCounter = preferences.getUInt("bootcounter", 0);

		bootCounter++;
		DEBUG_PRINT("Unsuccessful boots: ");
		DEBUG_PRINTLN(bootCounter);

		if (bootCounter > 3){
			DEBUG_PRINTLN("Configuration forcibly reset.");
			configuration.set("WifiConfigured", "False");
			configuration.save();
			preferences.putUInt("bootcounter", 0);
			preferences.end();
			Serial.println("Resetting the WiFi configuration.");
			ESP.restart();
		} else if (bootCounter > 2 && configuration.get("WifiConfigured") == "False") {
			Serial.println("Factory reset was forced.");
			SPIFFS.format();
			preferences.putUInt("bootcounter", 0);
			preferences.end();
			Serial.println("Rebooting.");
			ESP.restart();
		} else {
			preferences.putUInt("bootcounter", bootCounter);
		};

	} else {
		preferences.putUInt("bootcounter", 0);
	};
	preferences.end();
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
	while (1) {
		ArduinoOTA.handle();

		vTaskDelay(100);

	}
};
#endif
