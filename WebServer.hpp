/*
   Basecamp - ESP32 library to simplify the basics of IoT projects
   Written by Merlin Schumacher (mls@ct.de) for c't magazin für computer technik (https://www.ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */

#ifndef WebServer_h
#define WebServer_h

#include <ESPAsyncWebServer.h>
#include "data.hpp"
#include "debug.hpp"
#include "map"
#include "unordered_map"
#include "vector"
#include <SPIFFS.h>
#include <ESPmDNS.h>
#include "Configuration.hpp"
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include "WebInterface.hpp"

class WebServer {
	public:
		WebServer() {}
		~WebServer() {}
		AsyncWebServer *server;
		AsyncWebSocket *ws;
		AsyncEventSource *events;
		void begin(Configuration &configuration);
		bool addURL(const char* url, const char* content, const char* mimetype);
		void addInterfaceElement(String id, String element, String content, String parent = "#configform", String configvariable = "");
		void setInterfaceElementAttribute(String id, String key, String value);
		interfaceElement* getInterfaceElement(String id);

		struct cmp_str
		{
			bool operator()(String a, String b)
			{
				return strcmp(a.c_str(), b.c_str()) < 0;
			}
		};

		std::vector<interfaceElement*> interfaceElements;

	private:
		static void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);
		std::map<const char*, const char* > _URLList;
		bool handleFileRead(char* path);
		char* getContentType(char* filename);
		int _interfaceElementCounter = 0;
		static std::map<String, String, cmp_str> _interfaceMeta;
		int _typeof(String a){ return 0; };
		int _typeof(int a){ return 1; };
		int _typeof(std::map<String, String, cmp_str> a){ return 2; };
};

#endif
