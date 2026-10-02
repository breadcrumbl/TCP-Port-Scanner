#include "../include/claValidation.hpp"

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

    // Check CL argument count is correct and if not, print usage message and return 1
    if (argc != 2) {
        printf("Usage: ./%s <target IP address>\n", programName);
        return 1;
    }
    return 0;
}