#pragma once

void scanPorts(int startPort, int endPort, const char* targetIP);
int scanSinglePort(int port, const char* targetIP);