/*
   Basecamp - ESP32 library to simplify the basics of IoT projects
   Written by Merlin Schumacher (mls@ct.de) for c't magazin für computer technik (https://www.ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */

#ifndef WebServer_h
#define WebServer_h

#include "debug.hpp"

#include <map>
#include <vector>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>

#include "data.hpp"
#include "Configuration.hpp"
#include "WebInterface.hpp"

#ifdef BASECAMP_USEDNS
#ifdef DNSServer_h
#include "CaptiveRequestHandler.hpp"
#endif
#endif

class WebServer {
	public:
		WebServer();
		~WebServer() = default;

		void begin(Configuration &configuration);
		bool addURL(const char* url, const char* content, const char* mimetype);
		void addInterfaceElement(const String &id, String element, String content, String parent = "#configform", String configvariable = "");

		// Sets "key" to "value" in element with id "id" if exists.
		void setInterfaceElementAttribute(const String &id, const String &key, String value);

		struct cmp_str
		{
			bool operator()(String a, String b)
			{
				return strcmp(a.c_str(), b.c_str()) < 0;
			}
		};

		AsyncWebServer server;
	private:
		static void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);
		std::map<const char*, const char* > _URLList;
		bool handleFileRead(char* path);
		char* getContentType(char* filename);

		// Print "request" to serial console for debugging purposes.
		void debugPrintRequest(AsyncWebServerRequest *request);

		int _interfaceElementCounter = 0;
		static std::map<String, String, cmp_str> _interfaceMeta;
		int _typeof(String a){ return 0; };
		int _typeof(int a){ return 1; };
		int _typeof(std::map<String, String, cmp_str> a){ return 2; };

		AsyncEventSource events;
		std::vector<InterfaceElement> interfaceElements;
};

#endif
