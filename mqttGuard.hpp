#ifndef BASECAMP_MQTT_GUARD_HPP
#define BASECAMP_MQTT_GUARD_HPP

#include "log.hpp"

#include <functional>
#include <vector>

/**
  Helper to guard outgoing mqtt packets.
  On packet sending, register the packets within this class, unregister them within the onPublish and
  pull the allSent() function to see if everything has been sent completely.
*/
class MqttGuard
{
public:
    /// Typesafety forward of AsyncMqttClients packet_id type
    using IdType = uint16_t;

    /**
        Construct a new guard.
        @param LogCallback Optional log callback.
     */
    explicit MqttGuard(basecampLog::LogCallback logCallback = {});
    ~MqttGuard() = default;

    /**
        Register a new packet that is going to sent by mqtt.
        @param packetId Packet-ID returned by AsyncMqttClient::publish()
     */
    void registerPacket(IdType packetId);

    /**
        Unregister a packet after it has been sent successfully.
        @param packetId Packet-ID returned by AsyncMqttClient::onPublish()
     */
    void unregisterPacket(IdType packetId);

    /// Returns the amount of packets waiting to be sent.
    size_t remainingPacketCount() const;

    /// Returns true if all packets have been sent.
    bool allSent() const;

    /**
        Reset to empty state (force).
        Maybe necessary if getting disconnected.
     */
    void reset();
private:
    /**
        Check a packetId for validity.
        @param packetId Packet-ID to be checked.
        @return True if the packetId can be safely added.
     */
    bool isValidPacketId(IdType packetId) const;

    /// Try to log message if logCallback_ has been set in ctor.
    void tryLog(basecampLog::Severity severity, const std::string& message);

    /// Try to log message with packetId if logCallback_ has been set in ctor.
    void tryLog(basecampLog::Severity severity, const std::string& message, IdType packetId);

    /// Optional callback for log-messages
    basecampLog::LogCallback logCallback_;
    /// List of all remaining packets
    std::vector<IdType> packets_;
};

#endif // #define BASECAMP_MQTT_GUARD_HPP
