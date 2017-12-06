#define DEBUG 1
#include <Basecamp.hpp>

Basecamp iot;

void setup() {
  iot.begin();
}

void loop() {
	iot.mqtt.publish("frontdoor/status", 1, true, "open");
}
