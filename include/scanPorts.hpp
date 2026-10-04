#pragma once

enum class PortStatus {
    open,
    closed,
    filtered,
    error
};

void scanPorts(int startPort, int endPort, const char* targetIP);
PortStatus scanSinglePort(int port, const char* targetIP);