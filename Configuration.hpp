/*
   Basecamp - ESP32 library to simplify the basics of IoT projects
   Written by Merlin Schumacher (mls@ct.de) for c't magazin für computer technik (https://www.ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */

#ifndef Configuration_h
#define Configuration_h

#include <sstream>
#include <map>

#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>
#include "debug.hpp"

class Configuration {
	public:
		explicit Configuration(String filename);
		~Configuration() = default;

		bool load();
		bool save();
		void dump();
		void reset();

		bool set(String key, String value);
		const String& get(String key) const;

		struct cmp_str
		{
			bool operator()(const String &a, const String &b) const
			{
				return strcmp(a.c_str(), b.c_str()) < 0;
			}
		};

		std::map<String, String, cmp_str> configuration;

	private:
		static void CheckConfigStatus(void *);
		String _jsonFile;
		bool _configurationTainted = false;
		String noResult_ = {};
};

#endif
