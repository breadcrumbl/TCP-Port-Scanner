#pragma once

#include <vector>

enum class PortStatus {
    unscanned = -1,
    open,
    closed,
    filtered,
    error
};

struct Port {
    int number{};
    PortStatus status{};
};

namespace singleThread {
void scanPorts(int startPort, int endPort, const char* targetIP);
PortStatus scanSinglePort(int port, const char* targetIP);
};


namespace multiThread {
std::vector<Port> scanPorts(int startPort, int endPort, const char* targetIP);
Port scanSinglePort(int port, const char* targetIP);
void printPortArray(std::vector<Port>& portArray);
};
