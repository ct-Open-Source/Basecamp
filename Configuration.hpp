/*
   Basecamp - ESP32 library to simplify the basics of IoT projects
   Written by Merlin Schumacher (mls@ct.de) for c't magazin für computer technik (https://www.ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */

#ifndef Configuration_h
#define Configuration_h
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>
#include "debug.hpp"
#include "map"

class Configuration {
	public:
		Configuration() {};
		~Configuration() {};

		bool begin(String filename);
		bool load();
		bool save();
		bool dump();
		void reset();

		bool set(String key, String value);
		String get(String key);
		char* getCString(String key);

		struct cmp_str
		{
			bool operator()(String a, String b)
			{
				return strcmp(a.c_str(), b.c_str()) < 0;
			}
		};

		std::map<String, String, cmp_str> configuration;

	private:
		static void CheckConfigStatus(void *);
		String _jsonFile;
		bool _configurationTainted;

};

#endif
