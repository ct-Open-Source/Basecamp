#include "EthControl.hpp"


void EthControl::begin(String hostname) {
    ETH.begin();
    ETH.setHostname(hostname.c_str());
    WiFi.onEvent(WiFiEvent);
}

bool EthControl::config(const String& ip, const String& gateway, const String& subnet, const String& dns1, const String& dns2 ) {
    IPAddress mip, mgateway, msubnet, mdns1, mdns2; 
    mip.fromString(ip);
    mgateway.fromString(gateway);
    msubnet.fromString(subnet);
    mdns1.fromString(dns1);
    mdns2.fromString(dns2);

    return config(mip, mgateway, msubnet, mdns1, mdns1);
}


bool EthControl::config(IPAddress ip, IPAddress gateway, IPAddress subnet, IPAddress dns1, IPAddress dns2 )
{
    return ETH.config(ip, gateway, subnet, dns1, dns2);
}


IPAddress EthControl::getIP() {
    return ETH.localIP();
}

String EthControl::getMac() {
    return ETH.macAddress();
}

void EthControl::WiFiEvent(WiFiEvent_t event){
    switch (event) {
        case SYSTEM_EVENT_ETH_START:
            DEBUG_PRINTLN("ETH Started");
            break;
        case SYSTEM_EVENT_ETH_CONNECTED:
            DEBUG_PRINTLN("ETH Connected");
            break;
        case SYSTEM_EVENT_ETH_GOT_IP:
            DEBUG_PRINT("ETH MAC: ");
            DEBUG_PRINT(ETH.macAddress());
            DEBUG_PRINT(", IPv4: ");
            DEBUG_PRINT(ETH.localIP());
            if (ETH.fullDuplex()) {
                DEBUG_PRINT(", FULL_DUPLEX");
            }
            DEBUG_PRINT(", ");
            DEBUG_PRINT(ETH.linkSpeed());
            DEBUG_PRINTLN("Mbps");
            break;
        case SYSTEM_EVENT_ETH_DISCONNECTED:
            DEBUG_PRINTLN("ETH Disconnected");
            break;
        case SYSTEM_EVENT_ETH_STOP:
            DEBUG_PRINTLN("ETH Stopped");
            break;
        default:
            break;
    }
}