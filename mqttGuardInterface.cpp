#include "mqttGuardInterface.hpp"

MqttGuardInterface::MqttGuardInterface(AsyncMqttClient& mqttClient)
    : mqttClient_(mqttClient)
    , mqttGuard_(std::make_shared<MqttGuard>())
{
    // Default callbacks in case the user does not set one so internally everything is kept fine.
    mqttOnDisconnect([](AsyncMqttClientDisconnectReason /*reason*/){});
    mqttOnPublish([](uint16_t /*packetId*/) {});
}

uint16_t MqttGuardInterface::mqttPublish(const char* topic, uint8_t qos, bool retain,
        const char* payload, size_t length, bool dup, uint16_t message_id)
{
    mqttGuard_->registerPacket(mqttClient_.publish(topic, qos, retain, payload, length, dup, message_id));
}

AsyncMqttClient& MqttGuardInterface::mqttOnPublish(AsyncMqttClientInternals::OnPublishUserCallback callback)
{
    // Intercept the onPublish and make sure that mqttGuard is held alive until the callback is fired.
    auto mqttGuard = mqttGuard_;
    return mqttClient_.onPublish([mqttGuard, callback](uint16_t packetId){
        mqttGuard->unregisterPacket(packetId);
        if (callback) {
            callback(packetId);
        }
    });
}

AsyncMqttClient& MqttGuardInterface::mqttOnDisconnect(AsyncMqttClientInternals::OnDisconnectUserCallback callback)
{
    // Intercept the onDisconnect and make sure that mqttGuard is held alive until the callback is fired.
    auto mqttGuard = mqttGuard_;
    return mqttClient_.onDisconnect([mqttGuard, callback]
                            (AsyncMqttClientDisconnectReason reason){
        // Make sure that disconnet respects that there could be no more packets be sent.
        mqttGuard->reset();
        if (callback) {
            callback(reason);
        }
    });
}

bool MqttGuardInterface::mqttAllSent() const
{
    return mqttGuard_->allSent();
}

size_t MqttGuardInterface::mqttRemainingPackets() const
{
    return mqttGuard_->remainingPacketCount();
}
