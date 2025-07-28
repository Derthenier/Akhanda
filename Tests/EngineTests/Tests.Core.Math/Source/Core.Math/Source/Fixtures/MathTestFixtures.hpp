// Tests/Core.Math/Source/Fixtures/MathTestFixtures.hpp
#pragma once

#include <gtest/gtest.h>
#include <random>
#include <vector>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

import Akhanda.Core.Math;
using json = nlohmann::json;

namespace Akhanda::Tests::Math {

    // ============================================================================
    // Base Math Test Fixture
    // ============================================================================

    class MathTestFixture : public ::testing::Test {
    protected:
        void SetUp() override {
            // Initialize random number generator with fixed seed for reproducible tests
            rng_.seed(42);

            // Load test data if available
            LoadTestData();
        }

        void TearDown() override {
            // Cleanup any resources
        }

        // Generate random float in range
        float RandomFloat(float min = -1.0f, float max = 1.0f) {
            std::uniform_real_distribution<float> dist(min, max);
            return dist(rng_);
        }

        // Generate random integer in range
        int RandomInt(int min = -100, int max = 100) {
            std::uniform_int_distribution<int> dist(min, max);
            return dist(rng_);
        }

        // Create a random Vector3
        Akhanda::Math::Vector3 RandomVector3(float min = -10.0f, float max = 10.0f) {
            return Akhanda::Math::Vector3(
                RandomFloat(min, max),
                RandomFloat(min, max),
                RandomFloat(min, max)
            );
        }

        // Create a random normalized Vector3
        Akhanda::Math::Vector3 RandomNormalizedVector3() {
            Akhanda::Math::Vector3 v;
            do {
                v = RandomVector3(-1.0f, 1.0f);
            } while (Akhanda::Math::Length(v) < 1e-6f); // Avoid zero vector

            return Akhanda::Math::Normalize(v);
        }

        // Load test data from JSON files
        virtual void LoadTestData() {
            // Override in derived classes to load specific test data
        }

    protected:
        std::mt19937 rng_;
        json testData_;
    };

    // ============================================================================
    // Vector Test Fixture
    // ============================================================================

    class Vector3TestFixture : public MathTestFixture {
    protected:
        void SetUp() override {
            MathTestFixture::SetUp();

            // Initialize common test vectors
            zero = Akhanda::Math::Vector3(0.0f, 0.0f, 0.0f);
            unitX = Akhanda::Math::Vector3(1.0f, 0.0f, 0.0f);
            unitY = Akhanda::Math::Vector3(0.0f, 1.0f, 0.0f);
            unitZ = Akhanda::Math::Vector3(0.0f, 0.0f, 1.0f);
            ones = Akhanda::Math::Vector3(1.0f, 1.0f, 1.0f);

            // Common test vectors
            diagonal = Akhanda::Math::Vector3(1.0f, 1.0f, 1.0f);
            negativeOnes = Akhanda::Math::Vector3(-1.0f, -1.0f, -1.0f);
            sequential = Akhanda::Math::Vector3(1.0f, 2.0f, 3.0f);
            pythagorean = Akhanda::Math::Vector3(3.0f, 4.0f, 5.0f); // Length should be sqrt(50)

            // Generate random test vectors
            GenerateRandomVectors();
        }

        void LoadTestData() override {
            try {
                std::filesystem::path dataPath = "Data/TestVectors.json";
                if (std::filesystem::exists(dataPath)) {
                    std::ifstream file(dataPath);
                    file >> testData_;
                }
            }
            catch (...) {
                // Ignore errors - tests should work without external data
            }
        }

        void GenerateRandomVectors() {
            randomVectors.reserve(100);
            normalizedVectors.reserve(50);

            for (int i = 0; i < 100; ++i) {
                randomVectors.push_back(RandomVector3(-100.0f, 100.0f));
            }

            for (int i = 0; i < 50; ++i) {
                normalizedVectors.push_back(RandomNormalizedVector3());
            }
        }

    protected:
        // Common test vectors
        Akhanda::Math::Vector3 zero, unitX, unitY, unitZ, ones;
        Akhanda::Math::Vector3 diagonal, negativeOnes, sequential, pythagorean;

        // Collections of random vectors
        std::vector<Akhanda::Math::Vector3> randomVectors;
        std::vector<Akhanda::Math::Vector3> normalizedVectors;
    };

    // ============================================================================
    // Matrix Test Fixture
    // ============================================================================

    class Matrix4TestFixture : public MathTestFixture {
    protected:
        void SetUp() override {
            MathTestFixture::SetUp();

            // Initialize common test matrices
            identity = Akhanda::Math::Matrix4::IDENTITY;
            zero = Akhanda::Math::Matrix4::ZERO;

            // Common transformation matrices
            translation = Akhanda::Math::Translation({ 1.0f, 2.0f, 3.0f });
            scaling = Akhanda::Math::Scale({ 2.0f, 3.0f, 4.0f });
            rotationX = Akhanda::Math::RotationX(Akhanda::Math::QUARTER_PI);
            rotationY = Akhanda::Math::RotationY(Akhanda::Math::QUARTER_PI);
            rotationZ = Akhanda::Math::RotationZ(Akhanda::Math::QUARTER_PI);

            // Generate random matrices
            GenerateRandomMatrices();
        }

        void LoadTestData() override {
            try {
                std::filesystem::path dataPath = "Data/TestMatrices.json";
                if (std::filesystem::exists(dataPath)) {
                    std::ifstream file(dataPath);
                    file >> testData_;
                }
            }
            catch (...) {
                // Ignore errors
            }
        }

        void GenerateRandomMatrices() {
            randomMatrices.reserve(50);

            for (int i = 0; i < 50; ++i) {
                Akhanda::Math::Matrix4 m;
                for (int j = 0; j < 16; ++j) {
                    m.m[j] = RandomFloat(-10.0f, 10.0f);
                }
                randomMatrices.push_back(m);
            }
        }

        // Helper to create a random orthogonal matrix
        Akhanda::Math::Matrix4 RandomOrthogonalMatrix() {
            // Create rotation matrix from random axis and angle
            Akhanda::Math::Vector3 axis = RandomNormalizedVector3();
            float angle = RandomFloat(0.0f, Akhanda::Math::TWO_PI);
            Akhanda::Math::Quaternion q{ axis, angle };
            return Akhanda::Math::Transform(Akhanda::Math::Vector3::ZERO, q, Akhanda::Math::Vector3::ONE).ToMatrix();
        }

    protected:
        // Common test matrices
        Akhanda::Math::Matrix4 identity, zero;
        Akhanda::Math::Matrix4 translation, scaling;
        Akhanda::Math::Matrix4 rotationX, rotationY, rotationZ;

        // Collections of random matrices
        std::vector<Akhanda::Math::Matrix4> randomMatrices;
    };

    // ============================================================================
    // Quaternion Test Fixture
    // ============================================================================

    class QuaternionTestFixture : public MathTestFixture {
    protected:
        void SetUp() override {
            MathTestFixture::SetUp();

            // Initialize common test quaternions
            identity = Akhanda::Math::Quaternion::IDENTITY;

            // Common rotations
            rotX90 = Akhanda::Math::Quaternion::FromAxisAngle(Akhanda::Math::Vector3(1, 0, 0), Akhanda:: Math::HALF_PI);
            rotY90 = Akhanda::Math::Quaternion::FromAxisAngle(Akhanda::Math::Vector3(0, 1, 0), Akhanda:: Math::HALF_PI);
            rotZ90 = Akhanda::Math::Quaternion::FromAxisAngle(Akhanda::Math::Vector3(0, 0, 1), Akhanda:: Math::HALF_PI);

            rot45Diagonal = Akhanda::Math::Quaternion::FromAxisAngle(
                Akhanda::Math::Normalize(Akhanda::Math::Vector3(1, 1, 1)),
                Akhanda:: Math::QUARTER_PI
            );

            GenerateRandomQuaternions();
        }

        void LoadTestData() override {
            try {
                std::filesystem::path dataPath = "Data/TestQuaternions.json";
                if (std::filesystem::exists(dataPath)) {
                    std::ifstream file(dataPath);
                    file >> testData_;
                }
            }
            catch (...) {
                // Ignore errors
            }
        }

        void GenerateRandomQuaternions() {
            randomQuaternions.reserve(50);
            normalizedQuaternions.reserve(50);

            for (int i = 0; i < 50; ++i) {
                // Random quaternion (not necessarily normalized)
                Akhanda::Math::Quaternion q(
                    RandomFloat(-2.0f, 2.0f),
                    RandomFloat(-2.0f, 2.0f),
                    RandomFloat(-2.0f, 2.0f),
                    RandomFloat(-2.0f, 2.0f)
                );
                randomQuaternions.push_back(q);

                // Random normalized quaternion (represents valid rotation)
                Akhanda::Math::Vector3 axis = RandomNormalizedVector3();
                float angle = RandomFloat(0.0f, Akhanda:: Math::TWO_PI);
                normalizedQuaternions.push_back(Akhanda::Math::Quaternion::FromAxisAngle(axis, angle));
            }
        }

    protected:
        // Common test quaternions
        Akhanda::Math::Quaternion identity;
        Akhanda::Math::Quaternion rotX90, rotY90, rotZ90;
        Akhanda::Math::Quaternion rot45Diagonal;

        // Collections of random quaternions
        std::vector<Akhanda::Math::Quaternion> randomQuaternions;
        std::vector<Akhanda::Math::Quaternion> normalizedQuaternions;
    };

    // ============================================================================
    // Performance Test Fixture
    // ============================================================================

    class PerformanceTestFixture : public MathTestFixture {
    protected:
        void SetUp() override {
            MathTestFixture::SetUp();

            // Generate large datasets for performance testing
            GeneratePerformanceData();
        }

        void GeneratePerformanceData() {
            constexpr size_t PERF_DATA_SIZE = 10000;

            perfVectors.reserve(PERF_DATA_SIZE);
            perfMatrices.reserve(PERF_DATA_SIZE);
            perfQuaternions.reserve(PERF_DATA_SIZE);

            for (size_t i = 0; i < PERF_DATA_SIZE; ++i) {
                perfVectors.push_back(RandomVector3(-100.0f, 100.0f));

                Akhanda::Math::Matrix4 m;
                for (int j = 0; j < 16; ++j) {
                    m.m[j] = RandomFloat(-10.0f, 10.0f);
                }
                perfMatrices.push_back(m);

                Akhanda::Math::Vector3 axis = RandomNormalizedVector3();
                float angle = RandomFloat(0.0f, Akhanda:: Math::TWO_PI);
                perfQuaternions.push_back(Akhanda::Math::Quaternion::FromAxisAngle(axis, angle));
            }
        }

        // Benchmark a function and return average time in nanoseconds
        template<typename Func>
        double BenchmarkFunction(Func func, size_t iterations = 1000) {
            // Warm up
            for (size_t i = 0; i < 10; ++i) {
                func();
            }

            auto start = std::chrono::high_resolution_clock::now();

            for (size_t i = 0; i < iterations; ++i) {
                func();
            }

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

            return static_cast<double>(duration.count()) / static_cast<double>(iterations);
        }

        // Run a performance comparison between two functions
        template<typename Func1, typename Func2>
        void ComparePerformance(const char* name1, Func1 func1,
            const char* name2, Func2 func2,
            size_t iterations = 1000) {
            double time1 = BenchmarkFunction(func1, iterations);
            double time2 = BenchmarkFunction(func2, iterations);

            std::cout << "[PERF] " << name1 << ": " << time1 << " ns/op" << std::endl;
            std::cout << "[PERF] " << name2 << ": " << time2 << " ns/op" << std::endl;

            if (time1 < time2) {
                std::cout << "[PERF] " << name1 << " is " << (time2 / time1) << "x faster" << std::endl;
            }
            else {
                std::cout << "[PERF] " << name2 << " is " << (time1 / time2) << "x faster" << std::endl;
            }
        }

    protected:
        // Large datasets for performance testing
        std::vector<Akhanda::Math::Vector3> perfVectors;
        std::vector<Akhanda::Math::Matrix4> perfMatrices;
        std::vector<Akhanda::Math::Quaternion> perfQuaternions;
    };

    // ============================================================================
    // Parameterized Test Fixtures
    // ============================================================================

    // Fixture for testing with multiple floating point precisions
    class FloatPrecisionTestFixture : public MathTestFixture,
        public ::testing::WithParamInterface<float> {
    protected:
        float GetEpsilon() const { return GetParam(); }
    };

    // Fixture for testing with multiple vector inputs
    class Vector3InputTestFixture : public Vector3TestFixture,
        public ::testing::WithParamInterface<std::tuple<float, float, float>> {
    protected:
        Akhanda::Math::Vector3 GetTestVector() const {
            auto [x, y, z] = GetParam();
            return Akhanda::Math::Vector3(x, y, z);
        }
    };

} // namespace Akhanda::Tests::Math