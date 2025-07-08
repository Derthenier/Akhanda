#include <Core/Logging/Core.Logging.hpp>
#include <Core/Logging/SpdlogIntegration.hpp>

int main(int, char**) {
    AKHANDA_SETUP_LOGGING();

    Akhanda::Logging::LogManager::Instance().Initialize();

    auto& testChannel = Akhanda::Logging::LogManager::Instance().GetChannel("test");
    LOG_INFO(testChannel, "Migration Sucessful");
    LOG_SCOPE(testChannel, "Performance test {}");

    return 0;
}