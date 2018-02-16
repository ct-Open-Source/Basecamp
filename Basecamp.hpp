/*
   Basecamp - ESP32 library to simplify the basics of IoT projects
   Written by Merlin Schumacher (mls@ct.de) for c't magazin für computer technik (https://www.ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */

#ifndef Basecamp_h
#define Basecamp_h
#include "debug.hpp"
#include "Configuration.hpp"
#include <Preferences.h>
#include <rom/rtc.h>

#ifndef BASECAMP_NOWIFI
#include "WifiControl.hpp"
#endif

#ifndef BASECAMP_NOWEB
#ifdef BASECAMP_USEDNS
#include <DNSServer.h>
#endif

#include "WebServer.hpp"
#endif

#ifndef BASECAMP_NOMQTT
#include <AsyncMqttClient.h>
#endif

#ifndef BASECAMP_NOOTA
#include <ArduinoOTA.h>
#endif

class Basecamp {
	public:
		// How to handle encryption in setup mode (AP mode)
		enum class SetupModeWifiEncryption
		{
			none,		///< Do not use WiFi encryption (open network)
			secured,	///< Use ESP32 default encryption (WPA2 at this time)
		};

		// When to enable the Configuration UI (setup via local webserver)
		enum class ConfigurationUI
		{
			always,	///< Always start the configuration-ui webserver
			accessPoint,	///< Only start the server if acting as an access  (first setup mode)
		};

		explicit Basecamp(Basecamp::SetupModeWifiEncryption setupModeWifiEncryption =
			Basecamp::SetupModeWifiEncryption::none,
			Basecamp::ConfigurationUI configurationUi = Basecamp::ConfigurationUI::always);

		~Basecamp() = default;

		Configuration configuration;
		Preferences preferences;

		/** Initialize.
		 * Give a fixex ap secret here to override the one-time secret
		 * password generation. If a password is given, the ctor given
		 * SetupModeWifiEncryption will be overriden to SetupModeWifiEncryption::secure.
		*/
		bool begin(String fixedWiFiApEncryptionPassword = {});
		void checkResetReason();
		String showSystemInfo();
		String hostname;
		struct taskParms {
			const char* parm1;
			const char* parm2;
		};
#ifndef BASECAMP_NOWIFI
		String mac;
		WifiControl wifi;
#endif

#ifndef BASECAMP_NOMQTT
		AsyncMqttClient mqtt;
		static void MqttHandling(void *);
#endif

#ifndef BASECAMP_NOWEB

#ifdef BASECAMP_USEDNS
#ifdef DNSServer_h
		DNSServer dnsServer;
		static void DnsHandling(void *);
#endif
#endif
		WebServer web;
#endif

#ifndef BASECAMP_NOOTA
		static void OTAHandling(void *);
#endif
	private:
		String _cleanHostname();
		bool shouldEnableConfigWebserver() const;

		SetupModeWifiEncryption setupModeWifiEncryption_;
		ConfigurationUI configurationUi_;
};
#endif
