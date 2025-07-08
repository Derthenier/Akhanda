// Tests/Core.Math/Source/PerformanceTests/SIMDPerformanceTests.cpp
#include <gtest/gtest.h>
#include <chrono>
#include <vector>
#include <random>
#include <numeric>
#include <algorithm>
#include <immintrin.h>  // For SIMD intrinsics

import Akhanda.Core.Math;
#include "../Fixtures/MathTestFixtures.hpp"
#include "../TestConstants.hpp"

using namespace Akhanda::Tests::Math;
using namespace Akhanda::Math;

namespace {

    // ============================================================================
    // SIMD Performance Test Fixture
    // ============================================================================

    class SIMDPerformanceTests : public PerformanceTestFixture {
    protected:
        void SetUp() override {
            PerformanceTestFixture::SetUp();

            // Generate aligned test data for SIMD operations
            GenerateAlignedTestData();
        }

        void GenerateAlignedTestData() {
            constexpr size_t SIMD_VECTOR_COUNT = 10000;
            //constexpr size_t ALIGNMENT = 32; // AVX alignment

            // Allocate aligned memory for SIMD operations
            simdVectors1.resize(SIMD_VECTOR_COUNT);
            simdVectors2.resize(SIMD_VECTOR_COUNT);
            simdResults.resize(SIMD_VECTOR_COUNT);

            // Fill with random data
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<float> dist(-100.0f, 100.0f);

            for (size_t i = 0; i < SIMD_VECTOR_COUNT; ++i) {
                simdVectors1[i] = Vector3(dist(gen), dist(gen), dist(gen));
                simdVectors2[i] = Vector3(dist(gen), dist(gen), dist(gen));
            }

            // Generate scalar test data for comparison
            scalarData1.resize(SIMD_VECTOR_COUNT * 3);
            scalarData2.resize(SIMD_VECTOR_COUNT * 3);
            scalarResults.resize(SIMD_VECTOR_COUNT * 3);

            for (size_t i = 0; i < SIMD_VECTOR_COUNT; ++i) {
                scalarData1[i * 3 + 0] = simdVectors1[i].x;
                scalarData1[i * 3 + 1] = simdVectors1[i].y;
                scalarData1[i * 3 + 2] = simdVectors1[i].z;

                scalarData2[i * 3 + 0] = simdVectors2[i].x;
                scalarData2[i * 3 + 1] = simdVectors2[i].y;
                scalarData2[i * 3 + 2] = simdVectors2[i].z;
            }
        }

    protected:
        std::vector<Vector3> simdVectors1, simdVectors2, simdResults;
        std::vector<float> scalarData1, scalarData2, scalarResults;
    };

    // ============================================================================
    // Vector Addition Performance Tests
    // ============================================================================

    TEST_F(SIMDPerformanceTests, VectorAddition_SIMD_vs_Scalar) {
        const size_t iterations = 1000;
        const size_t vectorCount = simdVectors1.size();

        // Test SIMD vector addition
        auto simdTime = BenchmarkFunction([&]() {
            for (size_t i = 0; i < vectorCount; ++i) {
                simdResults[i] = simdVectors1[i] + simdVectors2[i];
            }
            }, iterations);

        // Test scalar addition (component-wise)
        auto scalarTime = BenchmarkFunction([&]() {
            for (size_t i = 0; i < vectorCount * 3; i += 3) {
                scalarResults[i + 0] = scalarData1[i + 0] + scalarData2[i + 0];
                scalarResults[i + 1] = scalarData1[i + 1] + scalarData2[i + 1];
                scalarResults[i + 2] = scalarData1[i + 2] + scalarData2[i + 2];
            }
            }, iterations);

        std::cout << "[PERF] Vector Addition (SIMD): " << simdTime << " ns/op" << std::endl;
        std::cout << "[PERF] Vector Addition (Scalar): " << scalarTime << " ns/op" << std::endl;

        double speedup = scalarTime / simdTime;
        std::cout << "[PERF] SIMD Speedup: " << speedup << "x" << std::endl;

        // SIMD should be at least as fast as scalar (may not always be faster due to overhead)
        EXPECT_GT(speedup, 0.8) << "SIMD implementation significantly slower than scalar";

        // Verify correctness by comparing a few results
        for (size_t i = 0; i < std::min(vectorCount, size_t(10)); ++i) {
            Vector3 expected(
                scalarResults[i * 3 + 0],
                scalarResults[i * 3 + 1],
                scalarResults[i * 3 + 2]
            );

            EXPECT_NEAR(simdResults[i].x, expected.x, Constants::EPSILON);
            EXPECT_NEAR(simdResults[i].y, expected.y, Constants::EPSILON);
            EXPECT_NEAR(simdResults[i].z, expected.z, Constants::EPSILON);
        }
    }

    TEST_F(SIMDPerformanceTests, VectorMultiplication_SIMD_vs_Scalar) {
        const size_t iterations = 1000;
        const size_t vectorCount = simdVectors1.size();
        const float scalar = 2.5f;

        // Test SIMD scalar multiplication
        auto simdTime = BenchmarkFunction([&]() {
            for (size_t i = 0; i < vectorCount; ++i) {
                simdResults[i] = simdVectors1[i] * scalar;
            }
            }, iterations);

        // Test scalar multiplication (component-wise)
        auto scalarTime = BenchmarkFunction([&]() {
            for (size_t i = 0; i < vectorCount * 3; i += 3) {
                scalarResults[i + 0] = scalarData1[i + 0] * scalar;
                scalarResults[i + 1] = scalarData1[i + 1] * scalar;
                scalarResults[i + 2] = scalarData1[i + 2] * scalar;
            }
            }, iterations);

        std::cout << "[PERF] Scalar Multiplication (SIMD): " << simdTime << " ns/op" << std::endl;
        std::cout << "[PERF] Scalar Multiplication (Scalar): " << scalarTime << " ns/op" << std::endl;

        double speedup = scalarTime / simdTime;
        std::cout << "[PERF] SIMD Speedup: " << speedup << "x" << std::endl;

        EXPECT_GT(speedup, 0.8);
    }

    // ============================================================================
    // Dot Product Performance Tests
    // ============================================================================

    TEST_F(SIMDPerformanceTests, DotProduct_SIMD_vs_Scalar) {
        const size_t iterations = 1000;
        const size_t vectorCount = simdVectors1.size();
        std::vector<float> dotResults(vectorCount);

        // Test SIMD dot product
        auto simdTime = BenchmarkFunction([&]() {
            for (size_t i = 0; i < vectorCount; ++i) {
                dotResults[i] = Dot(simdVectors1[i], simdVectors2[i]);
            }
            }, iterations);

        // Test scalar dot product
        std::vector<float> scalarDotResults(vectorCount);
        auto scalarTime = BenchmarkFunction([&]() {
            for (size_t i = 0; i < vectorCount; ++i) {
                size_t idx = i * 3;
                scalarDotResults[i] = scalarData1[idx + 0] * scalarData2[idx + 0] +
                    scalarData1[idx + 1] * scalarData2[idx + 1] +
                    scalarData1[idx + 2] * scalarData2[idx + 2];
            }
            }, iterations);

        std::cout << "[PERF] Dot Product (SIMD): " << simdTime << " ns/op" << std::endl;
        std::cout << "[PERF] Dot Product (Scalar): " << scalarTime << " ns/op" << std::endl;

        double speedup = scalarTime / simdTime;
        std::cout << "[PERF] SIMD Speedup: " << speedup << "x" << std::endl;

        EXPECT_GT(speedup, 0.8);

        // Verify correctness
        for (size_t i = 0; i < std::min(vectorCount, size_t(10)); ++i) {
            EXPECT_NEAR(dotResults[i], scalarDotResults[i], Constants::EPSILON);
        }
    }

    // ============================================================================
    // Cross Product Performance Tests
    // ============================================================================

    TEST_F(SIMDPerformanceTests, CrossProduct_SIMD_vs_Scalar) {
        const size_t iterations = 1000;
        const size_t vectorCount = simdVectors1.size();

        // Test SIMD cross product
        auto simdTime = BenchmarkFunction([&]() {
            for (size_t i = 0; i < vectorCount; ++i) {
                simdResults[i] = Cross(simdVectors1[i], simdVectors2[i]);
            }
            }, iterations);

        // Test scalar cross product
        auto scalarTime = BenchmarkFunction([&]() {
            for (size_t i = 0; i < vectorCount; ++i) {
                size_t idx = i * 3;
                float a1 = scalarData1[idx + 0], a2 = scalarData1[idx + 1], a3 = scalarData1[idx + 2];
                float b1 = scalarData2[idx + 0], b2 = scalarData2[idx + 1], b3 = scalarData2[idx + 2];

                scalarResults[idx + 0] = a2 * b3 - a3 * b2;
                scalarResults[idx + 1] = a3 * b1 - a1 * b3;
                scalarResults[idx + 2] = a1 * b2 - a2 * b1;
            }
            }, iterations);

        std::cout << "[PERF] Cross Product (SIMD): " << simdTime << " ns/op" << std::endl;
        std::cout << "[PERF] Cross Product (Scalar): " << scalarTime << " ns/op" << std::endl;

        double speedup = scalarTime / simdTime;
        std::cout << "[PERF] SIMD Speedup: " << speedup << "x" << std::endl;

        EXPECT_GT(speedup, 0.8);
    }

    // ============================================================================
    // Vector Length Performance Tests
    // ============================================================================

    TEST_F(SIMDPerformanceTests, VectorLength_SIMD_vs_Scalar) {
        const size_t iterations = 1000;
        const size_t vectorCount = simdVectors1.size();
        std::vector<float> lengthResults(vectorCount);

        // Test SIMD length calculation
        auto simdTime = BenchmarkFunction([&]() {
            for (size_t i = 0; i < vectorCount; ++i) {
                lengthResults[i] = Length(simdVectors1[i]);
            }
            }, iterations);

        // Test scalar length calculation
        std::vector<float> scalarLengthResults(vectorCount);
        auto scalarTime = BenchmarkFunction([&]() {
            for (size_t i = 0; i < vectorCount; ++i) {
                size_t idx = i * 3;
                float x = scalarData1[idx + 0];
                float y = scalarData1[idx + 1];
                float z = scalarData1[idx + 2];
                scalarLengthResults[i] = std::sqrt(x * x + y * y + z * z);
            }
            }, iterations);

        std::cout << "[PERF] Vector Length (SIMD): " << simdTime << " ns/op" << std::endl;
        std::cout << "[PERF] Vector Length (Scalar): " << scalarTime << " ns/op" << std::endl;

        double speedup = scalarTime / simdTime;
        std::cout << "[PERF] SIMD Speedup: " << speedup << "x" << std::endl;

        EXPECT_GT(speedup, 0.8);

        // Verify correctness
        for (size_t i = 0; i < std::min(vectorCount, size_t(10)); ++i) {
            EXPECT_NEAR(lengthResults[i], scalarLengthResults[i], Constants::EPSILON);
        }
    }

    // ============================================================================
    // Vector Normalization Performance Tests
    // ============================================================================

    TEST_F(SIMDPerformanceTests, VectorNormalize_SIMD_vs_Scalar) {
        const size_t iterations = 1000;
        const size_t vectorCount = simdVectors1.size();

        // Test SIMD normalization
        auto simdTime = BenchmarkFunction([&]() {
            for (size_t i = 0; i < vectorCount; ++i) {
                simdResults[i] = Normalize(simdVectors1[i]);
            }
            }, iterations);

        // Test scalar normalization
        auto scalarTime = BenchmarkFunction([&]() {
            for (size_t i = 0; i < vectorCount; ++i) {
                size_t idx = i * 3;
                float x = scalarData1[idx + 0];
                float y = scalarData1[idx + 1];
                float z = scalarData1[idx + 2];
                float length = std::sqrt(x * x + y * y + z * z);

                if (length > Constants::EPSILON) {
                    float invLength = 1.0f / length;
                    scalarResults[idx + 0] = x * invLength;
                    scalarResults[idx + 1] = y * invLength;
                    scalarResults[idx + 2] = z * invLength;
                }
                else {
                    scalarResults[idx + 0] = 0.0f;
                    scalarResults[idx + 1] = 0.0f;
                    scalarResults[idx + 2] = 0.0f;
                }
            }
            }, iterations);

        std::cout << "[PERF] Vector Normalize (SIMD): " << simdTime << " ns/op" << std::endl;
        std::cout << "[PERF] Vector Normalize (Scalar): " << scalarTime << " ns/op" << std::endl;

        double speedup = scalarTime / simdTime;
        std::cout << "[PERF] SIMD Speedup: " << speedup << "x" << std::endl;

        EXPECT_GT(speedup, 0.8);
    }

    // ============================================================================
    // Bulk Operations Performance Tests
    // ============================================================================

    TEST_F(SIMDPerformanceTests, BulkVectorOperations_MemoryBandwidth) {
        const size_t iterations = 100;
        const size_t vectorCount = simdVectors1.size();

        // Test memory-bound operations that benefit most from SIMD
        auto bulkOperationTime = BenchmarkFunction([&]() {
            // Perform multiple operations in sequence to test memory bandwidth
            for (size_t i = 0; i < vectorCount; ++i) {
                Vector3 temp1 = simdVectors1[i] + simdVectors2[i];
                Vector3 temp2 = temp1 * 2.0f;
                Vector3 temp3 = temp2 - simdVectors1[i];
                simdResults[i] = Normalize(temp3);
            }
            }, iterations);

        std::cout << "[PERF] Bulk Vector Operations: " << bulkOperationTime << " ns/op" << std::endl;

        // Calculate estimated throughput
        size_t totalOperations = vectorCount * 4; // 4 operations per vector
        double operationsPerSecond = totalOperations / (bulkOperationTime * 1e-9);
        double megaOperationsPerSecond = operationsPerSecond / 1e6;

        std::cout << "[PERF] Throughput: " << megaOperationsPerSecond << " MOps/sec" << std::endl;

        // Verify that SIMD implementation maintains reasonable performance
        // This is a sanity check rather than a comparison
        EXPECT_LT(bulkOperationTime, 1000.0) << "Bulk operations taking too long";
    }

    // ============================================================================
    // SIMD Alignment Tests
    // ============================================================================

    TEST_F(SIMDPerformanceTests, MemoryAlignment_Performance_Impact) {
        const size_t iterations = 1000;
        const size_t vectorCount = 1000;

        // Create aligned vectors
        std::vector<Vector3> alignedVectors(vectorCount);

        // Create deliberately misaligned vectors (if possible)
        std::vector<Vector3> misalignedVectors(vectorCount + 1);
        Vector3* misalignedPtr = &misalignedVectors[1]; // Start from offset 1

        // Fill with same data
        for (size_t i = 0; i < vectorCount; ++i) {
            alignedVectors[i] = simdVectors1[i];
            misalignedPtr[i] = simdVectors1[i];
        }

        // Test aligned access
        std::vector<Vector3> alignedResults(vectorCount);
        auto alignedTime = BenchmarkFunction([&]() {
            for (size_t i = 0; i < vectorCount; ++i) {
                alignedResults[i] = alignedVectors[i] + simdVectors2[i];
            }
            }, iterations);

        // Test potentially misaligned access
        std::vector<Vector3> misalignedResults(vectorCount);
        auto misalignedTime = BenchmarkFunction([&]() {
            for (size_t i = 0; i < vectorCount; ++i) {
                misalignedResults[i] = misalignedPtr[i] + simdVectors2[i];
            }
            }, iterations);

        std::cout << "[PERF] Aligned Access: " << alignedTime << " ns/op" << std::endl;
        std::cout << "[PERF] Misaligned Access: " << misalignedTime << " ns/op" << std::endl;

        double alignmentPenalty = misalignedTime / alignedTime;
        std::cout << "[PERF] Alignment Penalty: " << alignmentPenalty << "x" << std::endl;

        // Misaligned access should not be more than 2x slower
        EXPECT_LT(alignmentPenalty, 2.0) << "Excessive alignment penalty";
    }

    // ============================================================================
    // Cache Performance Tests
    // ============================================================================

    TEST_F(SIMDPerformanceTests, CachePerformance_SequentialVsRandom) {
        const size_t iterations = 100;
        const size_t vectorCount = simdVectors1.size();

        // Create random access pattern
        std::vector<size_t> randomIndices(vectorCount);
        std::iota(randomIndices.begin(), randomIndices.end(), 0);
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(randomIndices.begin(), randomIndices.end(), g);

        // Test sequential access (cache-friendly)
        auto sequentialTime = BenchmarkFunction([&]() {
            for (size_t i = 0; i < vectorCount; ++i) {
                simdResults[i] = simdVectors1[i] + simdVectors2[i];
            }
            }, iterations);

        // Test random access (cache-unfriendly)
        auto randomTime = BenchmarkFunction([&]() {
            for (size_t i = 0; i < vectorCount; ++i) {
                size_t idx = randomIndices[i];
                simdResults[i] = simdVectors1[idx] + simdVectors2[idx];
            }
            }, iterations);

        std::cout << "[PERF] Sequential Access: " << sequentialTime << " ns/op" << std::endl;
        std::cout << "[PERF] Random Access: " << randomTime << " ns/op" << std::endl;

        double cachePenalty = randomTime / sequentialTime;
        std::cout << "[PERF] Cache Miss Penalty: " << cachePenalty << "x" << std::endl;

        // Random access penalty should be reasonable (modern CPUs have good caches)
        EXPECT_LT(cachePenalty, 5.0) << "Excessive cache miss penalty";
    }

} // anonymous namespace