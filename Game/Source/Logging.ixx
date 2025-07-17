module;

#include <Core/Logging/Core.Logging.hpp>
#include <Core/Logging/SpdlogIntegration.hpp>

export module ThreadsOfKaliyuga.Logging;

import Akhanda.Core.Logging;

namespace ThreadsOfKaliyuga::Logging {
    export class ToKLogging {
    public:
        static bool Initialize() {
#ifdef _DEBUG
            Akhanda::Logging::Integration::ConfigureForDevelopment();
#elif _PROFILE
            Akhanda::Logging::Integration::ConfigureForProfiling();
#else
            Akhanda::Logging::Integration::ConfigureForShippping();
#endif

            Akhanda::Logging::LogManager::Instance().Initialize();

            auto& setup = Akhanda::Logging::Integration::SpdlogSetup::Instance();
            auto filesink = setup.CreateFileSink(L"Logs/threads_of_kaliyuga.log");

            SetupChannels(filesink);

            auto& engineChannel = Akhanda::Logging::LogManager::Instance().GetChannel("Engine");
            LOG_INFO(engineChannel, "Threads of Kaliyuga Logging System initialized successfully.");
            return true;
        }

        static void Shutdown() {
            Akhanda::Logging::LogManager::Instance().Flush();
            Akhanda::Logging::LogManager::Instance().Shutdown();
        }

        /// <summary>
        /// Log to the game channel
        /// </summary>
        /// <param name="level">logging level</param>
        /// <param name="fmt">format string to be logged</param>
        /// <param name="...args">parameters to the format string</param>
        template<typename... Args>
        static void Log(Akhanda::Logging::LogLevel level, std::format_string<Args...> fmt, Args&&... args) {
            auto& channel = Akhanda::Logging::LogManager::Instance().GetChannel("ThreadsOfKaliyuga");
            channel.LogFormat(level, fmt, std::forward<Args>(args)...);
        }

    private:
        static void SetupChannels(std::shared_ptr<spdlog::sinks::sink> fileSink) {
            // Define your main engine channels
            std::vector<std::string> channelNames = {
                "Engine", "Render", "Physics", "Audio", "AI", "Input",
                "Assets", "Network", "Memory", "Threading", "ThreadsOfKaliyuga"
            };

            // Add file logging to all channels
            for (const auto& channelName : channelNames) {
                auto& channel = Akhanda::Logging::LogManager::Instance().GetChannel(channelName);
                channel.AddSink(std::static_pointer_cast<void>(fileSink));

                // Set appropriate log levels per channel
#ifdef _DEBUG
                channel.SetLevel(Akhanda::Logging::LogLevel::Debug);
#else
                channel.SetLevel(Akhanda::Logging::LogLevel::Info);
#endif
            }
        }
    };
}