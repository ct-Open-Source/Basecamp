/*
   Basecamp - ESP32 library to simplify the basics of IoT projects
   Written by Merlin Schumacher (mls@ct.de) for c't magazin für computer technik (https://www.ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */
#include "Configuration.hpp"

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
	}

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
	}

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
	}

	if (configuration.empty())
	{
		Serial.println("Configuration empty");
	}

	DynamicJsonBuffer _jsonBuffer;
	JsonObject &_jsonData = _jsonBuffer.createObject();

	for (const auto& x : configuration)
	{
		_jsonData.set(x.first, String{x.second});
	}

	_jsonData.printTo(configFile);
#ifdef DEBUG
	_jsonData.prettyPrintTo(Serial);
#endif
	configFile.close();
	_configurationTainted = false;
	return true;
}

void Configuration::set(String key, String value) {
	std::ostringstream debug;
	debug << "Settting " << key.c_str() << " to " << value.c_str() << "(was " << get(key).c_str() << ")";
	DEBUG_PRINTLN(debug.str().c_str());

	if (get(key) != value) {
		_configurationTainted = true;
		configuration[key] = value;
	} else {
		DEBUG_PRINTLN("Cowardly refusing to overwrite existing key with the same value");
	}
}

void Configuration::set(ConfigurationKey key, String value)
{
	set(getKeyName(key), std::move(value));
}

const String &Configuration::get(String key) const
{
	auto found = configuration.find(key);
	if (found != configuration.end()) {
		std::ostringstream debug;
		debug << "Config value for " << key.c_str() << ": " << found->second.c_str();
		DEBUG_PRINTLN(debug.str().c_str());

		return found->second;
	}

	// Default: if not set, we just return an empty String. TODO: Throw?
	return noResult_;
}

const String &Configuration::get(ConfigurationKey key) const
{
	return get(getKeyName(key));
}

// return a char* instead of a Arduino String to maintain backwards compatibility
// with printed examples
[[deprecated("getCString() is deprecated. Use get() instead")]]
char* Configuration::getCString(String key)
{
	char *newCString = (char*) malloc(configuration[key].length()+1);
	strcpy(newCString,get(key).c_str());
	return newCString;
}

bool Configuration::keyExists(const String& key) const
{
	return (configuration.find(key) != configuration.end());
}

bool Configuration::keyExists(ConfigurationKey key) const
{
	return (configuration.find(getKeyName(key)) != configuration.end());
}

bool Configuration::isKeySet(ConfigurationKey key) const
{
	auto found = configuration.find(getKeyName(key));
	if (found == configuration.end())
	{
		return false;
	}

	return (found->second.length() > 0);
}

void Configuration::reset()
{
	configuration.clear();
	this->save();
	this->load();
}

void Configuration::resetExcept(const std::list<ConfigurationKey> &keysToPreserve)
{
	std::map<ConfigurationKey, String> preservedKeys;
	for (const auto &key : keysToPreserve) {
		if (keyExists(key)) {
			// Make a copy of the old value
			preservedKeys[key] = get(key);
		}
	}

	configuration.clear();

	for (const auto &key : preservedKeys) {
		set(key.first, key.second);
	}

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
	}
#endif
}
