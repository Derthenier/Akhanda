// Tests/Core.Math/Source/Main.cpp
// Main entry point for Core.Math tests

#include <gtest/gtest.h>
#include <iostream>
#include <filesystem>

// Test environment setup
class MathTestEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        std::cout << "Setting up Core.Math test environment...\n";

        // Verify test data directory exists
        std::filesystem::path testDataPath = "Data";
        if (!std::filesystem::exists(testDataPath)) {
            std::cerr << "Warning: Test data directory not found: " << testDataPath << "\n";
        }

        // Enable floating point exceptions for more robust testing
#ifdef _WIN32
        _controlfp_s(nullptr, 0, _MCW_EM); // Enable all floating point exceptions
#endif

        std::cout << "Core.Math test environment ready.\n";
    }

    void TearDown() override {
        std::cout << "Cleaning up Core.Math test environment...\n";

        // Restore floating point control
#ifdef _WIN32
        _controlfp_s(nullptr, _CW_DEFAULT, _MCW_EM); // Restore default FP behavior
#endif

        std::cout << "Core.Math test environment cleanup complete.\n";
    }
};


// Test environment setup
class ShaderTestEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        std::cout << "Setting up Renderer.Shader test environment...\n";

        // Verify test data directory exists
        std::filesystem::path testDataPath = "Data";
        if (!std::filesystem::exists(testDataPath)) {
            std::cerr << "Warning: Test data directory not found: " << testDataPath << "\n";
        }

        // Enable floating point exceptions for more robust testing
#ifdef _WIN32
        _controlfp_s(nullptr, 0, _MCW_EM); // Enable all floating point exceptions
#endif

        std::cout << "Renderer.Shader test environment ready.\n";
    }

    void TearDown() override {
        std::cout << "Cleaning up Renderer.Shader test environment...\n";

        // Restore floating point control
#ifdef _WIN32
        _controlfp_s(nullptr, _CW_DEFAULT, _MCW_EM); // Restore default FP behavior
#endif

        std::cout << "Renderer.Shader test environment cleanup complete.\n";
    }
};

// Custom test event listener for detailed reporting
class MathTestEventListener : public ::testing::EmptyTestEventListener {
private:
    void OnTestStart(const ::testing::TestInfo& test_info) override {
        std::cout << "[ RUNNING  ] " << test_info.test_suite_name()
            << "." << test_info.name() << "\n";
    }

    void OnTestEnd(const ::testing::TestInfo& test_info) override {
        if (test_info.result()->Passed()) {
            std::cout << "[       OK ] " << test_info.test_suite_name()
                << "." << test_info.name()
                << " (" << test_info.result()->elapsed_time() << " ms)" << "\n";
        }
        else {
            std::cout << "[  FAILED  ] " << test_info.test_suite_name()
                << "." << test_info.name()
                << " (" << test_info.result()->elapsed_time() << " ms)" << "\n";
        }
    }

    void OnTestSuiteStart(const ::testing::TestSuite& test_suite) override {
        std::cout << "[----------] " << test_suite.test_to_run_count()
            << " tests from " << test_suite.name() << "\n";
    }

    void OnTestSuiteEnd(const ::testing::TestSuite& test_suite) override {
        std::cout << "[----------] " << test_suite.test_to_run_count()
            << " tests from " << test_suite.name()
            << " (" << test_suite.elapsed_time() << " ms total)" << "\n";
    }
};

int main(int argc, char** argv) {
    // Initialize Google Test
    ::testing::InitGoogleTest(&argc, argv);

    // Add our custom environment
    ::testing::AddGlobalTestEnvironment(new MathTestEnvironment);
    ::testing::AddGlobalTestEnvironment(new ShaderTestEnvironment);

    // Add custom event listener for detailed output
    ::testing::TestEventListeners& listeners = ::testing::UnitTest::GetInstance()->listeners();
    delete listeners.Release(listeners.default_result_printer());
    listeners.Append(new MathTestEventListener);

    // Configure test execution
    std::cout << "========================================\n";
    std::cout << "Akhanda Engine - Core.Math Test Suite\n";
    std::cout << "========================================\n\n";

    // Print system information
    std::cout << "System Information:\n";
    std::cout << "  - Compiler: " <<
#ifdef _MSC_VER
        "MSVC " << _MSC_VER
#elif defined(__GNUC__)
        "GCC " << __GNUC__ << "." << __GNUC_MINOR__
#else
        "Unknown"
#endif
        << "\n";

    std::cout << "  - Architecture: " <<
#ifdef _M_X64
        "x64"
#elif defined(_M_IX86)
        "x86"
#else
        "Unknown"
#endif
        << "\n";

    std::cout << "  - SIMD Support: " <<
#ifdef __AVX2__
        "AVX2"
#elif defined(__AVX__)
        "AVX"
#elif defined(__SSE4_1__)
        "SSE4.1"
#elif defined(__SSE2__)
        "SSE2"
#else
        "None detected"
#endif
        << "\n";

    std::cout << "\n";

    // Run all tests
    int result = RUN_ALL_TESTS();

    // Print summary
    std::cout << "\n========================================\n";
    if (result == 0) {
        std::cout << "All Core.Math tests PASSED!\n";
    }
    else {
        std::cout << "Some Core.Math tests FAILED!\n";
    }
    std::cout << "========================================\n";

    return result;
}