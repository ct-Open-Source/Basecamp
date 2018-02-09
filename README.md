# Basecamp

Basecamp - ESP32 library to simplify the basics of IoT projects
Originally written by Merlin Schumacher [(mls@ct.de)](mailto://mls@ct.de) for [c't magazin für computer technik](https://www.ct.de)
Licensed under GPLv3. See LICENSE for details.

## Attention: Do not use the master-branch for production use! Use only the releases!

## Dependencies

This library has few dependencies:

[ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)

[ArduinoJSON](https://github.com/bblanchon/ArduinoJson)

[Async MQTT Client](https://github.com/marvinroger/async-mqtt-client)

[AsyncTCP](https://github.com/me-no-dev/AsyncTCP)

## Documentation

Exhaustive documentation will provided in the next few weeks. An example can be found inside the example folder.

### First Setup:
At the first start - when you initially flash the device, the ESP32 will generate an
unique password which is displayed at the debug console upon every start. In setup mode (when the ESP32 is acting as
	an access point for setup), the password for the "ESP_$macOfEsp32" wifi network will be set to this value. It will never change,
	except the configuration gets broken - then a new password will be generated.

## Basic example

```cpp

#include <Basecamp.hpp>
Basecamp iot;

void setup() {
	iot.begin();
    //The mqtt object is an instance of Async MQTT Client. See it's documentation for details.
    iot.mqtt.subscribe("test/lol",2);

    //Use the web object to add elements to the interface
    iot.web.addInterfaceElement("color", "input", "", "#configform", "LampColor");
    iot.web.setInterfaceElementAttribute("color", "type", "text");

}

void loop() {
	//your code
}

```
