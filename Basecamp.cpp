/*
   Basecamp - ESP32 library to simplify the basics of IoT projects
   Written by Merlin Schumacher (mls@ct.de) for c't magazin für computer technik (https://www.ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */
#include "Basecamp.hpp"
#include "debug.hpp"

String Basecamp::_generateHostname() {
	String clean_hostname =	configuration.get("DeviceName");
	if (clean_hostname == "") {
		return "basecamp-device";
	}
	clean_hostname.toLowerCase();
	for (int i = 0; i <= clean_hostname.length(); i++) {
		if (!isalnum(clean_hostname.charAt(i))) {
			clean_hostname.setCharAt(i,'-');
		};
	};
	DEBUG_PRINTLN(clean_hostname);
	return clean_hostname; 
};

bool Basecamp::begin() {
	Serial.begin(115200);
	Serial.println("Basecamp V.0.1.6");
	configuration.begin("/basecamp.json");
	if (!configuration.load()) {
		DEBUG_PRINTLN("Configuration is broken. Resetting.");
		configuration.reset();
	};
	hostname = _generateHostname();
	DEBUG_PRINTLN(hostname);
	checkResetReason();

#ifndef BASECAMP_NOWIFI

	wifi.begin(
			configuration.get("WifiEssid"),
			configuration.get("WifiPassword"),
			configuration.get("WifiConfigured"),
			hostname
		  );

	mac = _generateMac();
	DEBUG_PRINTLN(mac);
#endif
#ifndef BASECAMP_NOMQTT
	if (configuration.get("MQTTActive") != "false") {
		uint16_t mqttport = configuration.get("MQTTPort").toInt();
		char* mqtthost = configuration.getCString("MQTTHost");
		char* mqttuser = configuration.getCString("MQTTUser");
		char* mqttpass = configuration.getCString("MQTTPass");
		mqtt.setClientId(hostname.c_str());
		mqtt.setServer(mqtthost, mqttport);
		if(mqttuser != "") {
			mqtt.setCredentials(mqttuser,mqttpass);
		};

		xTaskCreatePinnedToCore(&MqttHandling, "MqttTask", 4096, (void*) &mqtt, 5, NULL,0);
	};
#endif

#ifndef BASECAMP_NOOTA
	if(configuration.get("OTAActive") != "false") {
		struct taskParms OTAParams[1];
		OTAParams[0].parm1 = configuration.getCString("OTAPass");
		OTAParams[0].parm2 = hostname.c_str();
		xTaskCreatePinnedToCore(&OTAHandling, "ArduinoOTATask", 4096, (void*)&OTAParams[0], 5, NULL,0);
	}
#endif

#ifndef BASECAMP_NOWEB
	web.begin(configuration);

	web.addInterfaceElement("heading", "h1", configuration.get("DeviceName"),"#wrapper");
	web.setInterfaceElementAttribute("heading", "class", "fat-border");

	web.addInterfaceElement("infotext1", "p", "Configure your device with the following options:","#wrapper");

	web.addInterfaceElement("configform", "form", "","#wrapper");
	web.setInterfaceElementAttribute("configform", "action", "saveConfig");

	web.addInterfaceElement("DeviceName", "input", "Device name","#configform" , "DeviceName");

	web.addInterfaceElement("WifiEssid", "input", "WIFI SSID:","#configform" , "WifiEssid");
	web.addInterfaceElement("WifiPassword", "input", "WIFI Password:", "#configform", "WifiPassword");
	web.setInterfaceElementAttribute("WifiPassword", "type", "password");
	web.addInterfaceElement("WifiConfigured", "input", "", "#configform", "WifiConfigured");
	web.setInterfaceElementAttribute("WifiConfigured", "type", "hidden");
	web.setInterfaceElementAttribute("WifiConfigured", "value", "true");
	if (configuration.get("MQTTActive") != "false") {
		web.addInterfaceElement("MQTTHost", "input", "MQTT Host:","#configform" , "MQTTHost");
		web.addInterfaceElement("MQTTPort", "input", "MQTT Port:","#configform" , "MQTTPort");
		web.setInterfaceElementAttribute("MQTTPort", "type", "number");
		web.addInterfaceElement("MQTTUser", "input", "MQTT Username:","#configform" , "MQTTUser");
		web.addInterfaceElement("MQTTPass", "input", "MQTT Password:","#configform" , "MQTTPass");
		web.setInterfaceElementAttribute("MQTTPass", "type", "password");
	}
	web.addInterfaceElement("saveform", "input", " ","#configform");
	web.setInterfaceElementAttribute("saveform", "type", "button");
	web.setInterfaceElementAttribute("saveform", "value", "Save");
	web.setInterfaceElementAttribute("saveform", "onclick", "collectConfiguration()");

	String infotext2 = "This device has the MAC-Address: " + mac;
	web.addInterfaceElement("infotext2", "p", infotext2,"#wrapper");
#ifdef DNSServer_h
	if(configuration.get("WifiConfigured") != "True"){

    		dnsServer.start(53, "*", wifi.getSoftAPIP());
	}
	xTaskCreatePinnedToCore(&DnsHandling, "DNSTask", 4096, (void*) &dnsServer, 5, NULL,0);
	#endif
#endif

	Serial.println(showSystemInfo());
}


#ifndef BASECAMP_NOMQTT

void Basecamp::MqttHandling(void * mqttPointer) {

		bool mqttIsConnecting = false;
		int loopCount = 0;
		AsyncMqttClient * mqtt = (AsyncMqttClient *) mqttPointer;
		while(1) {
			if (loopCount == 50 && mqtt->connected() != 1) {
				mqttIsConnecting = false;
				mqtt->disconnect(true);
			}
			if (!mqttIsConnecting) {
				if(mqtt->connected() != 1) {
					if (WiFi.status() == WL_CONNECTED) {
						mqtt->connect();
						mqttIsConnecting == true;
					} else {
						mqtt->disconnect();
					}
				}
			}
			loopCount++;
			vTaskDelay(100);
		}
};
#endif

#ifdef DNSServer_h
void Basecamp::DnsHandling(void * dnsServerPointer) {

		DNSServer * dnsServer = (DNSServer *) dnsServerPointer;
		while(1) {
			dnsServer->processNextRequest();
			vTaskDelay(100);
		}
};
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
void Basecamp::OTAHandling(void * OTAParams) {

	struct taskParms *params;
	params = (struct taskParms *) OTAParams;

	if(params->parm1 != "") { 
		ArduinoOTA.setPassword(params->parm1);
	}
	ArduinoOTA.setHostname(params->parm2);	
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

String Basecamp::_generateMac() {
	byte rawMac[6];
	WiFi.macAddress(rawMac);
	String mac;
	for (int i = 0; i < 6; i++) {
		mac += (String(rawMac[i], HEX));
		if (i < 5) {
			mac+=":";
		}
	}
	return mac; 
}

String Basecamp::showSystemInfo() {
	String Info = "";
	Info += "MAC-Address: " + mac;
	return Info;
}
