#include "mqttGuard.hpp"

#include <algorithm>
#include <sstream>

MqttGuard::MqttGuard(basecampLog::LogCallback logCallback)
    : logCallback_(std::move(logCallback))
{
}

void MqttGuard::registerPacket(IdType packetId)
{
    if (!isValidPacketId(packetId)) {
        tryLog(basecampLog::Severity::info, "Not registering invalid MQTT-packet.", packetId);
        return;
    }

    packets_.emplace_back(packetId);
}

void MqttGuard::unregisterPacket(IdType packetId)
{
    auto found = std::find(packets_.begin(), packets_.end(), packetId);

    if (found == packets_.end()) {
        tryLog(basecampLog::Severity::info, "Not unregistering unknown MQTT-packet.", packetId);
        return;
    }

    packets_.erase(found);
}

size_t MqttGuard::remainingPacketCount() const
{
    return packets_.size();
}

bool MqttGuard::allSent() const
{
    return (remainingPacketCount() == 0);
}

bool MqttGuard::isValidPacketId(IdType packetId) const
{
    // Generally invalid packet id
    if (packetId == 0)
    {
        return false;
    }

    // Do not allow duplicates
    if (std::find(packets_.begin(), packets_.end(), packetId) != packets_.end())
    {
        return false;
    }

    return true;
}

void MqttGuard::reset()
{
    tryLog(basecampLog::Severity::warning, "MqttGuard has been manually reset.");

    packets_.clear();
}

void MqttGuard::tryLog(basecampLog::Severity severity, const std::string &message)
{
    if (!logCallback_) {
        return;
    }

    logCallback_(severity, message);
}

void MqttGuard::tryLog(basecampLog::Severity severity, const std::string &message, IdType packetId)
{
    if (!logCallback_) {
        return;
    }

    std::ostringstream text;
    text << message << " - Packet-ID: " << packetId;
    logCallback_(severity, text.str());
}
