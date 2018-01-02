/*
   Basecamp - ESP32 library to simplify the basics of IoT projects
   Written by Merlin Schumacher (mls@ct.de) for c't magazin für computer technik (https://www.ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */

#include "Basecamp.hpp"
#include "debug.hpp"
#include <HTTPClient.h>
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
	HTTPClient http;
	//mqtt.setSecure(configuration.get("MQTTHost").toInt());
	//mqtt.setServerFingerprint(configuration.get("MQTTFingerprint").toInt());
	//Serial.println(configuration.get("MQTTHost"));
	//const char* mqtthost = configuration.get("MQTTHost").c_str();
	//const char* httphost;
        //sprintf(httphost);
	//http.begin("http://"+mqtthost);
	//Serial.println(mqtthost);
        //int httpCode = http.GET();

        //// httpCode will be negative on error
        //if(httpCode > 0) {
            //// HTTP header has been send and Server response header has been handled
            //Serial.printf("[HTTP] GET... code: %d\n", httpCode);

            //// file found at server
            //if(httpCode == HTTP_CODE_OK) {
                //String payload = http.getString();
                //Serial.println(payload);
            //}
        //} else {
            //Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        //}
	//const char* mqtthost = "mls-pc";
	uint16_t mqttport = configuration.get("MQTTPort").toInt();
	const char* mqtthost = configuration.get("MQTTHost").c_str();
	mqtt.setServer(mqtthost, mqttport);

	const char* mqttuser = configuration.get("MQTTUser").c_str();
	const char* mqttpass = configuration.get("MQTTPass").c_str();
	if(mqttuser != "") {
		mqtt.setCredentials(
				configuration.get("MQTTUser").c_str(),
				configuration.get("MQTTPass").c_str()
				);
	};
	mqtt.onDisconnect([this](AsyncMqttClientDisconnectReason reason) {
			MqttReconnect(&mqtt);
			});

	//xTaskCreatePinnedToCore(&MqttConnector, "MqttConnector", 4096, (void*)&mqtt,5,NULL,0);
	mqtt.connect();
#endif

#ifndef BASECAMP_NOOTA
	xTaskCreatePinnedToCore(&OTAHandling, "ArduinoOTATask", 4096, NULL, 5, NULL,0);
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
		web.addInterfaceElement("saveform", "input", "Save","#save");
	}
#endif

}


#ifndef BASECAMP_NOMQTT
void Basecamp::MqttConnector(void * mqtt) {
	AsyncMqttClient &mqttclient = *((AsyncMqttClient*)mqtt);
	while (1) {
		delay(1000);
		if (!mqttclient.connected() && WiFi.status() == WL_CONNECTED) {
	DEBUG_PRINTLN("MQTT disconnected, reconnecting");
			mqttclient.connect();
		} else if (WiFi.status() != WL_CONNECTED) {
			mqttclient.disconnect();
		}
	}
	vTaskDelete(NULL);
}

void Basecamp::MqttReconnect(AsyncMqttClient * mqtt) {
	DEBUG_PRINTLN("MQTT disconnected, reconnecting");
	//while (WiFi.status() != WL_CONNECTED) {
		//delay(1000);
	//}
	//vTaskDelay(1000);
	//mqtt->connect();
	//
	while (1) {
		if (!mqtt->connected() && WiFi.status() == WL_CONNECTED) {
			DEBUG_PRINTLN("MQTT disconnected, reconnecting");
			mqtt->connect();
			vTaskDelay(2000);
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
			//configuration.reset();
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
