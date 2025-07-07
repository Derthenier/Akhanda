#include <Core/Logging/Core.Logging.hpp>
#include <Core/Logging/Core.Spdlog.Integration.hpp>

int main(int argc, char** argv) {
    AKHANDA_SETUP_LOGGING();

    Akhanda::Logging::LogManager::Instance().Initialize();

    auto& testChannel = Akhanda::Logging::LogManager::Instance().GetChannel("test");
    LOG_INFO(testChannel, "Migration Sucessful");
    LOG_SCOPE(testChannel, "Performance test {}", argc);

    return 0;
}