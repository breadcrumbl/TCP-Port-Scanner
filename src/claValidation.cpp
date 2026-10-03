#include "claValidation.hpp"

#include <stdio.h>
#include <string>

int claValidation(int argc, char* argv[]) {
    // Extract program name from argv[0] for usage message
    const char* programName = strrchr(argv[0], '\\');
    if (programName != nullptr) {
        ++programName;  // Skip the '\'
    } 
    else {
        programName = argv[0];
    }

    // If correct input, return 0
    if (argc == 3 && argv[1] == "-ip")
        return 0;
    if (argc == 6 && argv[1] == "-ip" && argv[3] == "-p") {
        int startPort = std::stoi(argv[4]); 
        int endPort = std::stoi(argv[5]);
        if (startPort < 0) {
            printf("Minimum start port is 0.");
            return 1;
        }
        if (endPort > 65535) {
            printf("Maximum end port is 65,535.");
            return 1;
        }
        if (startPort > endPort) {
            printf("Start port must be less than or equal to end port.");
            return 1;
        }
        return 0;
    }
    printf("Usage: ./%s -ip <target IP address>\n or\n ./%s -ip <target IP address> -p <start port> <end port> \n", programName, programName);
    return 1;
}
