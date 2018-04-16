#ifndef BASECAMP_MQTT_GUARD_INTERFACE_HPP
#define BASECAMP_MQTT_GUARD_INTERFACE_HPP

#include "mqttGuard.hpp"

#include <memory>

#include <AsyncMqttClient.h>

/**
    Class to interface between AsyncMqttClient and MqttGuard.
    Intercepts outgoing messages and incoming publish acknowledgements to keep track of packets to be sent.

    Use mqttOnPublish instead of mqtt.OnPublish()
    Use consequently mqttOnDisconnect instead of mqtt.OnDisconnect()
    Use mqttPublish instead of mqtt.publish()

    Use then mqttAllSent() to check if there are remaining packets to be sent.

    Its up to the owner of this class to keep mqttClient valid in the lifetime of an instance of this class.
 */
class MqttGuardInterface
{
public:
    explicit MqttGuardInterface(AsyncMqttClient& mqttClient);

    AsyncMqttClient& mqttOnPublish(AsyncMqttClientInternals::OnPublishUserCallback callback);
    AsyncMqttClient& mqttOnDisconnect(AsyncMqttClientInternals::OnDisconnectUserCallback callback);    

    uint16_t mqttPublish(const char* topic, uint8_t qos, bool retain, const char* payload = nullptr, size_t length = 0, bool dup = false, uint16_t message_id = 0);

    /// Returns true if all mqtt-packets have been sent.
    bool mqttAllSent() const;

    /// Returns the remaining to-be-transmitted packet count.
    size_t mqttRemainingPackets() const;

private:
    AsyncMqttClient& mqttClient_;
    std::shared_ptr<MqttGuard> mqttGuard_;
};

#endif // BASECAMP_MQTT_GUARD_INTERFACE_HPP
