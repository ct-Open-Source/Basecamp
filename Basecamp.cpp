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
	//mqtt.setSecure(configuration.get("MQTTHost").toInt());
	//mqtt.setServerFingerprint(configuration.get("MQTTFingerprint").toInt());
	String tmpstring = configuration.get("MQTTHost");
	int tmpstring_len = tmpstring.length()+1;
	char mqtthost[tmpstring_len];
	configuration.get("MQTTHost").toCharArray(mqtthost,tmpstring_len);
		
	uint16_t mqttport = configuration.get("MQTTPort").toInt();
	Serial.println("cfg");
	Serial.println(mqtthost);
	Serial.println(mqttport);
	Serial.println("cfg");
	mqtt.setServer("openhab",mqttport);

	const char* mqttuser  = configuration.get("MQTTUser").c_str();
	const char* mqttpass  = configuration.get("MQTTPass").c_str();
	if(mqttuser != "") {
	mqtt.setCredentials(
			configuration.get("MQTTUser").c_str(),
			configuration.get("MQTTPass").c_str()
			);
	};
	mqtt.onDisconnect([this](AsyncMqttClientDisconnectReason reason) {
			MqttReconnect(&mqtt);
			});

	xTaskCreate(&MqttConnector, "MqttConnector", 4096, (void*)&mqtt,5,NULL);
#endif

#ifndef BASECAMP_NOOTA
	xTaskCreate(&OTAHandling, "ArduinoOTATask", 4096, NULL, 5, NULL);
#endif

#ifndef BASECAMP_NOWEB
	web.begin(configuration);

	if (configuration.get("WifiConfigured")) {
		web.addInterfaceElement("configform", "form", "","#wrapper");
		web.setInterfaceElementAttribute("configform", "action", "saveConfig");
		web.addInterfaceElement("WifiEssid", "input", "WIFI SSID:","#configform" , "WifiEssid");
		web.addInterfaceElement("WifiPassword", "input", "WIFI Password:", "#configform", "WifiPassword");
		web.setInterfaceElementAttribute("WifiPassword", "type", "password");
		web.addInterfaceElement("heading", "h1", "IoT Door Sensor","#wrapper");
		web.setInterfaceElementAttribute("heading", "class", "fat-border");
		web.addInterfaceElement("infotext1", "p", "Please finalize your configuration","#wrapper");
	}
#endif

};


#ifndef BASECAMP_NOMQTT
void Basecamp::MqttConnector(void * mqtt) {
	AsyncMqttClient &mqttclient = *((AsyncMqttClient*)mqtt);
	while (!mqttclient.connected()) {
	mqttclient.connect();
	vTaskDelay(1000);
	}
	vTaskDelete(NULL);
}

void Basecamp::MqttReconnect(AsyncMqttClient * mqtt) {
       DEBUG_PRINTLN("MQTT disconnected, reconnecting");
       while (WiFi.status() != WL_CONNECTED) {
	       delay(1000);
       }
       delay(1000);
       mqtt->connect();
}

#endif

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
	while (1) {
		ArduinoOTA.handle();

		vTaskDelay(100);

	}
};
#endif
