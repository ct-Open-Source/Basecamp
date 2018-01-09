#include <Basecamp.hpp>

Basecamp iot;

static const int SensorPin = 33;
static const int BatteryPin = 34;
int sensorValue = 0;
int batteryLimit = 3300;

uint16_t statusPacketIdSub = 0;

void setup() {
  pinMode(SensorPin, INPUT_PULLDOWN);
  pinMode(BatteryPin, INPUT);
  sensorValue = digitalRead(SensorPin);
  iot.begin();
  iot.mqtt.onConnect(transmitStatus);
  iot.mqtt.onPublish(suspendESP);
}

void transmitStatus(bool sessionPresent) {
  if (sensorValue == 0) {

    DEBUG_PRINTLN("Door open");
    statusPacketIdSub = iot.mqtt.publish("stat/frontdoor/status", 1, true, "open" );
    esp_sleep_enable_ext0_wakeup((gpio_num_t)SensorPin, 1);

  } else {

    statusPacketIdSub = iot.mqtt.publish("stat/frontdoor/status", 1, true, "closed" );
    DEBUG_PRINTLN("Door closed");
    esp_sleep_enable_ext0_wakeup((gpio_num_t)SensorPin, 0);

  }

  sensorValue = analogRead(BatteryPin);
  if (sensorValue < batteryLimit) {

    DEBUG_PRINTLN("Battery empty");
    iot.mqtt.publish("stat/frontdoor/battery", 1, true, "empty" );

  } else {

    DEBUG_PRINTLN("Battery full");
    iot.mqtt.publish("stat/frontdoor/battery", 1, true, "full" );

  }
}

void suspendESP(uint16_t packetId) {
  delay(150);
  DEBUG_PRINTLN("Data published");

  if (packetId == statusPacketIdSub) {

    DEBUG_PRINTLN("Entering deep sleep");
    esp_deep_sleep_start();

  }
}

void loop() {
}
