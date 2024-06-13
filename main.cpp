#include <iostream>
#include <fstream>
#include <windows.h>
#include "LeapC.h"

void checkLeapResult(eLeapRS result, const std::string& message) {
    if (result != eLeapRS_Success) {
        std::cerr << "Error: " << message << " (" << result << ")" << std::endl;
        exit(EXIT_FAILURE);
    }
}

void processTrackingEvent(const LEAP_TRACKING_EVENT* trackingEvent, std::ofstream& logFile) {
    logFile << "Frame ID: " << trackingEvent->info.frame_id << std::endl;
    logFile << "Timestamp: " << trackingEvent->info.timestamp << std::endl;
    logFile << "Number of Hands: " << trackingEvent->nHands << std::endl;
    for (uint32_t i = 0; i < trackingEvent->nHands; ++i) {
        const LEAP_HAND* hand = &trackingEvent->pHands[i];
        logFile << "Hand ID: " << hand->id << std::endl;
        logFile << "Hand Type: " << (hand->type == eLeapHandType_Left ? "Left" : "Right") << std::endl;
        logFile << "Palm Position: (" << hand->palm.position.x << ", " << hand->palm.position.y << ", " << hand->palm.position.z << ")" << std::endl;
    }

    if (trackingEvent->nHands > 0) {
        std::cout << std::to_string(trackingEvent->nHands) + " hands detected!" << std::endl;
    }
    else {
        std::cout << "No hands detected." << std::endl;
    }
}

int main() {
    // Open the log file
    std::ofstream logFile("tracking_data.log");
    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file" << std::endl;
        return EXIT_FAILURE;
    }

    // Create a connection configuration with default values
    LEAP_CONNECTION_CONFIG config = { 0 };

    // Create a connection object
    LEAP_CONNECTION connection;
    eLeapRS result = LeapCreateConnection(&config, &connection);
    checkLeapResult(result, "Failed to create connection");

    // Open the connection
    result = LeapOpenConnection(connection);
    checkLeapResult(result, "Failed to open connection");

    bool isConnected = false;
    // Poll the connection for messages and process tracking events
    LEAP_CONNECTION_MESSAGE message;
    while (true) {
        result = LeapPollConnection(connection, 1000, &message);
        if (result != eLeapRS_Success) {
            std::cerr << "Failed to poll connection. Error: " << result << std::endl;
            if (result == eLeapRS_NotConnected) {
                if (isConnected) {
                    std::cerr << "Connection lost, attempting to reconnect..." << std::endl;
                    isConnected = false;
                }
                // Try to reopen the connection
                result = LeapOpenConnection(connection);
                if (result == eLeapRS_Success) {
                    std::cerr << "Reconnected successfully." << std::endl;
                    isConnected = true;
                } else {
                    std::cerr << "Failed to reconnect. Error: " << result << std::endl;
                }
            }
            continue;
        }

        switch (message.type) {
            case eLeapEventType_Tracking:
                processTrackingEvent(message.tracking_event, logFile);
                break;
            case eLeapEventType_Connection:
                std::cout << "Leap Motion connection successfully initialized." << std::endl;
                isConnected = true;
                break;
            default:
                std::cerr << "Unexpected message type: " << message.type << std::endl;
                break;
        }
    }

    // Close the connection
    LeapCloseConnection(connection);
    LeapDestroyConnection(connection);

    // Close the log file
    logFile.close();

    return 0;
}
