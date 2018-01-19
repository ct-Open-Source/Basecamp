/*
   Basecamp - ESP32 library to simplify the basics of IoT projects
   Written by Merlin Schumacher (mls@ct.de) for c't magazin für computer technik (https://www.ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */
#include "Configuration.hpp"
#include "debug.hpp"

Configuration::Configuration(String filename)
	: _jsonFile(std::move(filename))
{
}

bool Configuration::load() {
	DEBUG_PRINTLN("Loading config file ");
	DEBUG_PRINTLN(_jsonFile);
	if (!SPIFFS.begin(true)) {
		Serial.println("Could not access SPIFFS.");
		return false;

	};

	File configFile = SPIFFS.open(_jsonFile, "r");

	if (!configFile || configFile.isDirectory()) {
		Serial.println("Failed to open config file");
		return false;
	}

	DynamicJsonBuffer _jsonBuffer;
	JsonObject& _jsonData = _jsonBuffer.parseObject(configFile);

	if (!_jsonData.success()) {
		Serial.println("Failed to parse config file.");
		return false;
	};

	for (const auto& configItem : _jsonData) {
		set(configItem.key, configItem.value);
	}

	configFile.close();
	return true;
}

bool Configuration::save() {
	DEBUG_PRINTLN("Saving config file");

	File configFile = SPIFFS.open(_jsonFile, "w");
	if (!configFile) {
		Serial.println("Failed to open config file for writing");
		return false;
	};

	if (configuration.empty())
	{
		Serial.println("Configuration empty");
	};

	DynamicJsonBuffer _jsonBuffer;
	JsonObject& _jsonData = _jsonBuffer.createObject();

	for (auto const& x : configuration)
	{
		_jsonData.set(_jsonBuffer.strdup(x.first), _jsonBuffer.strdup(x.second));
	}

	_jsonData.printTo(configFile);
#ifdef DEBUG
	_jsonData.prettyPrintTo(Serial);
#endif
	configFile.close();
	_configurationTainted = false;
	return true;
}


bool Configuration::set(String key, String value) {
	DEBUG_PRINTLN(key);
	DEBUG_PRINTLN(configuration[key]);
	configuration[key] = value;
	_configurationTainted = true;

	// TODO: Again an unset return code. Check if its used or makes sense.
	return true;
}

const String &Configuration::get(String key) const {
	auto found = configuration.find(key);
	std::ostringstream debug;
	if (found != configuration.end()) {
		debug << "Config value for " << key << ": " << found->second;
		DEBUG_PRINTLN(debug.str().c_str());
		return found->second;
	}

	// Default: if not set, we just return an empty String. TODO: Throw?
	debug << "No config value for " << key;
	DEBUG_PRINTLN(debug.str().c_str());
	return noResult_;
}

void Configuration::reset() {
	configuration.clear();
	this->save();
	this->load();
}

void Configuration::dump() {
#ifdef DEBUG
	for (const auto &p : configuration) {
		Serial.print( "configuration[");
		Serial.print(p.first);
		Serial.print("] = ");
		Serial.println(p.second);
	};
#endif
}
