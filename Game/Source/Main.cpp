#include <thread>
#include <iostream>
#include <Core/Logging/Core.Logging.hpp>
#include <Core/Logging/SpdlogIntegration.hpp>

import ThreadsOfKaliyuga.Logging;
import ThreadsOfKaliyuga.Game;

// ============================================================================
// Application Entry Point
// ============================================================================

int main() {
    try {
        // Create and initialize game
        ThreadsOfKaliyuga::Game game;

        if (!game.Initialize()) {
            ThreadsOfKaliyuga::Logging::ToKLogging::Log(Akhanda::Logging::LogLevel::Error, "Failed to initialize game!");
            return -1;
        }

        // Run the game
        game.Run();

        ThreadsOfKaliyuga::Logging::ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Game completed successfully.");
        return 0;
    }
    catch (const std::exception& e) {
        ThreadsOfKaliyuga::Logging::ToKLogging::Log(Akhanda::Logging::LogLevel::Error, "Game crashed with exception: {}", e.what());
        return -1;
    }
    catch (...) {
        ThreadsOfKaliyuga::Logging::ToKLogging::Log(Akhanda::Logging::LogLevel::Error, "Game crashed with unknown exception!");
        return -1;
    }
}