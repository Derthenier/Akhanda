#include <thread>
#include <iostream>
#include <Core/Logging/Core.Logging.hpp>
#include <Core/Logging/SpdlogIntegration.hpp>

import ThreadsOfKaliyuga.Logging;

import Akhanda.Core.Math;

int main(int, char**) {
    
    ThreadsOfKaliyuga::Logging::ToKLogging::Initialize();

    ThreadsOfKaliyuga::Logging::ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "Hello World!");

    Akhanda::Math::Vector2 v2(1.0f, 2.0f);
    Akhanda::Math::Vector2 v3(2.0f, 3.0f);

    Akhanda::Math::Vector2 v1 = v2 + v3;

    ThreadsOfKaliyuga::Logging::ToKLogging::Log(Akhanda::Logging::LogLevel::Info, "{{ {}, {} }}", v1.x, v1.y);
    
    ThreadsOfKaliyuga::Logging::ToKLogging::Shutdown();

    return 0;
}