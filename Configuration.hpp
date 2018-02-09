/*
   Basecamp - ESP32 library to simplify the basics of IoT projects
   Written by Merlin Schumacher (mls@ct.de) for c't magazin für computer technik (https://www.ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */

#ifndef Configuration_h
#define Configuration_h

#include "debug.hpp"

#include <sstream>
#include <list>
#include <map>
#include <ArduinoJson.h>
#include <SPIFFS.h>

// TODO: Extend with all known keys
enum class ConfigurationKey {
	accessPointSecret,
};

// TODO: Extend with all known keys
static const String getKeyName(ConfigurationKey key)
{
	// This automatically will break the compiler if a known key has been forgotten
	// (if the warnings are turned on exactly...)
	switch (key)
	{
		case ConfigurationKey::accessPointSecret:
			return "APSecret";
			break;
	}
	return "";
}

class Configuration {
	public:
		explicit Configuration(String filename);
		~Configuration() = default;

		const String& getKey(ConfigurationKey configKey) const;

		bool load();
		bool save();
		void dump();

		// Returns true if the key 'key' exists
		bool keyExists(const String& key) const;

		// Returns true if the key 'key' exists
		bool keyExists(ConfigurationKey key) const;

		// Returns true if the key 'key' exists and is not empty
		bool isKeySet(ConfigurationKey key) const;

		// Reset the whole configuration
		void reset();

		// Reset everything except the AP secret
		void resetExcept(const std::list<ConfigurationKey> &keysToPreserve);

		// FIXME: Get rid of every direct access ("name") set() and get()
		// to minimize the rist of unknown-key usage. Move to private.
		void set(String key, String value);
		// FIXME: use this instead
		void set(ConfigurationKey key, String value);

		// FIXME: Get rid of every direct access ("name") set() and get()
		// to minimize the rist of unknown-key usage. Move to private.
		const String& get(String key) const;
		// FIXME: use this instead
		const String& get(ConfigurationKey key) const;
		char* getCString(String key);
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
