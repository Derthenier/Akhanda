// Tests/Core.Math/Source/TestConstants.hpp
#pragma once

#include <chrono>
#include <cmath>
#include <limits>
#include <random>
#include <vector>
#include <gtest/gtest.h>

namespace Akhanda::Tests::Math {

    // ============================================================================
    // Mathematical Constants
    // ============================================================================

    namespace Constants {
        // Floating point precision constants
        constexpr float EPSILON = 1e-6f;
        constexpr float LOOSE_EPSILON = 1e-4f;  // For accumulated errors
        constexpr float STRICT_EPSILON = 1e-8f; // For high precision tests

        constexpr double DOUBLE_EPSILON = 1e-12;
        constexpr double DOUBLE_LOOSE_EPSILON = 1e-10;

        // Mathematical constants
        constexpr float PI = 3.14159265358979323846f;
        constexpr float PI_2 = PI / 2.0f;
        constexpr float PI_4 = PI / 4.0f;
        constexpr float TWO_PI = 2.0f * PI;
        constexpr float INV_PI = 1.0f / PI;

        constexpr float E = 2.71828182845904523536f;
        constexpr float SQRT_2 = 1.41421356237309504880f;
        constexpr float SQRT_3 = 1.73205080756887729352f;
        constexpr float INV_SQRT_2 = 1.0f / SQRT_2;
        constexpr float INV_SQRT_3 = 1.0f / SQRT_3;

        // Degree/Radian conversion
        constexpr float DEG_TO_RAD = PI / 180.0f;
        constexpr float RAD_TO_DEG = 180.0f / PI;

        // Common angles in radians
        constexpr float ANGLE_0 = 0.0f;
        constexpr float ANGLE_30 = PI / 6.0f;
        constexpr float ANGLE_45 = PI / 4.0f;
        constexpr float ANGLE_60 = PI / 3.0f;
        constexpr float ANGLE_90 = PI / 2.0f;
        constexpr float ANGLE_120 = 2.0f * PI / 3.0f;
        constexpr float ANGLE_135 = 3.0f * PI / 4.0f;
        constexpr float ANGLE_150 = 5.0f * PI / 6.0f;
        constexpr float ANGLE_180 = PI;
        constexpr float ANGLE_270 = 3.0f * PI / 2.0f;
        constexpr float ANGLE_360 = TWO_PI;

        // Special floating point values
        constexpr float INFINITY_F = std::numeric_limits<float>::infinity();
        constexpr float NEG_INFINITY_F = -std::numeric_limits<float>::infinity();
        constexpr float NAN_F = std::numeric_limits<float>::quiet_NaN();
        constexpr float MAX_FLOAT = std::numeric_limits<float>::max();
        constexpr float MIN_FLOAT = std::numeric_limits<float>::lowest();
        constexpr float DENORM_MIN_FLOAT = std::numeric_limits<float>::denorm_min();
    }

    // ============================================================================
    // Test Vector Collections
    // ============================================================================

    namespace TestVectors {
        // Common test vectors for Vector3
        struct Vector3TestData {
            float x, y, z;
            const char* description;
        };

        constexpr Vector3TestData COMMON_VECTORS[] = {
            { 0.0f, 0.0f, 0.0f, "Zero vector" },
            { 1.0f, 0.0f, 0.0f, "Unit X" },
            { 0.0f, 1.0f, 0.0f, "Unit Y" },
            { 0.0f, 0.0f, 1.0f, "Unit Z" },
            { 1.0f, 1.0f, 1.0f, "All ones" },
            { -1.0f, -1.0f, -1.0f, "All negative ones" },
            { Constants::SQRT_3, Constants::SQRT_3, Constants::SQRT_3, "Length 3 diagonal" },
            { 1.0f, 2.0f, 3.0f, "Sequential" },
            { 3.0f, 4.0f, 5.0f, "Pythagorean" },
            { Constants::MAX_FLOAT, 0.0f, 0.0f, "Max float X" },
            { 0.0f, Constants::MAX_FLOAT, 0.0f, "Max float Y" },
            { 0.0f, 0.0f, Constants::MAX_FLOAT, "Max float Z" },
            { 1e-6f, 1e-6f, 1e-6f, "Very small" },
            { 1e6f, 1e6f, 1e6f, "Very large" },
        };

        // Edge case vectors
        constexpr Vector3TestData EDGE_CASE_VECTORS[] = {
            { Constants::INFINITY_F, 0.0f, 0.0f, "Infinite X" },
            { 0.0f, Constants::NEG_INFINITY_F, 0.0f, "Negative infinite Y" },
            { Constants::NAN_F, 0.0f, 0.0f, "NaN X" },
            { Constants::DENORM_MIN_FLOAT, 0.0f, 0.0f, "Denormalized" },
            { -0.0f, -0.0f, -0.0f, "Negative zero" },
        };

        // Performance test vectors (large datasets)
        constexpr size_t PERF_VECTOR_COUNT = 10000;

        // Generate test vectors for performance tests
        inline std::vector<Vector3TestData> GeneratePerformanceVectors() {
            std::vector<Vector3TestData> vectors;
            vectors.reserve(PERF_VECTOR_COUNT);

            for (size_t i = 0; i < PERF_VECTOR_COUNT; ++i) {
                float t = static_cast<float>(i) / static_cast<float>(PERF_VECTOR_COUNT - 1);
                vectors.push_back({
                    std::sin(t * Constants::TWO_PI),
                    std::cos(t * Constants::TWO_PI),
                    t * 2.0f - 1.0f,
                    "Generated performance vector"
                    });
            }

            return vectors;
        }
    }

    // ============================================================================
    // Test Utilities
    // ============================================================================

    namespace Utils {
        // Floating point comparison with custom epsilon
        inline bool FloatEqual(float a, float b, float epsilon = Constants::EPSILON) {
            if (std::isnan(a) && std::isnan(b)) return true;
            if (std::isinf(a) && std::isinf(b)) return a == b;
            if (std::isnan(a) || std::isnan(b)) return false;
            if (std::isinf(a) || std::isinf(b)) return false;

            return std::abs(a - b) <= epsilon;
        }

        // Double precision comparison
        inline bool DoubleEqual(double a, double b, double epsilon = Constants::DOUBLE_EPSILON) {
            if (std::isnan(a) && std::isnan(b)) return true;
            if (std::isinf(a) && std::isinf(b)) return a == b;
            if (std::isnan(a) || std::isnan(b)) return false;
            if (std::isinf(a) || std::isinf(b)) return false;

            return std::abs(a - b) <= epsilon;
        }

        // Relative floating point comparison (better for large numbers)
        inline bool FloatEqualRelative(float a, float b, float relativeEpsilon = Constants::EPSILON) {
            if (std::isnan(a) && std::isnan(b)) return true;
            if (std::isinf(a) && std::isinf(b)) return a == b;
            if (std::isnan(a) || std::isnan(b)) return false;
            if (std::isinf(a) || std::isinf(b)) return false;

            float maxValue = std::max(std::abs(a), std::abs(b));
            return std::abs(a - b) <= relativeEpsilon * maxValue;
        }

        // Check if a float is "normal" (not NaN, infinity, or denormalized)
        inline bool IsNormalFloat(float value) {
            return std::isfinite(value) && std::isnormal(value);
        }

        // Generate a random float in range [min, max]
        inline float RandomFloat(float min = -1.0f, float max = 1.0f) {
            static thread_local std::random_device rd;
            static thread_local std::mt19937 gen(rd());
            std::uniform_real_distribution<float> dis(min, max);
            return dis(gen);
        }

        // Generate a random normalized vector
        inline void RandomNormalizedVector3(float& x, float& y, float& z) {
            // Generate uniform point on unit sphere using Marsaglia method
            float u1, u2, s;
            do {
                u1 = RandomFloat(-1.0f, 1.0f);
                u2 = RandomFloat(-1.0f, 1.0f);
                s = u1 * u1 + u2 * u2;
            } while (s >= 1.0f || s == 0.0f);

            float factor = 2.0f * std::sqrt(1.0f - s);
            x = u1 * factor;
            y = u2 * factor;
            z = 1.0f - 2.0f * s;
        }
    }


    // ============================================================================
    // Performance Testing Utilities
    // ============================================================================

    namespace Performance {
        // Simple timer for performance measurements
        class ScopedTimer {
        public:
            explicit ScopedTimer(const char* name)
                : name_(name), start_(std::chrono::high_resolution_clock::now()) {
            }

            ~ScopedTimer() {
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_);
                std::cout << "[PERF] " << name_ << ": " << duration.count() << " mu-s" << std::endl;
            }

        private:
            const char* name_;
            std::chrono::high_resolution_clock::time_point start_;
        };

        // Macro for easy performance timing
#define PERF_TIME(name) Performance::ScopedTimer timer(name)

// Run a function multiple times and measure average performance
        template<typename Func>
        double BenchmarkFunction(Func func, size_t iterations = 1000) {
            auto start = std::chrono::high_resolution_clock::now();

            for (size_t i = 0; i < iterations; ++i) {
                func();
            }

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

            return static_cast<double>(duration.count()) / static_cast<double>(iterations);
        }
    }

} // namespace Akhanda::Tests::Math