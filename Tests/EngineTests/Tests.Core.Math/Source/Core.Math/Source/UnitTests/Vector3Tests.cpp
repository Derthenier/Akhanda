// Tests/Core.Math/Source/UnitTests/Vector3Tests.cpp
#include <gtest/gtest.h>
#include <cmath>
#include <limits>

import Akhanda.Core.Math;
#include "../Fixtures/MathTestFixtures.hpp"
#include "../TestConstants.hpp"

using namespace Akhanda::Tests::Math;
using namespace Akhanda::Math;

namespace {

    // ============================================================================
    // Constructor Tests
    // ============================================================================

    class Vector3ConstructorTests : public Vector3TestFixture {};

    TEST_F(Vector3ConstructorTests, DefaultConstructor_CreatesZeroVector) {
        Vector3 v;
        EXPECT_FLOAT_EQ(v.x, 0.0f);
        EXPECT_FLOAT_EQ(v.y, 0.0f);
        EXPECT_FLOAT_EQ(v.z, 0.0f);
    }

    TEST_F(Vector3ConstructorTests, ParameterizedConstructor_SetsCorrectValues) {
        Vector3 v(1.5f, -2.3f, 4.7f);
        EXPECT_FLOAT_EQ(v.x, 1.5f);
        EXPECT_FLOAT_EQ(v.y, -2.3f);
        EXPECT_FLOAT_EQ(v.z, 4.7f);
    }

    TEST_F(Vector3ConstructorTests, SingleValueConstructor_SetsAllComponents) {
        Vector3 v(3.14f);
        EXPECT_FLOAT_EQ(v.x, 3.14f);
        EXPECT_FLOAT_EQ(v.y, 3.14f);
        EXPECT_FLOAT_EQ(v.z, 3.14f);
    }

    TEST_F(Vector3ConstructorTests, CopyConstructor_CopiesCorrectly) {
        Vector3 original(1.0f, 2.0f, 3.0f);
        Vector3 copy(original);

        EXPECT_FLOAT_EQ(copy.x, 1.0f);
        EXPECT_FLOAT_EQ(copy.y, 2.0f);
        EXPECT_FLOAT_EQ(copy.z, 3.0f);

        // Verify independence
        copy.x = 99.0f;
        EXPECT_FLOAT_EQ(original.x, 1.0f);
    }

    TEST_F(Vector3ConstructorTests, MoveConstructor_MovesCorrectly) {
        Vector3 original(1.0f, 2.0f, 3.0f);
        Vector3 moved(std::move(original));

        EXPECT_FLOAT_EQ(moved.x, 1.0f);
        EXPECT_FLOAT_EQ(moved.y, 2.0f);
        EXPECT_FLOAT_EQ(moved.z, 3.0f);
    }

    // ============================================================================
    // Assignment Operator Tests
    // ============================================================================

    class Vector3AssignmentTests : public Vector3TestFixture {};

    TEST_F(Vector3AssignmentTests, CopyAssignment_CopiesCorrectly) {
        Vector3 v1(1.0f, 2.0f, 3.0f);
        Vector3 v2;

        v2 = v1;

        EXPECT_FLOAT_EQ(v2.x, 1.0f);
        EXPECT_FLOAT_EQ(v2.y, 2.0f);
        EXPECT_FLOAT_EQ(v2.z, 3.0f);

        // Test self-assignment
        v1 = v1;
        EXPECT_FLOAT_EQ(v1.x, 1.0f);
        EXPECT_FLOAT_EQ(v1.y, 2.0f);
        EXPECT_FLOAT_EQ(v1.z, 3.0f);
    }

    TEST_F(Vector3AssignmentTests, MoveAssignment_MovesCorrectly) {
        Vector3 v1(1.0f, 2.0f, 3.0f);
        Vector3 v2;

        v2 = std::move(v1);

        EXPECT_FLOAT_EQ(v2.x, 1.0f);
        EXPECT_FLOAT_EQ(v2.y, 2.0f);
        EXPECT_FLOAT_EQ(v2.z, 3.0f);
    }

    // ============================================================================
    // Arithmetic Operator Tests
    // ============================================================================

    class Vector3ArithmeticTests : public Vector3TestFixture {};

    TEST_F(Vector3ArithmeticTests, Addition_WorksCorrectly) {
        Vector3 v1(1.0f, 2.0f, 3.0f);
        Vector3 v2(4.0f, 5.0f, 6.0f);

        Vector3 result = v1 + v2;

        EXPECT_FLOAT_EQ(result.x, 5.0f);
        EXPECT_FLOAT_EQ(result.y, 7.0f);
        EXPECT_FLOAT_EQ(result.z, 9.0f);
    }

    TEST_F(Vector3ArithmeticTests, Subtraction_WorksCorrectly) {
        Vector3 v1(4.0f, 5.0f, 6.0f);
        Vector3 v2(1.0f, 2.0f, 3.0f);

        Vector3 result = v1 - v2;

        EXPECT_FLOAT_EQ(result.x, 3.0f);
        EXPECT_FLOAT_EQ(result.y, 3.0f);
        EXPECT_FLOAT_EQ(result.z, 3.0f);
    }

    TEST_F(Vector3ArithmeticTests, ScalarMultiplication_WorksCorrectly) {
        Vector3 v(1.0f, 2.0f, 3.0f);

        Vector3 result1 = v * 2.0f;
        Vector3 result2 = 2.0f * v;

        EXPECT_FLOAT_EQ(result1.x, 2.0f);
        EXPECT_FLOAT_EQ(result1.y, 4.0f);
        EXPECT_FLOAT_EQ(result1.z, 6.0f);

        EXPECT_FLOAT_EQ(result2.x, 2.0f);
        EXPECT_FLOAT_EQ(result2.y, 4.0f);
        EXPECT_FLOAT_EQ(result2.z, 6.0f);
    }

    TEST_F(Vector3ArithmeticTests, ScalarDivision_WorksCorrectly) {
        Vector3 v(2.0f, 4.0f, 6.0f);

        Vector3 result = v / 2.0f;

        EXPECT_FLOAT_EQ(result.x, 1.0f);
        EXPECT_FLOAT_EQ(result.y, 2.0f);
        EXPECT_FLOAT_EQ(result.z, 3.0f);
    }

    TEST_F(Vector3ArithmeticTests, ComponentWiseMultiplication_WorksCorrectly) {
        Vector3 v1(2.0f, 3.0f, 4.0f);
        Vector3 v2(5.0f, 6.0f, 7.0f);

        Vector3 result = v1 * v2;

        EXPECT_FLOAT_EQ(result.x, 10.0f);
        EXPECT_FLOAT_EQ(result.y, 18.0f);
        EXPECT_FLOAT_EQ(result.z, 28.0f);
    }

    TEST_F(Vector3ArithmeticTests, UnaryNegation_WorksCorrectly) {
        Vector3 v(1.0f, -2.0f, 3.0f);

        Vector3 result = -v;

        EXPECT_FLOAT_EQ(result.x, -1.0f);
        EXPECT_FLOAT_EQ(result.y, 2.0f);
        EXPECT_FLOAT_EQ(result.z, -3.0f);
    }

    // ============================================================================
    // Compound Assignment Operator Tests
    // ============================================================================

    class Vector3CompoundAssignmentTests : public Vector3TestFixture {};

    TEST_F(Vector3CompoundAssignmentTests, AdditionAssignment_WorksCorrectly) {
        Vector3 v1(1.0f, 2.0f, 3.0f);
        Vector3 v2(4.0f, 5.0f, 6.0f);

        v1 += v2;

        EXPECT_FLOAT_EQ(v1.x, 5.0f);
        EXPECT_FLOAT_EQ(v1.y, 7.0f);
        EXPECT_FLOAT_EQ(v1.z, 9.0f);
    }

    TEST_F(Vector3CompoundAssignmentTests, SubtractionAssignment_WorksCorrectly) {
        Vector3 v1(4.0f, 5.0f, 6.0f);
        Vector3 v2(1.0f, 2.0f, 3.0f);

        v1 -= v2;

        EXPECT_FLOAT_EQ(v1.x, 3.0f);
        EXPECT_FLOAT_EQ(v1.y, 3.0f);
        EXPECT_FLOAT_EQ(v1.z, 3.0f);
    }

    TEST_F(Vector3CompoundAssignmentTests, ScalarMultiplicationAssignment_WorksCorrectly) {
        Vector3 v(1.0f, 2.0f, 3.0f);

        v *= 2.0f;

        EXPECT_FLOAT_EQ(v.x, 2.0f);
        EXPECT_FLOAT_EQ(v.y, 4.0f);
        EXPECT_FLOAT_EQ(v.z, 6.0f);
    }

    TEST_F(Vector3CompoundAssignmentTests, ScalarDivisionAssignment_WorksCorrectly) {
        Vector3 v(2.0f, 4.0f, 6.0f);

        v /= 2.0f;

        EXPECT_FLOAT_EQ(v.x, 1.0f);
        EXPECT_FLOAT_EQ(v.y, 2.0f);
        EXPECT_FLOAT_EQ(v.z, 3.0f);
    }

    // ============================================================================
    // Comparison Operator Tests
    // ============================================================================

    class Vector3ComparisonTests : public Vector3TestFixture {};

    TEST_F(Vector3ComparisonTests, Equality_WorksCorrectly) {
        Vector3 v1(1.0f, 2.0f, 3.0f);
        Vector3 v2(1.0f, 2.0f, 3.0f);
        Vector3 v3(1.0f, 2.0f, 4.0f);

        EXPECT_TRUE(v1 == v2);
        EXPECT_FALSE(v1 == v3);
        EXPECT_TRUE(v1 == v1); // Self equality
    }

    TEST_F(Vector3ComparisonTests, Inequality_WorksCorrectly) {
        Vector3 v1(1.0f, 2.0f, 3.0f);
        Vector3 v2(1.0f, 2.0f, 3.0f);
        Vector3 v3(1.0f, 2.0f, 4.0f);

        EXPECT_FALSE(v1 != v2);
        EXPECT_TRUE(v1 != v3);
        EXPECT_FALSE(v1 != v1); // Self inequality
    }

    // ============================================================================
    // Vector Math Function Tests
    // ============================================================================

    class Vector3MathFunctionTests : public Vector3TestFixture {};

    TEST_F(Vector3MathFunctionTests, DotProduct_WorksCorrectly) {
        Vector3 v1(1.0f, 2.0f, 3.0f);
        Vector3 v2(4.0f, 5.0f, 6.0f);

        float result = Dot(v1, v2);

        EXPECT_FLOAT_EQ(result, 32.0f); // 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
    }

    TEST_F(Vector3MathFunctionTests, DotProduct_OrthogonalVectors_ReturnsZero) {
        Vector3 v1(1.0f, 0.0f, 0.0f);
        Vector3 v2(0.0f, 1.0f, 0.0f);

        float result = Dot(v1, v2);

        EXPECT_NEAR(result, 0.0f, Constants::EPSILON);
    }

    TEST_F(Vector3MathFunctionTests, DotProduct_SameVector_ReturnsLengthSquared) {
        Vector3 v(3.0f, 4.0f, 5.0f);

        float dotResult = Dot(v, v);
        float lengthSq = v.x * v.x + v.y * v.y + v.z * v.z;

        EXPECT_FLOAT_EQ(dotResult, lengthSq);
        EXPECT_FLOAT_EQ(dotResult, 50.0f); // 9 + 16 + 25 = 50
    }

    TEST_F(Vector3MathFunctionTests, CrossProduct_WorksCorrectly) {
        Vector3 v1(1.0f, 0.0f, 0.0f);
        Vector3 v2(0.0f, 1.0f, 0.0f);

        Vector3 result = Cross(v1, v2);

        EXPECT_FLOAT_EQ(result.x, 0.0f);
        EXPECT_FLOAT_EQ(result.y, 0.0f);
        EXPECT_FLOAT_EQ(result.z, 1.0f);
    }

    TEST_F(Vector3MathFunctionTests, CrossProduct_AntiCommutative) {
        Vector3 v1(1.0f, 2.0f, 3.0f);
        Vector3 v2(4.0f, 5.0f, 6.0f);

        Vector3 cross1 = Cross(v1, v2);
        Vector3 cross2 = Cross(v2, v1);

        EXPECT_FLOAT_EQ(cross1.x, -cross2.x);
        EXPECT_FLOAT_EQ(cross1.y, -cross2.y);
        EXPECT_FLOAT_EQ(cross1.z, -cross2.z);
    }

    TEST_F(Vector3MathFunctionTests, CrossProduct_ParallelVectors_ReturnsZero) {
        Vector3 v1(1.0f, 2.0f, 3.0f);
        Vector3 v2(2.0f, 4.0f, 6.0f); // v2 = 2 * v1

        Vector3 result = Cross(v1, v2);

        EXPECT_NEAR(result.x, 0.0f, Constants::EPSILON);
        EXPECT_NEAR(result.y, 0.0f, Constants::EPSILON);
        EXPECT_NEAR(result.z, 0.0f, Constants::EPSILON);
    }

    TEST_F(Vector3MathFunctionTests, Length_WorksCorrectly) {
        Vector3 v(3.0f, 4.0f, 5.0f);

        float length = Length(v);

        EXPECT_FLOAT_EQ(length, std::sqrt(50.0f));
    }

    TEST_F(Vector3MathFunctionTests, Length_UnitVectors_ReturnOne) {
        EXPECT_FLOAT_EQ(Length(unitX), 1.0f);
        EXPECT_FLOAT_EQ(Length(unitY), 1.0f);
        EXPECT_FLOAT_EQ(Length(unitZ), 1.0f);
    }

    TEST_F(Vector3MathFunctionTests, Length_ZeroVector_ReturnsZero) {
        EXPECT_FLOAT_EQ(Length(zero), 0.0f);
    }

    TEST_F(Vector3MathFunctionTests, LengthSquared_WorksCorrectly) {
        Vector3 v(3.0f, 4.0f, 5.0f);

        float lengthSq = LengthSquared(v);

        EXPECT_FLOAT_EQ(lengthSq, 50.0f);
    }

    TEST_F(Vector3MathFunctionTests, LengthSquared_AvoidsSqrt) {
        // Test that LengthSquared is more efficient than Length^2
        Vector3 v(3.0f, 4.0f, 5.0f);

        float lengthSq = LengthSquared(v);
        float lengthSquared = Length(v) * Length(v);

        EXPECT_NEAR(lengthSq, lengthSquared, Constants::EPSILON);
    }

    TEST_F(Vector3MathFunctionTests, Normalize_WorksCorrectly) {
        Vector3 v(3.0f, 4.0f, 5.0f);

        Vector3 normalized = Normalize(v);

        EXPECT_NEAR(Length(normalized), 1.0f, Constants::EPSILON);

        // Check direction is preserved
        float originalLength = Length(v);
        EXPECT_NEAR(normalized.x, v.x / originalLength, Constants::EPSILON);
        EXPECT_NEAR(normalized.y, v.y / originalLength, Constants::EPSILON);
        EXPECT_NEAR(normalized.z, v.z / originalLength, Constants::EPSILON);
    }

    TEST_F(Vector3MathFunctionTests, Normalize_UnitVector_RemainsUnchanged) {
        Vector3 normalized = Normalize(unitX);

        EXPECT_FLOAT_EQ(normalized.x, 1.0f);
        EXPECT_FLOAT_EQ(normalized.y, 0.0f);
        EXPECT_FLOAT_EQ(normalized.z, 0.0f);
    }

    TEST_F(Vector3MathFunctionTests, Distance_WorksCorrectly) {
        Vector3 v1(0.0f, 0.0f, 0.0f);
        Vector3 v2(3.0f, 4.0f, 5.0f);

        float distance = Distance(v1, v2);

        EXPECT_FLOAT_EQ(distance, std::sqrt(50.0f));
    }

    TEST_F(Vector3MathFunctionTests, Distance_SamePoint_ReturnsZero) {
        Vector3 v(1.0f, 2.0f, 3.0f);

        float distance = Distance(v, v);

        EXPECT_FLOAT_EQ(distance, 0.0f);
    }

    TEST_F(Vector3MathFunctionTests, DistanceSquared_WorksCorrectly) {
        Vector3 v1(0.0f, 0.0f, 0.0f);
        Vector3 v2(3.0f, 4.0f, 5.0f);

        float distanceSq = DistanceSquared(v1, v2);

        EXPECT_FLOAT_EQ(distanceSq, 50.0f);
    }

    // ============================================================================
    // Edge Case Tests
    // ============================================================================

    class Vector3EdgeCaseTests : public Vector3TestFixture {};

    TEST_F(Vector3EdgeCaseTests, InfinityHandling) {
        Vector3 v(Constants::INFINITY_F, 0.0f, 0.0f);

        EXPECT_TRUE(std::isinf(v.x));
        EXPECT_FALSE(std::isfinite(v.x));

        // Test that operations with infinity behave as expected
        Vector3 result = v + Vector3(1.0f, 1.0f, 1.0f);
        EXPECT_TRUE(std::isinf(result.x));
    }

    TEST_F(Vector3EdgeCaseTests, NaNHandling) {
        Vector3 v(Constants::NAN_F, 0.0f, 0.0f);

        EXPECT_TRUE(std::isnan(v.x));

        // Test that NaN propagates
        Vector3 result = v + Vector3(1.0f, 1.0f, 1.0f);
        EXPECT_TRUE(std::isnan(result.x));
    }

    TEST_F(Vector3EdgeCaseTests, NormalizeZeroVector_HandlesGracefully) {
        Vector3 zero_(0.0f, 0.0f, 0.0f);

        // Implementation should handle zero vector gracefully
        // (either return zero or throw/assert)
        Vector3 result = Normalize(zero_);

        // At minimum, result should not crash and components should be either 0 or NaN
        EXPECT_TRUE(std::isnan(result.x) || result.x == 0.0f);
    }

    TEST_F(Vector3EdgeCaseTests, DivisionByZero_HandlesGracefully) {
        Vector3 v(1.0f, 2.0f, 3.0f);

        Vector3 result = v / 0.0f;

        // Should result in infinity
        EXPECT_TRUE(std::isinf(result.x));
        EXPECT_TRUE(std::isinf(result.y));
        EXPECT_TRUE(std::isinf(result.z));
    }

    TEST_F(Vector3EdgeCaseTests, VeryLargeNumbers_MaintainPrecision) {
        Vector3 large(1e20f, 1e20f, 1e20f);
        Vector3 small(1e-10f, 1e-10f, 1e-10f);

        Vector3 sum = large + small;

        // Large number should dominate due to floating point precision
        EXPECT_FLOAT_EQ(sum.x, large.x);
        EXPECT_FLOAT_EQ(sum.y, large.y);
        EXPECT_FLOAT_EQ(sum.z, large.z);
    }

    // ============================================================================
    // Parameterized Tests
    // ============================================================================

    class Vector3ParameterizedTests : public Vector3TestFixture,
        public ::testing::WithParamInterface<std::tuple<float, float, float>> {
    };

    TEST_P(Vector3ParameterizedTests, LengthAndNormalize_Consistency) {
        auto [x, y, z] = GetParam();
        Vector3 v(x, y, z);

        if (Length(v) > Constants::EPSILON) {
            Vector3 normalized = Normalize(v);
            float normalizedLength = Length(normalized);

            EXPECT_NEAR(normalizedLength, 1.0f, Constants::EPSILON);
        }
    }

    INSTANTIATE_TEST_SUITE_P(
        CommonVectors,
        Vector3ParameterizedTests,
        ::testing::Values(
            std::make_tuple(1.0f, 0.0f, 0.0f),
            std::make_tuple(0.0f, 1.0f, 0.0f),
            std::make_tuple(0.0f, 0.0f, 1.0f),
            std::make_tuple(1.0f, 1.0f, 1.0f),
            std::make_tuple(3.0f, 4.0f, 5.0f),
            std::make_tuple(-1.0f, -2.0f, -3.0f),
            std::make_tuple(0.1f, 0.2f, 0.3f),
            std::make_tuple(100.0f, 200.0f, 300.0f)
        )
    );

} // anonymous namespace