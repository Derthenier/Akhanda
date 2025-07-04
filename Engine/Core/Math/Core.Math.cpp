// Core.Math.cpp - Main Implementation
module Core.Math;

import <cmath>;
import <algorithm>;
import <array>;
import <vector>;
import <random>;
import <limits>;
import <immintrin.h>; // SIMD intrinsics

using namespace Math;


// =============================================================================
// Static Constants
// =============================================================================
#pragma region Constants

// Vector2 constants
const Vector2 Vector2::ZERO(0.0f, 0.0f);
const Vector2 Vector2::ONE(1.0f, 1.0f);
const Vector2 Vector2::UNIT_X(1.0f, 0.0f);
const Vector2 Vector2::UNIT_Y(0.0f, 1.0f);

// Vector3 constants
const Vector3 Vector3::ZERO(0.0f, 0.0f, 0.0f);
const Vector3 Vector3::ONE(1.0f, 1.0f, 1.0f);
const Vector3 Vector3::UNIT_X(1.0f, 0.0f, 0.0f);
const Vector3 Vector3::UNIT_Y(0.0f, 1.0f, 0.0f);
const Vector3 Vector3::UNIT_Z(0.0f, 0.0f, 1.0f);
const Vector3 Vector3::UP(0.0f, 1.0f, 0.0f);
const Vector3 Vector3::DOWN(0.0f, -1.0f, 0.0f);
const Vector3 Vector3::FORWARD(0.0f, 0.0f, 1.0f);
const Vector3 Vector3::BACK(0.0f, 0.0f, -1.0f);
const Vector3 Vector3::LEFT(-1.0f, 0.0f, 0.0f);
const Vector3 Vector3::RIGHT(1.0f, 0.0f, 0.0f);

// Vector4 constants
const Vector4 Vector4::ZERO(0.0f, 0.0f, 0.0f, 0.0f);
const Vector4 Vector4::ONE(1.0f, 1.0f, 1.0f, 1.0f);
const Vector4 Vector4::UNIT_X(1.0f, 0.0f, 0.0f, 0.0f);
const Vector4 Vector4::UNIT_Y(0.0f, 1.0f, 0.0f, 0.0f);
const Vector4 Vector4::UNIT_Z(0.0f, 0.0f, 1.0f, 0.0f);
const Vector4 Vector4::UNIT_W(0.0f, 0.0f, 0.0f, 1.0f);

// Matrix constants
const Matrix3 Matrix3::ZERO{};
const Matrix3 Matrix3::IDENTITY(1.0f);
const Matrix4 Matrix4::ZERO{};
const Matrix4 Matrix4::IDENTITY(1.0f);

// Quaternion constants
const Quaternion Quaternion::IDENTITY(0.0f, 0.0f, 0.0f, 1.0f);

// Transform constants
const Transform Transform::IDENTITY{};
const Transform2D Transform2D::IDENTITY{};

// Color constants
const Color3 Color3::BLACK(0.0f, 0.0f, 0.0f);
const Color3 Color3::WHITE(1.0f, 1.0f, 1.0f);
const Color3 Color3::RED(1.0f, 0.0f, 0.0f);
const Color3 Color3::GREEN(0.0f, 1.0f, 0.0f);
const Color3 Color3::BLUE(0.0f, 0.0f, 1.0f);
const Color3 Color3::YELLOW(1.0f, 1.0f, 0.0f);
const Color3 Color3::MAGENTA(1.0f, 0.0f, 1.0f);
const Color3 Color3::CYAN(0.0f, 1.0f, 1.0f);

const Color4 Color4::BLACK(0.0f, 0.0f, 0.0f, 1.0f);
const Color4 Color4::WHITE(1.0f, 1.0f, 1.0f, 1.0f);
const Color4 Color4::RED(1.0f, 0.0f, 0.0f, 1.0f);
const Color4 Color4::GREEN(0.0f, 1.0f, 0.0f, 1.0f);
const Color4 Color4::BLUE(0.0f, 0.0f, 1.0f, 1.0f);
const Color4 Color4::YELLOW(1.0f, 1.0f, 0.0f, 1.0f);
const Color4 Color4::MAGENTA(1.0f, 0.0f, 1.0f, 1.0f);
const Color4 Color4::CYAN(0.0f, 1.0f, 1.0f, 1.0f);
const Color4 Color4::TRANSPARENT(0.0f, 0.0f, 0.0f, 0.0f);

#pragma endregion



// =============================================================================
// Vector2 Implementation
// =============================================================================
#pragma region Vector2
constexpr float& Vector2::operator[](size_t index) noexcept {
    return (&x)[index];
}

constexpr const float& Vector2::operator[](size_t index) const noexcept {
    return (&x)[index];
}

constexpr Vector2 Vector2::operator+(const Vector2& other) const noexcept {
    return Vector2(x + other.x, y + other.y);
}

constexpr Vector2 Vector2::operator-(const Vector2& other) const noexcept {
    return Vector2(x - other.x, y - other.y);
}

constexpr Vector2 Vector2::operator*(const Vector2& other) const noexcept {
    return Vector2(x * other.x, y * other.y);
}

constexpr Vector2 Vector2::operator/(const Vector2& other) const noexcept {
    return Vector2(x / other.x, y / other.y);
}

constexpr Vector2 Vector2::operator*(float scalar) const noexcept {
    return Vector2(x * scalar, y * scalar);
}

constexpr Vector2 Vector2::operator/(float scalar) const noexcept {
    const float invScalar = 1.0f / scalar;
    return Vector2(x * invScalar, y * invScalar);
}

constexpr Vector2& Vector2::operator+=(const Vector2& other) noexcept {
    x += other.x;
    y += other.y;
    return *this;
}

constexpr Vector2& Vector2::operator-=(const Vector2& other) noexcept {
    x -= other.x;
    y -= other.y;
    return *this;
}

constexpr Vector2& Vector2::operator*=(const Vector2& other) noexcept {
    x *= other.x;
    y *= other.y;
    return *this;
}

constexpr Vector2& Vector2::operator/=(const Vector2& other) noexcept {
    x /= other.x;
    y /= other.y;
    return *this;
}

constexpr Vector2& Vector2::operator*=(float scalar) noexcept {
    x *= scalar;
    y *= scalar;
    return *this;
}

constexpr Vector2& Vector2::operator/=(float scalar) noexcept {
    const float invScalar = 1.0f / scalar;
    x *= invScalar;
    y *= invScalar;
    return *this;
}

constexpr Vector2 Vector2::operator-() const noexcept {
    return Vector2(-x, -y);
}

constexpr Vector2 Vector2::operator+() const noexcept {
    return *this;
}

constexpr bool Vector2::operator==(const Vector2& other) const noexcept {
    return IsNearlyEqual(x, other.x) && IsNearlyEqual(y, other.y);
}

constexpr bool Vector2::operator!=(const Vector2& other) const noexcept {
    return !(*this == other);
}

#pragma endregion

// =============================================================================
// Vector3 Implementation
// =============================================================================
#pragma region Vector3

constexpr float& Vector3::operator[](size_t index) noexcept {
    return (&x)[index];
}

constexpr const float& Vector3::operator[](size_t index) const noexcept {
    return (&x)[index];
}

constexpr Vector3 Vector3::operator+(const Vector3& other) const noexcept {
    return Vector3(x + other.x, y + other.y, z + other.z);
}

constexpr Vector3 Vector3::operator-(const Vector3& other) const noexcept {
    return Vector3(x - other.x, y - other.y, z - other.z);
}

constexpr Vector3 Vector3::operator*(const Vector3& other) const noexcept {
    return Vector3(x * other.x, y * other.y, z * other.z);
}

constexpr Vector3 Vector3::operator/(const Vector3& other) const noexcept {
    return Vector3(x / other.x, y / other.y, z / other.z);
}

constexpr Vector3 Vector3::operator*(float scalar) const noexcept {
    return Vector3(x * scalar, y * scalar, z * scalar);
}

constexpr Vector3 Vector3::operator/(float scalar) const noexcept {
    const float invScalar = 1.0f / scalar;
    return Vector3(x * invScalar, y * invScalar, z * invScalar);
}

constexpr Vector3& Vector3::operator+=(const Vector3& other) noexcept {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}

constexpr Vector3& Vector3::operator-=(const Vector3& other) noexcept {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
}

constexpr Vector3& Vector3::operator*=(const Vector3& other) noexcept {
    x *= other.x;
    y *= other.y;
    z *= other.z;
    return *this;
}

constexpr Vector3& Vector3::operator/=(const Vector3& other) noexcept {
    x /= other.x;
    y /= other.y;
    z /= other.z;
    return *this;
}

constexpr Vector3& Vector3::operator*=(float scalar) noexcept {
    x *= scalar;
    y *= scalar;
    z *= scalar;
    return *this;
}

constexpr Vector3& Vector3::operator/=(float scalar) noexcept {
    const float invScalar = 1.0f / scalar;
    x *= invScalar;
    y *= invScalar;
    z *= invScalar;
    return *this;
}

constexpr Vector3 Vector3::operator-() const noexcept {
    return Vector3(-x, -y, -z);
}

constexpr Vector3 Vector3::operator+() const noexcept {
    return *this;
}

constexpr bool Vector3::operator==(const Vector3& other) const noexcept {
    return IsNearlyEqual(x, other.x) && IsNearlyEqual(y, other.y) && IsNearlyEqual(z, other.z);
}

constexpr bool Vector3::operator!=(const Vector3& other) const noexcept {
    return !(*this == other);
}

#pragma endregion

// =============================================================================
// Vector4 Implementation
// =============================================================================
#pragma region Vector4

constexpr float& Vector4::operator[](size_t index) noexcept {
    return (&x)[index];
}

constexpr const float& Vector4::operator[](size_t index) const noexcept {
    return (&x)[index];
}

constexpr Vector4 Vector4::operator+(const Vector4& other) const noexcept {
    return Vector4(x + other.x, y + other.y, z + other.z, w + other.w);
}

constexpr Vector4 Vector4::operator-(const Vector4& other) const noexcept {
    return Vector4(x - other.x, y - other.y, z - other.z, w - other.w);
}

constexpr Vector4 Vector4::operator*(const Vector4& other) const noexcept {
    return Vector4(x * other.x, y * other.y, z * other.z, w * other.w);
}

constexpr Vector4 Vector4::operator/(const Vector4& other) const noexcept {
    return Vector4(x / other.x, y / other.y, z / other.z, w / other.w);
}

constexpr Vector4 Vector4::operator*(float scalar) const noexcept {
    return Vector4(x * scalar, y * scalar, z * scalar, w * scalar);
}

constexpr Vector4 Vector4::operator/(float scalar) const noexcept {
    const float invScalar = 1.0f / scalar;
    return Vector4(x * invScalar, y * invScalar, z * invScalar, w * invScalar);
}

constexpr Vector4& Vector4::operator+=(const Vector4& other) noexcept {
    x += other.x;
    y += other.y;
    z += other.z;
    w += other.w;
    return *this;
}

constexpr Vector4& Vector4::operator-=(const Vector4& other) noexcept {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    w -= other.w;
    return *this;
}

constexpr Vector4& Vector4::operator*=(const Vector4& other) noexcept {
    x *= other.x;
    y *= other.y;
    z *= other.z;
    w *= other.w;
    return *this;
}

constexpr Vector4& Vector4::operator/=(const Vector4& other) noexcept {
    x /= other.x;
    y /= other.y;
    z /= other.z;
    w /= other.w;
    return *this;
}

constexpr Vector4& Vector4::operator*=(float scalar) noexcept {
    x *= scalar;
    y *= scalar;
    z *= scalar;
    w *= scalar;
    return *this;
}

constexpr Vector4& Vector4::operator/=(float scalar) noexcept {
    const float invScalar = 1.0f / scalar;
    x *= invScalar;
    y *= invScalar;
    z *= invScalar;
    w *= invScalar;
    return *this;
}

constexpr Vector4 Vector4::operator-() const noexcept {
    return Vector4(-x, -y, -z, -w);
}

constexpr Vector4 Vector4::operator+() const noexcept {
    return *this;
}

constexpr bool Vector4::operator==(const Vector4& other) const noexcept {
    return IsNearlyEqual(x, other.x) && IsNearlyEqual(y, other.y) &&
        IsNearlyEqual(z, other.z) && IsNearlyEqual(w, other.w);
}

constexpr bool Vector4::operator!=(const Vector4& other) const noexcept {
    return !(*this == other);
}

#pragma endregion

// =============================================================================
// Matrix3 Implementation
// =============================================================================
#pragma region Matrix3

constexpr Matrix3::Matrix3() noexcept : m{ 0 } {}

constexpr Matrix3::Matrix3(float m00, float m01, float m02,
    float m10, float m11, float m12,
    float m20, float m21, float m22) noexcept {
    m[0] = m00; m[1] = m10; m[2] = m20;
    m[3] = m01; m[4] = m11; m[5] = m21;
    m[6] = m02; m[7] = m12; m[8] = m22;
}

constexpr Matrix3::Matrix3(float diagonal) noexcept : m{ 0 } {
    m[0] = diagonal;
    m[4] = diagonal;
    m[8] = diagonal;
}

constexpr float& Matrix3::operator()(size_t row, size_t col) noexcept {
    return m[col * 3 + row];
}

constexpr const float& Matrix3::operator()(size_t row, size_t col) const noexcept {
    return m[col * 3 + row];
}

constexpr float* Matrix3::operator[](size_t row) noexcept {
    return &m[row * 3];
}

constexpr const float* Matrix3::operator[](size_t row) const noexcept {
    return &m[row * 3];
}

constexpr Matrix3 Matrix3::operator+(const Matrix3& other) const noexcept {
    Matrix3 result;
    for (int i = 0; i < 9; ++i) {
        result.m[i] = m[i] + other.m[i];
    }
    return result;
}

constexpr Matrix3 Matrix3::operator-(const Matrix3& other) const noexcept {
    Matrix3 result;
    for (int i = 0; i < 9; ++i) {
        result.m[i] = m[i] - other.m[i];
    }
    return result;
}

constexpr Matrix3 Matrix3::operator*(const Matrix3& other) const noexcept {
    Matrix3 result;
    for (int col = 0; col < 3; ++col) {
        for (int row = 0; row < 3; ++row) {
            result(row, col) =
                (*this)(row, 0) * other(0, col) +
                (*this)(row, 1) * other(1, col) +
                (*this)(row, 2) * other(2, col);
        }
    }
    return result;
}

constexpr Matrix3 Matrix3::operator*(float scalar) const noexcept {
    Matrix3 result;
    for (int i = 0; i < 9; ++i) {
        result.m[i] = m[i] * scalar;
    }
    return result;
}

constexpr Vector3 Matrix3::operator*(const Vector3& vector) const noexcept {
    return Vector3(
        (*this)(0, 0) * vector.x + (*this)(0, 1) * vector.y + (*this)(0, 2) * vector.z,
        (*this)(1, 0) * vector.x + (*this)(1, 1) * vector.y + (*this)(1, 2) * vector.z,
        (*this)(2, 0) * vector.x + (*this)(2, 1) * vector.y + (*this)(2, 2) * vector.z
    );
}

constexpr Matrix3& Matrix3::operator+=(const Matrix3& other) noexcept {
    for (int i = 0; i < 9; ++i) {
        m[i] += other.m[i];
    }
    return *this;
}

constexpr Matrix3& Matrix3::operator-=(const Matrix3& other) noexcept {
    for (int i = 0; i < 9; ++i) {
        m[i] -= other.m[i];
    }
    return *this;
}

constexpr Matrix3& Matrix3::operator*=(const Matrix3& other) noexcept {
    *this = *this * other;
    return *this;
}

constexpr Matrix3& Matrix3::operator*=(float scalar) noexcept {
    for (int i = 0; i < 9; ++i) {
        m[i] *= scalar;
    }
    return *this;
}

constexpr bool Matrix3::operator==(const Matrix3& other) const noexcept {
    for (int i = 0; i < 9; ++i) {
        if (!IsNearlyEqual(m[i], other.m[i])) {
            return false;
        }
    }
    return true;
}

constexpr bool Matrix3::operator!=(const Matrix3& other) const noexcept {
    return !(*this == other);
}

#pragma endregion

// =============================================================================
// Matrix4 Implementation
// =============================================================================
#pragma region Matrix4

constexpr Matrix4::Matrix4() noexcept : m{ 0 } {}

constexpr Matrix4::Matrix4(float m00, float m01, float m02, float m03,
    float m10, float m11, float m12, float m13,
    float m20, float m21, float m22, float m23,
    float m30, float m31, float m32, float m33) noexcept {
    m[0] = m00; m[1] = m10; m[2] = m20; m[3] = m30;
    m[4] = m01; m[5] = m11; m[6] = m21; m[7] = m31;
    m[8] = m02; m[9] = m12; m[10] = m22; m[11] = m32;
    m[12] = m03; m[13] = m13; m[14] = m23; m[15] = m33;
}

constexpr Matrix4::Matrix4(float diagonal) noexcept : m{ 0 } {
    m[0] = diagonal;
    m[5] = diagonal;
    m[10] = diagonal;
    m[15] = diagonal;
}

constexpr Matrix4::Matrix4(const Matrix3& mat3) noexcept : m{ 0 } {
    m[0] = mat3.m[0]; m[1] = mat3.m[1]; m[2] = mat3.m[2];
    m[4] = mat3.m[3]; m[5] = mat3.m[4]; m[6] = mat3.m[5];
    m[8] = mat3.m[6]; m[9] = mat3.m[7]; m[10] = mat3.m[8];
    m[15] = 1.0f;
}

constexpr float& Matrix4::operator()(size_t row, size_t col) noexcept {
    return m[col * 4 + row];
}

constexpr const float& Matrix4::operator()(size_t row, size_t col) const noexcept {
    return m[col * 4 + row];
}

constexpr float* Matrix4::operator[](size_t row) noexcept {
    return &m[row * 4];
}

constexpr const float* Matrix4::operator[](size_t row) const noexcept {
    return &m[row * 4];
}

constexpr Matrix4 Matrix4::operator+(const Matrix4& other) const noexcept {
    Matrix4 result;
    for (int i = 0; i < 16; ++i) {
        result.m[i] = m[i] + other.m[i];
    }
    return result;
}

constexpr Matrix4 Matrix4::operator-(const Matrix4& other) const noexcept {
    Matrix4 result;
    for (int i = 0; i < 16; ++i) {
        result.m[i] = m[i] - other.m[i];
    }
    return result;
}

constexpr Matrix4 Matrix4::operator*(const Matrix4& other) const noexcept {
    Matrix4 result;
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            result(row, col) =
                (*this)(row, 0) * other(0, col) +
                (*this)(row, 1) * other(1, col) +
                (*this)(row, 2) * other(2, col) +
                (*this)(row, 3) * other(3, col);
        }
    }
    return result;
}

constexpr Matrix4 Matrix4::operator*(float scalar) const noexcept {
    Matrix4 result;
    for (int i = 0; i < 16; ++i) {
        result.m[i] = m[i] * scalar;
    }
    return result;
}

constexpr Vector4 Matrix4::operator*(const Vector4& vector) const noexcept {
    return Vector4(
        (*this)(0, 0) * vector.x + (*this)(0, 1) * vector.y + (*this)(0, 2) * vector.z + (*this)(0, 3) * vector.w,
        (*this)(1, 0) * vector.x + (*this)(1, 1) * vector.y + (*this)(1, 2) * vector.z + (*this)(1, 3) * vector.w,
        (*this)(2, 0) * vector.x + (*this)(2, 1) * vector.y + (*this)(2, 2) * vector.z + (*this)(2, 3) * vector.w,
        (*this)(3, 0) * vector.x + (*this)(3, 1) * vector.y + (*this)(3, 2) * vector.z + (*this)(3, 3) * vector.w
    );
}

constexpr Vector3 Matrix4::TransformPoint(const Vector3& point) const noexcept {
    const Vector4 result = *this * Vector4(point, 1.0f);
    return Vector3(result.x, result.y, result.z) / result.w;
}

constexpr Vector3 Matrix4::TransformVector(const Vector3& vector) const noexcept {
    const Vector4 result = *this * Vector4(vector, 0.0f);
    return Vector3(result.x, result.y, result.z);
}

constexpr Matrix4& Matrix4::operator+=(const Matrix4& other) noexcept {
    for (int i = 0; i < 16; ++i) {
        m[i] += other.m[i];
    }
    return *this;
}

constexpr Matrix4& Matrix4::operator-=(const Matrix4& other) noexcept {
    for (int i = 0; i < 16; ++i) {
        m[i] -= other.m[i];
    }
    return *this;
}

constexpr Matrix4& Matrix4::operator*=(const Matrix4& other) noexcept {
    *this = *this * other;
    return *this;
}

constexpr Matrix4& Matrix4::operator*=(float scalar) noexcept {
    for (int i = 0; i < 16; ++i) {
        m[i] *= scalar;
    }
    return *this;
}

constexpr bool Matrix4::operator==(const Matrix4& other) const noexcept {
    for (int i = 0; i < 16; ++i) {
        if (!IsNearlyEqual(m[i], other.m[i])) {
            return false;
        }
    }
    return true;
}

constexpr bool Matrix4::operator!=(const Matrix4& other) const noexcept {
    return !(*this == other);
}

#pragma endregion

// =============================================================================
// Basic Mathematical Functions
// =============================================================================
#pragma region Utility Functions

constexpr float Math::Abs(float x) noexcept {
    return (x >= 0.0f) ? x : -x;
}

constexpr float Math::Sign(float x) noexcept {
    return (x > 0.0f) ? 1.0f : (x < 0.0f) ? -1.0f : 0.0f;
}

constexpr float Math::Min(float a, float b) noexcept {
    return (a < b) ? a : b;
}

constexpr float Math::Max(float a, float b) noexcept {
    return (a > b) ? a : b;
}

constexpr float Math::Clamp(float value, float min, float max) noexcept {
    return (value < min) ? min : (value > max) ? max : value;
}

constexpr float Math::Saturate(float value) noexcept {
    return Clamp(value, 0.0f, 1.0f);
}

constexpr float Math::ToRadians(float degrees) noexcept {
    return degrees * DEG_TO_RAD;
}

constexpr float Math::ToDegrees(float radians) noexcept {
    return radians * RAD_TO_DEG;
}

constexpr float Math::WrapAngle(float angle) noexcept {
    angle = std::fmod(angle + PI, TWO_PI);
    if (angle < 0) {
        angle += TWO_PI;
    }
    return angle - PI;
}

constexpr float Math::AngleDifference(float a, float b) noexcept {
    return WrapAngle(b - a);
}

constexpr float Math::Lerp(float a, float b, float t) noexcept {
    return a + t * (b - a);
}

constexpr float Math::InverseLerp(float a, float b, float value) noexcept {
    return (value - a) / (b - a);
}

constexpr float Math::SmoothStep(float edge0, float edge1, float x) noexcept {
    const float t = Saturate((x - edge0) / (edge1 - edge0));
    return t * t * (3.0f - 2.0f * t);
}

constexpr float Math::SmootherStep(float edge0, float edge1, float x) noexcept {
    const float t = Saturate((x - edge0) / (edge1 - edge0));
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

#pragma endregion

// =============================================================================
// Vector Operations
// =============================================================================
#pragma region Vector Operations

constexpr float Math::Dot(const Vector2& a, const Vector2& b) noexcept {
    return a.x * b.x + a.y * b.y;
}

constexpr float Math::Cross(const Vector2& a, const Vector2& b) noexcept {
    return a.x * b.y - a.y * b.x;
}

constexpr float Math::Length(const Vector2& v) noexcept {
    return std::sqrt(LengthSquared(v));
}

constexpr float Math::LengthSquared(const Vector2& v) noexcept {
    return Dot(v, v);
}

constexpr Vector2 Math::Normalize(const Vector2& v) noexcept {
    const float length = Length(v);
    return IsNearlyZero(length) ? Vector2::ZERO : v / length;
}

constexpr float Math::Distance(const Vector2& a, const Vector2& b) noexcept {
    return Length(b - a);
}

constexpr float Math::DistanceSquared(const Vector2& a, const Vector2& b) noexcept {
    return LengthSquared(b - a);
}

constexpr Vector2 Math::Reflect(const Vector2& incident, const Vector2& normal) noexcept {
    return incident - normal * 2.0f * Dot(incident, normal);
}

constexpr Vector2 Math::Project(const Vector2& a, const Vector2& b) noexcept {
    return b * (Dot(a, b) / LengthSquared(b));
}

constexpr Vector2 Math::Perpendicular(const Vector2& v) noexcept {
    return Vector2(-v.y, v.x);
}

constexpr float Math::Angle(const Vector2& a, const Vector2& b) noexcept {
    const float dot = Dot(Normalize(a), Normalize(b));
    return std::acos(Clamp(dot, -1.0f, 1.0f));
}



constexpr float Math::Dot(const Vector3& a, const Vector3& b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

constexpr Vector3 Math::Cross(const Vector3& a, const Vector3& b) noexcept {
    return Vector3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

constexpr float Math::Length(const Vector3& v) noexcept {
    return std::sqrt(LengthSquared(v));
}

constexpr float Math::LengthSquared(const Vector3& v) noexcept {
    return Dot(v, v);
}

constexpr Vector3 Math::Normalize(const Vector3& v) noexcept {
    const float length = Length(v);
    return IsNearlyZero(length) ? Vector3::ZERO : v / length;
}

constexpr float Math::Distance(const Vector3& a, const Vector3& b) noexcept {
    return Length(b - a);
}

constexpr float Math::DistanceSquared(const Vector3& a, const Vector3& b) noexcept {
    return LengthSquared(b - a);
}

constexpr Vector3 Math::Reflect(const Vector3& incident, const Vector3& normal) noexcept {
    return incident - normal * 2.0f * Dot(incident, normal);
}

constexpr Vector3 Math::Refract(const Vector3& incident, const Vector3& normal, float ior) noexcept {
    const float cosI = -Dot(normal, incident);
    const float sinT2 = ior * ior * (1.0f - cosI * cosI);

    if (sinT2 > 1.0f) {
        return Vector3::ZERO; // Total internal reflection
    }

    const float cosT = std::sqrt(1.0f - sinT2);
    const Vector3 inc = incident * ior;
    const Vector3 nor = normal * (ior * cosI - cosT);
    return inc + nor;
}

constexpr Vector3 Math::Project(const Vector3& a, const Vector3& b) noexcept {
    return b * (Dot(a, b) / LengthSquared(b));
}

constexpr float Math::Angle(const Vector3& a, const Vector3& b) noexcept {
    const float dot = Dot(Normalize(a), Normalize(b));
    return std::acos(Clamp(dot, -1.0f, 1.0f));
}

constexpr float Math::Dot(const Vector4& a, const Vector4& b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

constexpr float Math::Length(const Vector4& v) noexcept {
    return std::sqrt(LengthSquared(v));
}

constexpr float Math::LengthSquared(const Vector4& v) noexcept {
    return Dot(v, v);
}

constexpr Vector4 Math::Normalize(const Vector4& v) noexcept {
    const float length = Length(v);
    return IsNearlyZero(length) ? Vector4::ZERO : v / length;
}

constexpr float Math::Distance(const Vector4& a, const Vector4& b) noexcept {
    return Length(b - a);
}

constexpr float Math::DistanceSquared(const Vector4& a, const Vector4& b) noexcept {
    return LengthSquared(b - a);
}

#pragma endregion

// =============================================================================
// Utility Functions
// =============================================================================
#pragma region Utility Functions

constexpr bool Math::IsNearlyEqual(float a, float b, float epsilon) noexcept {
    return Abs(a - b) <= epsilon;
}

constexpr bool Math::IsNearlyZero(float value, float epsilon) noexcept {
    return Abs(value) <= epsilon;
}

constexpr bool Math::IsFinite(float value) noexcept {
    return std::isfinite(value);
}

constexpr bool Math::IsInfinite(float value) noexcept {
    return std::isinf(value);
}

constexpr bool Math::IsNaN(float value) noexcept {
    return std::isnan(value);
}

constexpr bool Math::IsPowerOfTwo(uint32_t value) noexcept {
    return value > 0 && (value & (value - 1)) == 0;
}

constexpr uint32_t Math::NextPowerOfTwo(uint32_t value) noexcept {
    if (value == 0) return 1;
    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    return value + 1;
}

constexpr uint32_t Math::PreviousPowerOfTwo(uint32_t value) noexcept {
    if (value == 0) return 0;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    return value - (value >> 1);
}

constexpr uint32_t Math::CountSetBits(uint32_t value) noexcept {
    uint32_t count = 0;
    while (value) {
        count += value & 1;
        value >>= 1;
    }
    return count;
}

constexpr uint32_t Math::CountLeadingZeros(uint32_t value) noexcept {
    if (value == 0) return 32;
    uint32_t count = 0;
    if (value <= 0x0000FFFF) { count += 16; value <<= 16; }
    if (value <= 0x00FFFFFF) { count += 8; value <<= 8; }
    if (value <= 0x0FFFFFFF) { count += 4; value <<= 4; }
    if (value <= 0x3FFFFFFF) { count += 2; value <<= 2; }
    if (value <= 0x7FFFFFFF) { count += 1; }
    return count;
}

constexpr uint32_t Math::CountTrailingZeros(uint32_t value) noexcept {
    if (value == 0) return 32;
    uint32_t count = 0;
    while ((value & 1) == 0) {
        count++;
        value >>= 1;
    }
    return count;
}

// Global random instance (will be properly initialized in implementation)
static Random g_random;

Random& Math::GlobalRandom() noexcept {
    return g_random;
}

// Convenience random functions
float Math::RandomFloat() noexcept {
    return GlobalRandom().NextFloat();
}

float Math::RandomFloat(float min, float max) noexcept {
    return GlobalRandom().NextFloat(min, max);
}

int32_t Math::RandomInt(int32_t min, int32_t max) noexcept {
    return GlobalRandom().NextInt(min, max);
}

bool Math::RandomBool() noexcept {
    return GlobalRandom().NextBool();
}

Vector3 Math::RandomUnitVector3() noexcept {
    return GlobalRandom().NextUnitVector3();
}

Vector3 Math::RandomPointInSphere() noexcept {
    return GlobalRandom().NextPointInSphere();
}

Vector3 Math::RandomPointOnSphere() noexcept {
    return GlobalRandom().NextPointOnSphere();
}

#pragma endregion


// =============================================================================
// Matrix3 Operations
// =============================================================================

constexpr float Math::Determinant(const Matrix3& m) noexcept {
    return m(0, 0) * (m(1, 1) * m(2, 2) - m(1, 2) * m(2, 1)) -
        m(0, 1) * (m(1, 0) * m(2, 2) - m(1, 2) * m(2, 0)) +
        m(0, 2) * (m(1, 0) * m(2, 1) - m(1, 1) * m(2, 0));
}

constexpr Matrix3 Math::Transpose(const Matrix3& m) noexcept {
    return Matrix3(
        m(0, 0), m(1, 0), m(2, 0),
        m(0, 1), m(1, 1), m(2, 1),
        m(0, 2), m(1, 2), m(2, 2)
    );
}

constexpr Matrix3 Math::Inverse(const Matrix3& m) noexcept {
    const float det = Determinant(m);

    if (IsNearlyZero(det)) {
        return Matrix3::IDENTITY; // Return identity for singular matrices
    }

    const float invDet = 1.0f / det;

    return Matrix3(
        (m(1, 1) * m(2, 2) - m(1, 2) * m(2, 1)) * invDet,
        (m(0, 2) * m(2, 1) - m(0, 1) * m(2, 2)) * invDet,
        (m(0, 1) * m(1, 2) - m(0, 2) * m(1, 1)) * invDet,

        (m(1, 2) * m(2, 0) - m(1, 0) * m(2, 2)) * invDet,
        (m(0, 0) * m(2, 2) - m(0, 2) * m(2, 0)) * invDet,
        (m(0, 2) * m(1, 0) - m(0, 0) * m(1, 2)) * invDet,

        (m(1, 0) * m(2, 1) - m(1, 1) * m(2, 0)) * invDet,
        (m(0, 1) * m(2, 0) - m(0, 0) * m(2, 1)) * invDet,
        (m(0, 0) * m(1, 1) - m(0, 1) * m(1, 0)) * invDet
    );
}

constexpr Matrix3 Math::Scale(const Vector2& scale) noexcept {
    return Matrix3(
        scale.x, 0.0f, 0.0f,
        0.0f, scale.y, 0.0f,
        0.0f, 0.0f, 1.0f
    );
}

constexpr Matrix3 Math::Rotation(float angle) noexcept {
    const float cos_a = std::cos(angle);
    const float sin_a = std::sin(angle);

    return Matrix3(
        cos_a, -sin_a, 0.0f,
        sin_a, cos_a, 0.0f,
        0.0f, 0.0f, 1.0f
    );
}

constexpr Matrix3 Math::Translation(const Vector2& translation) noexcept {
    return Matrix3(
        1.0f, 0.0f, translation.x,
        0.0f, 1.0f, translation.y,
        0.0f, 0.0f, 1.0f
    );
}

// =============================================================================
// Matrix4 Operations
// =============================================================================

constexpr float Math::Determinant(const Matrix4& m) noexcept {
    const float a = m(0, 0), b = m(0, 1), c = m(0, 2), d = m(0, 3);
    const float e = m(1, 0), f = m(1, 1), g = m(1, 2), h = m(1, 3);
    const float i = m(2, 0), j = m(2, 1), k = m(2, 2), l = m(2, 3);
    const float mm = m(3, 0), n = m(3, 1), o = m(3, 2), p = m(3, 3);

    return a * (f * (k * p - l * o) - g * (j * p - l * n) + h * (j * o - k * n)) -
        b * (e * (k * p - l * o) - g * (i * p - l * mm) + h * (i * o - k * mm)) +
        c * (e * (j * p - l * n) - f * (i * p - l * mm) + h * (i * n - j * mm)) -
        d * (e * (j * o - k * n) - f * (i * o - k * mm) + g * (i * n - j * mm));
}

constexpr Matrix4 Math::Transpose(const Matrix4& m) noexcept {
    return Matrix4(
        m(0, 0), m(1, 0), m(2, 0), m(3, 0),
        m(0, 1), m(1, 1), m(2, 1), m(3, 1),
        m(0, 2), m(1, 2), m(2, 2), m(3, 2),
        m(0, 3), m(1, 3), m(2, 3), m(3, 3)
    );
}

constexpr Matrix4 Math::Inverse(const Matrix4& m) noexcept {
    const float a = m(0, 0), b = m(0, 1), c = m(0, 2), d = m(0, 3);
    const float e = m(1, 0), f = m(1, 1), g = m(1, 2), h = m(1, 3);
    const float i = m(2, 0), j = m(2, 1), k = m(2, 2), l = m(2, 3);
    const float mm = m(3, 0), n = m(3, 1), o = m(3, 2), p = m(3, 3);

    const float det = Determinant(m);

    if (IsNearlyZero(det)) {
        return Matrix4::IDENTITY; // Return identity for singular matrices
    }

    const float invDet = 1.0f / det;

    Matrix4 result;

    result(0, 0) = (f * (k * p - l * o) - g * (j * p - l * n) + h * (j * o - k * n)) * invDet;
    result(0, 1) = -(b * (k * p - l * o) - c * (j * p - l * n) + d * (j * o - k * n)) * invDet;
    result(0, 2) = (b * (g * p - h * o) - c * (f * p - h * n) + d * (f * o - g * n)) * invDet;
    result(0, 3) = -(b * (g * l - h * k) - c * (f * l - h * j) + d * (f * k - g * j)) * invDet;

    result(1, 0) = -(e * (k * p - l * o) - g * (i * p - l * mm) + h * (i * o - k * mm)) * invDet;
    result(1, 1) = (a * (k * p - l * o) - c * (i * p - l * mm) + d * (i * o - k * mm)) * invDet;
    result(1, 2) = -(a * (g * p - h * o) - c * (e * p - h * mm) + d * (e * o - g * mm)) * invDet;
    result(1, 3) = (a * (g * l - h * k) - c * (e * l - h * i) + d * (e * k - g * i)) * invDet;

    result(2, 0) = (e * (j * p - l * n) - f * (i * p - l * mm) + h * (i * n - j * mm)) * invDet;
    result(2, 1) = -(a * (j * p - l * n) - b * (i * p - l * mm) + d * (i * n - j * mm)) * invDet;
    result(2, 2) = (a * (f * p - h * n) - b * (e * p - h * mm) + d * (e * n - f * mm)) * invDet;
    result(2, 3) = -(a * (f * l - h * j) - b * (e * l - h * i) + d * (e * j - f * i)) * invDet;

    result(3, 0) = -(e * (j * o - k * n) - f * (i * o - k * mm) + g * (i * n - j * mm)) * invDet;
    result(3, 1) = (a * (j * o - k * n) - b * (i * o - k * mm) + c * (i * n - j * mm)) * invDet;
    result(3, 2) = -(a * (f * o - g * n) - b * (e * o - g * mm) + c * (e * n - f * mm)) * invDet;
    result(3, 3) = (a * (f * k - g * j) - b * (e * k - g * i) + c * (e * j - f * i)) * invDet;

    return result;
}

constexpr Matrix4 Math::Scale(const Vector3& scale) noexcept {
    return Matrix4(
        scale.x, 0.0f, 0.0f, 0.0f,
        0.0f, scale.y, 0.0f, 0.0f,
        0.0f, 0.0f, scale.z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

constexpr Matrix4 Math::RotationX(float angle) noexcept {
    const float cos_a = std::cos(angle);
    const float sin_a = std::sin(angle);

    return Matrix4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, cos_a, -sin_a, 0.0f,
        0.0f, sin_a, cos_a, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

constexpr Matrix4 Math::RotationY(float angle) noexcept {
    const float cos_a = std::cos(angle);
    const float sin_a = std::sin(angle);

    return Matrix4(
        cos_a, 0.0f, sin_a, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        -sin_a, 0.0f, cos_a, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

constexpr Matrix4 Math::RotationZ(float angle) noexcept {
    const float cos_a = std::cos(angle);
    const float sin_a = std::sin(angle);

    return Matrix4(
        cos_a, -sin_a, 0.0f, 0.0f,
        sin_a, cos_a, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

constexpr Matrix4 Math::Rotation(const Vector3& axis, float angle) noexcept {
    const Vector3 normalizedAxis = Normalize(axis);
    const float cos_a = std::cos(angle);
    const float sin_a = std::sin(angle);
    const float one_minus_cos = 1.0f - cos_a;

    const float x = normalizedAxis.x;
    const float y = normalizedAxis.y;
    const float z = normalizedAxis.z;

    return Matrix4(
        cos_a + x * x * one_minus_cos, x * y * one_minus_cos - z * sin_a, x * z * one_minus_cos + y * sin_a, 0.0f,
        y * x * one_minus_cos + z * sin_a, cos_a + y * y * one_minus_cos, y * z * one_minus_cos - x * sin_a, 0.0f,
        z * x * one_minus_cos - y * sin_a, z * y * one_minus_cos + x * sin_a, cos_a + z * z * one_minus_cos, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

constexpr Matrix4 Math::Translation(const Vector3& translation) noexcept {
    return Matrix4(
        1.0f, 0.0f, 0.0f, translation.x,
        0.0f, 1.0f, 0.0f, translation.y,
        0.0f, 0.0f, 1.0f, translation.z,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

constexpr Matrix4 Math::LookAt(const Vector3& eye, const Vector3& target, const Vector3& up) noexcept {
    const Vector3 forward = Normalize(target - eye);
    const Vector3 side = Normalize(Cross(forward, up));
    const Vector3 upVector = Cross(side, forward);

    Matrix4 result = Matrix4::IDENTITY;

    result(0, 0) = side.x;
    result(1, 0) = side.y;
    result(2, 0) = side.z;
    result(0, 1) = upVector.x;
    result(1, 1) = upVector.y;
    result(2, 1) = upVector.z;
    result(0, 2) = -forward.x;
    result(1, 2) = -forward.y;
    result(2, 2) = -forward.z;
    result(3, 0) = -Dot(side, eye);
    result(3, 1) = -Dot(upVector, eye);
    result(3, 2) = Dot(forward, eye);

    return result;
}

// =============================================================================
// Projection Matrices
// =============================================================================

constexpr Matrix4 Math::Perspective(float fovY, float aspect, float near, float far) noexcept {
    const float tanHalfFovY = std::tan(fovY * 0.5f);
    const float range = far - near;

    Matrix4 result = Matrix4::ZERO;
    result(0, 0) = 1.0f / (aspect * tanHalfFovY);
    result(1, 1) = 1.0f / tanHalfFovY;
    result(2, 2) = -(far + near) / range;
    result(2, 3) = -1.0f;
    result(3, 2) = -(2.0f * far * near) / range;

    return result;
}

constexpr Matrix4 Math::PerspectiveReversedZ(float fovY, float aspect, float near, float far) noexcept {
    const float tanHalfFovY = std::tan(fovY * 0.5f);
    const float range = far - near;

    Matrix4 result = Matrix4::ZERO;
    result(0, 0) = 1.0f / (aspect * tanHalfFovY);
    result(1, 1) = 1.0f / tanHalfFovY;
    result(2, 2) = near / range;
    result(2, 3) = -1.0f;
    result(3, 2) = (far * near) / range;

    return result;
}

constexpr Matrix4 Math::Orthographic(float left, float right, float bottom, float top, float near, float far) noexcept {
    const float width = right - left;
    const float height = top - bottom;
    const float depth = far - near;

    Matrix4 result = Matrix4::ZERO;
    result(0, 0) = 2.0f / width;
    result(1, 1) = 2.0f / height;
    result(2, 2) = -2.0f / depth;
    result(3, 0) = -(right + left) / width;
    result(3, 1) = -(top + bottom) / height;
    result(3, 2) = -(far + near) / depth;
    result(3, 3) = 1.0f;

    return result;
}

constexpr Matrix4 Math::OrthographicReversedZ(float left, float right, float bottom, float top, float near, float far) noexcept {
    const float width = right - left;
    const float height = top - bottom;
    const float depth = far - near;

    Matrix4 result = Matrix4::ZERO;
    result(0, 0) = 2.0f / width;
    result(1, 1) = 2.0f / height;
    result(2, 2) = 1.0f / depth;
    result(3, 0) = -(right + left) / width;
    result(3, 1) = -(top + bottom) / height;
    result(3, 2) = -near / depth;
    result(3, 3) = 1.0f;

    return result;
}

// =============================================================================
// Matrix Decomposition and Composition
// =============================================================================

bool Math::Decompose(const Matrix4& matrix, Vector3& translation, Quaternion& rotation, Vector3& scale) noexcept {
    // Extract translation
    translation = Vector3(matrix(3, 0), matrix(3, 1), matrix(3, 2));

    // Extract scale
    const Vector3 col0(matrix(0, 0), matrix(1, 0), matrix(2, 0));
    const Vector3 col1(matrix(0, 1), matrix(1, 1), matrix(2, 1));
    const Vector3 col2(matrix(0, 2), matrix(1, 2), matrix(2, 2));

    scale.x = Length(col0);
    scale.y = Length(col1);
    scale.z = Length(col2);

    // Check for negative determinant (which means we have a negative scale)
    const float det = Determinant(Matrix3(
        matrix(0, 0), matrix(0, 1), matrix(0, 2),
        matrix(1, 0), matrix(1, 1), matrix(1, 2),
        matrix(2, 0), matrix(2, 1), matrix(2, 2)
    ));

    if (det < 0.0f) {
        scale.x = -scale.x;
    }

    // Remove scaling from the matrix
    if (IsNearlyZero(scale.x) || IsNearlyZero(scale.y) || IsNearlyZero(scale.z)) {
        rotation = Quaternion::IDENTITY;
        return false; // Cannot decompose if any scale component is zero
    }

    const Matrix3 rotationMatrix(
        matrix(0, 0) / scale.x, matrix(0, 1) / scale.y, matrix(0, 2) / scale.z,
        matrix(1, 0) / scale.x, matrix(1, 1) / scale.y, matrix(1, 2) / scale.z,
        matrix(2, 0) / scale.x, matrix(2, 1) / scale.y, matrix(2, 2) / scale.z
    );

    rotation = FromRotationMatrix(rotationMatrix);

    return true;
}

Matrix4 Math::Compose(const Vector3& translation, const Quaternion& rotation, const Vector3& scale) noexcept {
    const Matrix4 rotationMatrix = ToMatrix(rotation);
    const Matrix4 scaleMatrix = Scale(scale);
    const Matrix4 translationMatrix = Translation(translation);

    return translationMatrix * rotationMatrix * scaleMatrix;
}

// =============================================================================
// Scalar Operators for Matrix Types
// =============================================================================

constexpr Matrix3 operator*(float scalar, const Matrix3& matrix) noexcept {
    return matrix * scalar;
}

constexpr Matrix4 operator*(float scalar, const Matrix4& matrix) noexcept {
    return matrix * scalar;
}

constexpr Vector2 operator*(float scalar, const Vector2& vector) noexcept {
    return vector * scalar;
}

constexpr Vector3 operator*(float scalar, const Vector3& vector) noexcept {
    return vector * scalar;
}

constexpr Vector4 operator*(float scalar, const Vector4& vector) noexcept {
    return vector * scalar;
}

constexpr Quaternion operator*(float scalar, const Quaternion& quaternion) noexcept {
    return quaternion * scalar;
}


// =============================================================================
// Ray Implementation
// =============================================================================

constexpr Vector3 Ray::GetPoint(float t) const noexcept {
    return origin + direction * t;
}

// =============================================================================
// Plane Implementation
// =============================================================================

constexpr Plane::Plane(const Vector3& normal, const Vector3& point) noexcept
    : normal(Normalize(normal)), distance(Dot(this->normal, point)) {
}

constexpr Plane::Plane(const Vector3& p1, const Vector3& p2, const Vector3& p3) noexcept {
    const Vector3 edge1 = p2 - p1;
    const Vector3 edge2 = p3 - p1;
    normal = Normalize(Cross(edge1, edge2));
    distance = Dot(normal, p1);
}

constexpr float Plane::DistanceToPoint(const Vector3& point) const noexcept {
    return Dot(normal, point) - distance;
}

constexpr Vector3 Plane::ClosestPoint(const Vector3& point) const noexcept {
    return point - normal * DistanceToPoint(point);
}

// =============================================================================
// AABB Implementation
// =============================================================================

constexpr AABB::AABB(const Vector3& center, float radius) noexcept
    : min(center - Vector3(radius)), max(center + Vector3(radius)) {
}

constexpr Vector3 AABB::Center() const noexcept {
    return (min + max) * 0.5f;
}

constexpr Vector3 AABB::Size() const noexcept {
    return max - min;
}

constexpr Vector3 AABB::Extents() const noexcept {
    return Size() * 0.5f;
}

constexpr float AABB::Volume() const noexcept {
    const Vector3 size = Size();
    return size.x * size.y * size.z;
}

constexpr bool AABB::Contains(const Vector3& point) const noexcept {
    return point.x >= min.x && point.x <= max.x &&
        point.y >= min.y && point.y <= max.y &&
        point.z >= min.z && point.z <= max.z;
}

constexpr bool AABB::Intersects(const AABB& other) const noexcept {
    return max.x >= other.min.x && min.x <= other.max.x &&
        max.y >= other.min.y && min.y <= other.max.y &&
        max.z >= other.min.z && min.z <= other.max.z;
}

constexpr AABB AABB::Union(const AABB& other) const noexcept {
    return AABB(
        Vector3(Min(min.x, other.min.x), Min(min.y, other.min.y), Min(min.z, other.min.z)),
        Vector3(Max(max.x, other.max.x), Max(max.y, other.max.y), Max(max.z, other.max.z))
    );
}

constexpr AABB AABB::Intersection(const AABB& other) const noexcept {
    const Vector3 newMin(Max(min.x, other.min.x), Max(min.y, other.min.y), Max(min.z, other.min.z));
    const Vector3 newMax(Min(max.x, other.max.x), Min(max.y, other.max.y), Min(max.z, other.max.z));

    // Check if intersection is valid
    if (newMin.x <= newMax.x && newMin.y <= newMax.y && newMin.z <= newMax.z) {
        return AABB(newMin, newMax);
    }

    return AABB(); // Empty AABB
}

constexpr void AABB::Expand(const Vector3& point) noexcept {
    min.x = Min(min.x, point.x);
    min.y = Min(min.y, point.y);
    min.z = Min(min.z, point.z);
    max.x = Max(max.x, point.x);
    max.y = Max(max.y, point.y);
    max.z = Max(max.z, point.z);
}

constexpr void AABB::Expand(float amount) noexcept {
    const Vector3 expansion(amount);
    min -= expansion;
    max += expansion;
}

// =============================================================================
// OBB Implementation
// =============================================================================

constexpr bool OBB::Contains(const Vector3& point) const noexcept {
    const Vector3 localPoint = Transpose(rotation) * (point - center);
    return Abs(localPoint.x) <= extents.x &&
        Abs(localPoint.y) <= extents.y &&
        Abs(localPoint.z) <= extents.z;
}

constexpr bool OBB::Intersects(const OBB& other) const noexcept {
    // SAT (Separating Axis Theorem) implementation
    const Vector3 translation = other.center - center;

    // Test axes from this OBB
    for (int i = 0; i < 3; ++i) {
        Vector3 axis;
        axis[i] = 1.0f;
        axis = rotation * axis;

        float projectedDistance = Abs(Dot(translation, axis));
        float projectedExtents = extents[i];

        // Project other OBB onto this axis
        float otherProjectedExtents = 0.0f;
        for (int j = 0; j < 3; ++j) {
            Vector3 otherAxis;
            otherAxis[j] = 1.0f;
            otherAxis = other.rotation * otherAxis;
            otherProjectedExtents += other.extents[j] * Abs(Dot(axis, otherAxis));
        }

        if (projectedDistance > projectedExtents + otherProjectedExtents) {
            return false;
        }
    }

    // Test axes from other OBB
    for (int i = 0; i < 3; ++i) {
        Vector3 axis;
        axis[i] = 1.0f;
        axis = other.rotation * axis;

        float projectedDistance = Abs(Dot(translation, axis));
        float projectedExtents = other.extents[i];

        // Project this OBB onto other's axis
        float thisProjectedExtents = 0.0f;
        for (int j = 0; j < 3; ++j) {
            Vector3 thisAxis;
            thisAxis[j] = 1.0f;
            thisAxis = rotation * thisAxis;
            thisProjectedExtents += extents[j] * Abs(Dot(axis, thisAxis));
        }

        if (projectedDistance > projectedExtents + thisProjectedExtents) {
            return false;
        }
    }

    // Test cross product axes
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            Vector3 axis1, axis2;
            axis1[i] = 1.0f;
            axis2[j] = 1.0f;
            axis1 = rotation * axis1;
            axis2 = other.rotation * axis2;

            const Vector3 crossAxis = Cross(axis1, axis2);
            if (LengthSquared(crossAxis) < EPSILON) continue; // Parallel axes

            const Vector3 normalizedCrossAxis = Normalize(crossAxis);
            const float projectedDistance = Abs(Dot(translation, normalizedCrossAxis));

            float thisProjectedExtents = 0.0f;
            float otherProjectedExtents = 0.0f;

            for (int k = 0; k < 3; ++k) {
                Vector3 thisAxis, otherAxis;
                thisAxis[k] = 1.0f;
                otherAxis[k] = 1.0f;
                thisAxis = rotation * thisAxis;
                otherAxis = other.rotation * otherAxis;

                thisProjectedExtents += extents[k] * Abs(Dot(normalizedCrossAxis, thisAxis));
                otherProjectedExtents += other.extents[k] * Abs(Dot(normalizedCrossAxis, otherAxis));
            }

            if (projectedDistance > thisProjectedExtents + otherProjectedExtents) {
                return false;
            }
        }
    }

    return true; // No separating axis found
}

constexpr AABB OBB::ToAABB() const noexcept {
    Vector3 min_point = center;
    Vector3 max_point = center;

    for (int i = 0; i < 3; ++i) {
        Vector3 axis;
        axis[i] = 1.0f;
        axis = rotation * axis;
        axis *= extents[i];

        min_point -= Vector3(Abs(axis.x), Abs(axis.y), Abs(axis.z));
        max_point += Vector3(Abs(axis.x), Abs(axis.y), Abs(axis.z));
    }

    return AABB(min_point, max_point);
}

// =============================================================================
// Sphere Implementation
// =============================================================================

constexpr bool Sphere::Contains(const Vector3& point) const noexcept {
    return DistanceSquared(center, point) <= radius * radius;
}

constexpr bool Sphere::Intersects(const Sphere& other) const noexcept {
    const float combinedRadius = radius + other.radius;
    return DistanceSquared(center, other.center) <= combinedRadius * combinedRadius;
}

constexpr bool Sphere::Intersects(const AABB& aabb) const noexcept {
    const Vector3 closestPoint = Vector3(
        Clamp(center.x, aabb.min.x, aabb.max.x),
        Clamp(center.y, aabb.min.y, aabb.max.y),
        Clamp(center.z, aabb.min.z, aabb.max.z)
    );

    return DistanceSquared(center, closestPoint) <= radius * radius;
}

constexpr AABB Sphere::ToAABB() const noexcept {
    const Vector3 radiusVec(radius);
    return AABB(center - radiusVec, center + radiusVec);
}

constexpr float Sphere::Volume() const noexcept {
    return (4.0f / 3.0f) * PI * radius * radius * radius;
}

constexpr float Sphere::SurfaceArea() const noexcept {
    return 4.0f * PI * radius * radius;
}

// =============================================================================
// Triangle Implementation
// =============================================================================

constexpr Vector3 Triangle::Normal() const noexcept {
    return Normalize(Cross(v1 - v0, v2 - v0));
}

constexpr Vector3 Triangle::Center() const noexcept {
    return (v0 + v1 + v2) / 3.0f;
}

constexpr float Triangle::Area() const noexcept {
    return Length(Cross(v1 - v0, v2 - v0)) * 0.5f;
}

constexpr bool Triangle::Contains(const Vector3& point) const noexcept {
    // Barycentric coordinate method
    const Vector3 edge0 = v1 - v0;
    const Vector3 edge1 = v2 - v0;
    const Vector3 pointToV0 = point - v0;

    const float dot00 = Dot(edge0, edge0);
    const float dot01 = Dot(edge0, edge1);
    const float dot02 = Dot(edge0, pointToV0);
    const float dot11 = Dot(edge1, edge1);
    const float dot12 = Dot(edge1, pointToV0);

    const float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
    const float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    const float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

    return (u >= 0) && (v >= 0) && (u + v <= 1);
}

constexpr Vector3 Triangle::ClosestPoint(const Vector3& point) const noexcept {
    const Vector3 edge0 = v1 - v0;
    const Vector3 edge1 = v2 - v0;
    const Vector3 pointToV0 = v0 - point;

    const float a = Dot(edge0, edge0);
    const float b = Dot(edge0, edge1);
    const float c = Dot(edge1, edge1);
    const float d = Dot(edge0, pointToV0);
    const float e = Dot(edge1, pointToV0);

    const float det = a * c - b * b;
    float s = b * e - c * d;
    float t = b * d - a * e;

    if (s + t < det) {
        if (s < 0.0f) {
            if (t < 0.0f) {
                // Region 4
                if (d < 0.0f) {
                    t = 0.0f;
                    s = (-d >= a) ? 1.0f : -d / a;
                }
                else {
                    s = 0.0f;
                    t = (e >= 0.0f) ? 0.0f : ((-e >= c) ? 1.0f : -e / c);
                }
            }
            else {
                // Region 3
                s = 0.0f;
                t = (e >= 0.0f) ? 0.0f : ((-e >= c) ? 1.0f : -e / c);
            }
        }
        else if (t < 0.0f) {
            // Region 5
            t = 0.0f;
            s = (d >= 0.0f) ? 0.0f : ((-d >= a) ? 1.0f : -d / a);
        }
        else {
            // Region 0
            const float invDet = 1.0f / det;
            s *= invDet;
            t *= invDet;
        }
    }
    else {
        if (s < 0.0f) {
            // Region 2
            const float tmp0 = b + d;
            const float tmp1 = c + e;
            if (tmp1 > tmp0) {
                const float numer = tmp1 - tmp0;
                const float denom = a - 2.0f * b + c;
                s = (numer >= denom) ? 1.0f : numer / denom;
                t = 1.0f - s;
            }
            else {
                t = 0.0f;
                s = (tmp0 <= 0.0f) ? 1.0f : ((d >= 0.0f) ? 0.0f : -d / a);
            }
        }
        else if (t < 0.0f) {
            // Region 6
            const float tmp0 = b + e;
            const float tmp1 = a + d;
            if (tmp1 > tmp0) {
                const float numer = tmp1 - tmp0;
                const float denom = a - 2.0f * b + c;
                t = (numer >= denom) ? 1.0f : numer / denom;
                s = 1.0f - t;
            }
            else {
                s = 0.0f;
                t = (tmp0 <= 0.0f) ? 1.0f : ((e >= 0.0f) ? 0.0f : -e / c);
            }
        }
        else {
            // Region 1
            const float numer = c + e - b - d;
            if (numer <= 0.0f) {
                s = 0.0f;
            }
            else {
                const float denom = a - 2.0f * b + c;
                s = (numer >= denom) ? 1.0f : numer / denom;
            }
            t = 1.0f - s;
        }
    }

    return v0 + s * edge0 + t * edge1;
}

// =============================================================================
// Frustum Implementation
// =============================================================================

constexpr Frustum::Frustum(const Matrix4& viewProjectionMatrix) noexcept {
    // Extract frustum planes from view-projection matrix
    const float* m = viewProjectionMatrix.m;

    // Left plane
    planes[0] = Plane(
        Vector3(m[3] + m[0], m[7] + m[4], m[11] + m[8]),
        m[15] + m[12]
    );

    // Right plane
    planes[1] = Plane(
        Vector3(m[3] - m[0], m[7] - m[4], m[11] - m[8]),
        m[15] - m[12]
    );

    // Bottom plane
    planes[2] = Plane(
        Vector3(m[3] + m[1], m[7] + m[5], m[11] + m[9]),
        m[15] + m[13]
    );

    // Top plane
    planes[3] = Plane(
        Vector3(m[3] - m[1], m[7] - m[5], m[11] - m[9]),
        m[15] - m[13]
    );

    // Near plane
    planes[4] = Plane(
        Vector3(m[3] + m[2], m[7] + m[6], m[11] + m[10]),
        m[15] + m[14]
    );

    // Far plane
    planes[5] = Plane(
        Vector3(m[3] - m[2], m[7] - m[6], m[11] - m[10]),
        m[15] - m[14]
    );

    // Normalize all planes
    for (auto& plane : planes) {
        const float length = Length(plane.normal);
        if (!IsNearlyZero(length)) {
            plane.normal /= length;
            plane.distance /= length;
        }
    }
}

constexpr bool Frustum::Contains(const Vector3& point) const noexcept {
    for (const auto& plane : planes) {
        if (plane.DistanceToPoint(point) < 0.0f) {
            return false;
        }
    }
    return true;
}

constexpr bool Frustum::Intersects(const Sphere& sphere) const noexcept {
    for (const auto& plane : planes) {
        if (plane.DistanceToPoint(sphere.center) < -sphere.radius) {
            return false;
        }
    }
    return true;
}

constexpr bool Frustum::Intersects(const AABB& aabb) const noexcept {
    for (const auto& plane : planes) {
        // Find the positive vertex (furthest in the direction of the plane normal)
        Vector3 positiveVertex = aabb.min;
        if (plane.normal.x >= 0) positiveVertex.x = aabb.max.x;
        if (plane.normal.y >= 0) positiveVertex.y = aabb.max.y;
        if (plane.normal.z >= 0) positiveVertex.z = aabb.max.z;

        // If the positive vertex is behind the plane, the AABB is outside
        if (plane.DistanceToPoint(positiveVertex) < 0.0f) {
            return false;
        }
    }
    return true;
}

// =============================================================================
// Frustum Utility Function
// =============================================================================

void Math::ExtractFrustumPlanes(const Matrix4& viewProjectionMatrix, std::array<Plane, 6>& planes) noexcept {
    const Frustum frustum(viewProjectionMatrix);
    planes = frustum.planes;
}


// =============================================================================
// Transform Implementation
// =============================================================================

constexpr Matrix4 Transform::ToMatrix() const noexcept {
    const Matrix4 scaleMatrix = Scale(scale);
    const Matrix4 rotationMatrix = Math::ToMatrix(rotation);
    const Matrix4 translationMatrix = Translation(position);

    return translationMatrix * rotationMatrix * scaleMatrix;
}

constexpr Transform Transform::Inverse() const noexcept {
    const Quaternion invRotation = Conjugate(rotation);
    const Vector3 invScale(1.0f / scale.x, 1.0f / scale.y, 1.0f / scale.z);
    const Vector3 invPosition = invRotation * (-position * invScale);

    return Transform(invPosition, invRotation, invScale);
}

constexpr Vector3 Transform::TransformPoint(const Vector3& point) const noexcept {
    return position + rotation * (point * scale);
}

constexpr Vector3 Transform::TransformVector(const Vector3& vector) const noexcept {
    return rotation * (vector * scale);
}

constexpr Vector3 Transform::InverseTransformPoint(const Vector3& point) const noexcept {
    const Quaternion invRotation = Conjugate(rotation);
    const Vector3 invScale(1.0f / scale.x, 1.0f / scale.y, 1.0f / scale.z);
    return (invRotation * (point - position)) * invScale;
}

constexpr Vector3 Transform::InverseTransformVector(const Vector3& vector) const noexcept {
    const Quaternion invRotation = Conjugate(rotation);
    const Vector3 invScale(1.0f / scale.x, 1.0f / scale.y, 1.0f / scale.z);
    return (invRotation * vector) * invScale;
}

constexpr Transform Transform::operator*(const Transform& other) const noexcept {
    const Vector3 newPosition = TransformPoint(other.position);
    const Quaternion newRotation = rotation * other.rotation;
    const Vector3 newScale = scale * other.scale;

    return Transform(newPosition, newRotation, newScale);
}

constexpr Transform& Transform::operator*=(const Transform& other) noexcept {
    *this = *this * other;
    return *this;
}

// =============================================================================
// Transform2D Implementation
// =============================================================================

constexpr Matrix3 Transform2D::ToMatrix() const noexcept {
    const float cos_r = std::cos(rotation);
    const float sin_r = std::sin(rotation);

    return Matrix3(
        scale.x * cos_r, -scale.x * sin_r, position.x,
        scale.y * sin_r, scale.y * cos_r, position.y,
        0.0f, 0.0f, 1.0f
    );
}

constexpr Transform2D Transform2D::Inverse() const noexcept {
    const float invRotation = -rotation;
    const Vector2 invScale(1.0f / scale.x, 1.0f / scale.y);

    const float cos_r = std::cos(invRotation);
    const float sin_r = std::sin(invRotation);

    const Vector2 invPosition = Vector2(
        -position.x * cos_r * invScale.x + position.y * sin_r * invScale.x,
        -position.x * sin_r * invScale.y - position.y * cos_r * invScale.y
    );

    return Transform2D(invPosition, invRotation, invScale);
}

constexpr Vector2 Transform2D::TransformPoint(const Vector2& point) const noexcept {
    const float cos_r = std::cos(rotation);
    const float sin_r = std::sin(rotation);

    const Vector2 scaledPoint = point * scale;
    return Vector2(
        scaledPoint.x * cos_r - scaledPoint.y * sin_r + position.x,
        scaledPoint.x * sin_r + scaledPoint.y * cos_r + position.y
    );
}

constexpr Vector2 Transform2D::TransformVector(const Vector2& vector) const noexcept {
    const float cos_r = std::cos(rotation);
    const float sin_r = std::sin(rotation);

    const Vector2 scaledVector = vector * scale;
    return Vector2(
        scaledVector.x * cos_r - scaledVector.y * sin_r,
        scaledVector.x * sin_r + scaledVector.y * cos_r
    );
}

constexpr Transform2D Transform2D::operator*(const Transform2D& other) const noexcept {
    const Vector2 newPosition = TransformPoint(other.position);
    const float newRotation = rotation + other.rotation;
    const Vector2 newScale = scale * other.scale;

    return Transform2D(newPosition, newRotation, newScale);
}

constexpr Transform2D& Transform2D::operator*=(const Transform2D& other) noexcept {
    *this = *this * other;
    return *this;
}

// =============================================================================
// Color Operations Implementation
// =============================================================================

constexpr Color3 Color3::operator+(const Color3& other) const noexcept {
    return Color3(r + other.r, g + other.g, b + other.b);
}

constexpr Color3 Color3::operator-(const Color3& other) const noexcept {
    return Color3(r - other.r, g - other.g, b - other.b);
}

constexpr Color3 Color3::operator*(const Color3& other) const noexcept {
    return Color3(r * other.r, g * other.g, b * other.b);
}

constexpr Color3 Color3::operator*(float scalar) const noexcept {
    return Color3(r * scalar, g * scalar, b * scalar);
}

constexpr Color4 Color4::operator+(const Color4& other) const noexcept {
    return Color4(r + other.r, g + other.g, b + other.b, a + other.a);
}

constexpr Color4 Color4::operator-(const Color4& other) const noexcept {
    return Color4(r - other.r, g - other.g, b - other.b, a - other.a);
}

constexpr Color4 Color4::operator*(const Color4& other) const noexcept {
    return Color4(r * other.r, g * other.g, b * other.b, a * other.a);
}

constexpr Color4 Color4::operator*(float scalar) const noexcept {
    return Color4(r * scalar, g * scalar, b * scalar, a * scalar);
}

// =============================================================================
// Advanced Transform Utilities
// =============================================================================

namespace Math {
    // Transform interpolation
    Transform LerpTransform(const Transform& a, const Transform& b, float t) noexcept {
        return Transform(
            Lerp(a.position, b.position, t),
            Slerp(a.rotation, b.rotation, t),
            Lerp(a.scale, b.scale, t)
        );
    }

    Transform2D LerpTransform2D(const Transform2D& a, const Transform2D& b, float t) noexcept {
        return Transform2D(
            Lerp(a.position, b.position, t),
            Lerp(a.rotation, b.rotation, t),
            Lerp(a.scale, b.scale, t)
        );
    }

    // Transform composition helpers
    Transform CreateTransform(const Vector3& position, const Vector3& eulerAngles, const Vector3& scale) noexcept {
        return Transform(position, FromEulerAngles(eulerAngles), scale);
    }

    Transform CreateLookAtTransform(const Vector3& position, const Vector3& target, const Vector3& up, const Vector3& scale) noexcept {
        const Vector3 forward = Normalize(target - position);
        const Quaternion rotation = LookRotation(forward, up);
        return Transform(position, rotation, scale);
    }

    // Transform hierarchy operations
    Transform CombineTransforms(const Transform& parent, const Transform& child) noexcept {
        return parent * child;
    }

    Transform GetRelativeTransform(const Transform& from, const Transform& to) noexcept {
        return from.Inverse() * to;
    }

    // Transform validation
    bool IsValidTransform(const Transform& transform) noexcept {
        return IsFinite(transform.position.x) && IsFinite(transform.position.y) && IsFinite(transform.position.z) &&
            IsFinite(transform.rotation.x) && IsFinite(transform.rotation.y) && IsFinite(transform.rotation.z) && IsFinite(transform.rotation.w) &&
            IsFinite(transform.scale.x) && IsFinite(transform.scale.y) && IsFinite(transform.scale.z) &&
            !IsNearlyZero(transform.scale.x) && !IsNearlyZero(transform.scale.y) && !IsNearlyZero(transform.scale.z);
    }

    bool IsValidTransform2D(const Transform2D& transform) noexcept {
        return IsFinite(transform.position.x) && IsFinite(transform.position.y) &&
            IsFinite(transform.rotation) &&
            IsFinite(transform.scale.x) && IsFinite(transform.scale.y) &&
            !IsNearlyZero(transform.scale.x) && !IsNearlyZero(transform.scale.y);
    }

    // Transform normalization
    Transform NormalizeTransform(const Transform& transform) noexcept {
        Transform result = transform;
        result.rotation = Normalize(result.rotation);

        // Ensure non-zero scale
        if (IsNearlyZero(result.scale.x)) result.scale.x = 1.0f;
        if (IsNearlyZero(result.scale.y)) result.scale.y = 1.0f;
        if (IsNearlyZero(result.scale.z)) result.scale.z = 1.0f;

        return result;
    }

    Transform2D NormalizeTransform2D(const Transform2D& transform) noexcept {
        Transform2D result = transform;

        // Wrap rotation to [-PI, PI]
        result.rotation = WrapAngle(result.rotation);

        // Ensure non-zero scale
        if (IsNearlyZero(result.scale.x)) result.scale.x = 1.0f;
        if (IsNearlyZero(result.scale.y)) result.scale.y = 1.0f;

        return result;
    }

    // Transform bounding calculations
    AABB TransformAABB(const AABB& aabb, const Transform& transform) noexcept {
        const Matrix4 matrix = transform.ToMatrix();

        // Transform all 8 corners of the AABB
        const Vector3 corners[8] = {
            Vector3(aabb.min.x, aabb.min.y, aabb.min.z),
            Vector3(aabb.max.x, aabb.min.y, aabb.min.z),
            Vector3(aabb.min.x, aabb.max.y, aabb.min.z),
            Vector3(aabb.max.x, aabb.max.y, aabb.min.z),
            Vector3(aabb.min.x, aabb.min.y, aabb.max.z),
            Vector3(aabb.max.x, aabb.min.y, aabb.max.z),
            Vector3(aabb.min.x, aabb.max.y, aabb.max.z),
            Vector3(aabb.max.x, aabb.max.y, aabb.max.z)
        };

        AABB result;
        bool first = true;

        for (const auto& corner : corners) {
            const Vector3 transformedCorner = matrix.TransformPoint(corner);

            if (first) {
                result.min = result.max = transformedCorner;
                first = false;
            }
            else {
                result.Expand(transformedCorner);
            }
        }

        return result;
    }

    Sphere TransformSphere(const Sphere& sphere, const Transform& transform) noexcept {
        const Vector3 transformedCenter = transform.TransformPoint(sphere.center);

        // Calculate the maximum scale component to determine new radius
        const float maxScale = Max(Max(Abs(transform.scale.x), Abs(transform.scale.y)), Abs(transform.scale.z));
        const float transformedRadius = sphere.radius * maxScale;

        return Sphere(transformedCenter, transformedRadius);
    }

    // Transform comparison
    bool AreTransformsEqual(const Transform& a, const Transform& b, float epsilon = EPSILON) noexcept {
        return IsNearlyEqual(Distance(a.position, b.position), 0.0f, epsilon) &&
            IsNearlyEqual(Angle(a.rotation, b.rotation), 0.0f, epsilon) &&
            IsNearlyEqual(Distance(a.scale, b.scale), 0.0f, epsilon);
    }

    bool AreTransforms2DEqual(const Transform2D& a, const Transform2D& b, float epsilon = EPSILON) noexcept {
        return IsNearlyEqual(Distance(a.position, b.position), 0.0f, epsilon) &&
            IsNearlyEqual(AngleDifference(a.rotation, b.rotation), 0.0f, epsilon) &&
            IsNearlyEqual(Distance(a.scale, b.scale), 0.0f, epsilon);
    }
}

// =============================================================================
// Global Transform Operators
// =============================================================================

constexpr Color3 operator*(float scalar, const Color3& color) noexcept {
    return color * scalar;
}

constexpr Color4 operator*(float scalar, const Color4& color) noexcept {
    return color * scalar;
}


// =============================================================================
// Random Class Implementation
// =============================================================================

Random::Random() noexcept : state_(1) {}

Random::Random(uint32_t seed) noexcept : state_(seed == 0 ? 1 : seed) {}

void Random::SetSeed(uint32_t seed) noexcept {
    state_ = (seed == 0) ? 1 : seed;
}

uint32_t Random::NextUInt() noexcept {
    // Xorshift32 algorithm
    state_ ^= state_ << 13;
    state_ ^= state_ >> 17;
    state_ ^= state_ << 5;
    return state_;
}

int32_t Random::NextInt() noexcept {
    return static_cast<int32_t>(NextUInt());
}

int32_t Random::NextInt(int32_t min, int32_t max) noexcept {
    if (min >= max) return min;
    const uint32_t range = static_cast<uint32_t>(max - min);
    return min + static_cast<int32_t>(NextUInt() % range);
}

float Random::NextFloat() noexcept {
    // Convert to [0, 1) range
    return static_cast<float>(NextUInt()) / static_cast<float>(0xFFFFFFFFU);
}

float Random::NextFloat(float min, float max) noexcept {
    return min + NextFloat() * (max - min);
}

double Random::NextDouble() noexcept {
    // Use two 32-bit values for better precision
    const uint32_t high = NextUInt() >> 5; // Upper 27 bits
    const uint32_t low = NextUInt() >> 6;  // Upper 26 bits
    return static_cast<double>(high * 67108864.0 + low) / 9007199254740992.0;
}

double Random::NextDouble(double min, double max) noexcept {
    return min + NextDouble() * (max - min);
}

bool Random::NextBool() noexcept {
    return (NextUInt() & 1) != 0;
}

Vector2 Random::NextVector2() noexcept {
    return Vector2(NextFloat(), NextFloat());
}

Vector2 Random::NextVector2(const Vector2& min, const Vector2& max) noexcept {
    return Vector2(
        NextFloat(min.x, max.x),
        NextFloat(min.y, max.y)
    );
}

Vector3 Random::NextVector3() noexcept {
    return Vector3(NextFloat(), NextFloat(), NextFloat());
}

Vector3 Random::NextVector3(const Vector3& min, const Vector3& max) noexcept {
    return Vector3(
        NextFloat(min.x, max.x),
        NextFloat(min.y, max.y),
        NextFloat(min.z, max.z)
    );
}

Vector3 Random::NextUnitVector3() noexcept {
    // Marsaglia's method for uniform distribution on sphere
    float x1, x2, lengthSquared;
    do {
        x1 = NextFloat(-1.0f, 1.0f);
        x2 = NextFloat(-1.0f, 1.0f);
        lengthSquared = x1 * x1 + x2 * x2;
    } while (lengthSquared >= 1.0f);

    const float factor = 2.0f * std::sqrt(1.0f - lengthSquared);
    return Vector3(
        x1 * factor,
        x2 * factor,
        1.0f - 2.0f * lengthSquared
    );
}

Vector2 Random::NextPointInCircle() noexcept {
    // Rejection sampling
    Vector2 point;
    do {
        point = Vector2(NextFloat(-1.0f, 1.0f), NextFloat(-1.0f, 1.0f));
    } while (LengthSquared(point) > 1.0f);
    return point;
}

Vector3 Random::NextPointInSphere() noexcept {
    // Rejection sampling
    Vector3 point;
    do {
        point = Vector3(NextFloat(-1.0f, 1.0f), NextFloat(-1.0f, 1.0f), NextFloat(-1.0f, 1.0f));
    } while (LengthSquared(point) > 1.0f);
    return point;
}

Vector2 Random::NextPointOnCircle() noexcept {
    const float angle = NextFloat(0.0f, TWO_PI);
    return Vector2(std::cos(angle), std::sin(angle));
}

Vector3 Random::NextPointOnSphere() noexcept {
    return NextUnitVector3();
}

Quaternion Random::NextRotation() noexcept {
    // Generate uniform random quaternion using Shepperd's method
    const float u1 = NextFloat();
    const float u2 = NextFloat();
    const float u3 = NextFloat();

    const float sqrt1MinusU1 = std::sqrt(1.0f - u1);
    const float sqrtU1 = std::sqrt(u1);
    const float theta1 = TWO_PI * u2;
    const float theta2 = TWO_PI * u3;

    return Quaternion(
        sqrt1MinusU1 * std::sin(theta1),
        sqrt1MinusU1 * std::cos(theta1),
        sqrtU1 * std::sin(theta2),
        sqrtU1 * std::cos(theta2)
    );
}

Color3 Random::NextColor3() noexcept {
    return Color3(NextFloat(), NextFloat(), NextFloat());
}

Color4 Random::NextColor4() noexcept {
    return Color4(NextFloat(), NextFloat(), NextFloat(), NextFloat());
}

// =============================================================================
// Global Random Instance and Functions
// =============================================================================

static Random g_globalRandom(static_cast<uint32_t>(std::time(nullptr)));

Random& Math::GlobalRandom() noexcept {
    return g_globalRandom;
}


// =============================================================================
// Noise Implementation - Private Helpers
// =============================================================================

namespace {
    // Permutation table for Perlin noise
    constexpr int PERM_SIZE = 256;
    constexpr int PERM[PERM_SIZE * 2] = {
        151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
        190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,134,139,48,27,166,
        77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,200,196,
        135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
        223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,228,
        251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
        138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
        // Duplicate the permutation table
        151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
        190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,134,139,48,27,166,
        77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,200,196,
        135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
        223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,228,
        251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
        138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
    };

    // Fade function for smooth interpolation
    constexpr float Fade(float t) noexcept {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    // Linear interpolation
    constexpr float NoLerp(float a, float b, float t) noexcept {
        return a + t * (b - a);
    }

    // Gradient function for Perlin noise
    constexpr float Grad(int hash, float x, float y, float z) noexcept {
        const int h = hash & 15;
        const float u = h < 8 ? x : y;
        const float v = h < 4 ? y : h == 12 || h == 14 ? x : z;
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }

    constexpr float Grad(int hash, float x, float y) noexcept {
        const int h = hash & 7;
        const float u = h < 4 ? x : y;
        const float v = h < 4 ? y : x;
        return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
    }

    constexpr float Grad(int hash, float x) noexcept {
        return (hash & 1) ? -x : x;
    }

    // Simplex noise helpers
    constexpr float sqrt3 = 1.732050807569f;
    constexpr float G2 = (3.0f - sqrt3) / 6.0f;
    constexpr float F2 = 0.5f * (sqrt3 - 1.0f);
    constexpr float G3 = 1.0f / 6.0f;
    constexpr float F3 = 1.0f / 3.0f;

    // 2D simplex gradient vectors
    constexpr Vector2 GRAD2[8] = {
        Vector2(1, 1), Vector2(-1, 1), Vector2(1, -1), Vector2(-1, -1),
        Vector2(1, 0), Vector2(-1, 0), Vector2(0, 1), Vector2(0, -1)
    };

    // 3D simplex gradient vectors
    constexpr Vector3 GRAD3[12] = {
        Vector3(1, 1, 0), Vector3(-1, 1, 0), Vector3(1, -1, 0), Vector3(-1, -1, 0),
        Vector3(1, 0, 1), Vector3(-1, 0, 1), Vector3(1, 0, -1), Vector3(-1, 0, -1),
        Vector3(0, 1, 1), Vector3(0, -1, 1), Vector3(0, 1, -1), Vector3(0, -1, -1)
    };

    float SimplexGrad2(int hash, float x, float y) noexcept {
        const Vector2& grad = GRAD2[hash & 7];
        return grad.x * x + grad.y * y;
    }

    float SimplexGrad3(int hash, float x, float y, float z) noexcept {
        const Vector3& grad = GRAD3[hash % 12];
        return grad.x * x + grad.y * y + grad.z * z;
    }
}

// =============================================================================
// 1D Noise Functions
// =============================================================================

float Math::PerlinNoise1D(float x) noexcept {
    const int xi = static_cast<int>(std::floor(x)) & 255;
    x -= std::floor(x);

    const float u = Fade(x);

    const int a = PERM[xi];
    const int b = PERM[xi + 1];

    return NoLerp(Grad(a, x), Grad(b, x - 1.0f), u);
}

float Math::SimplexNoise1D(float x) noexcept {
    const int i0 = static_cast<int>(std::floor(x));
    const int i1 = i0 + 1;
    const float x0 = x - i0;
    const float x1 = x0 - 1.0f;

    float n0 = 0.0f, n1 = 0.0f;

    float t0 = 1.0f - x0 * x0;
    if (t0 > 0.0f) {
        t0 *= t0;
        n0 = t0 * t0 * Grad(PERM[i0 & 255], x0);
    }

    float t1 = 1.0f - x1 * x1;
    if (t1 > 0.0f) {
        t1 *= t1;
        n1 = t1 * t1 * Grad(PERM[i1 & 255], x1);
    }

    return 0.395f * (n0 + n1);
}

// =============================================================================
// 2D Noise Functions
// =============================================================================

float Math::PerlinNoise2D(float x, float y) noexcept {
    const int xi = static_cast<int>(std::floor(x)) & 255;
    const int yi = static_cast<int>(std::floor(y)) & 255;

    x -= std::floor(x);
    y -= std::floor(y);

    const float u = Fade(x);
    const float v = Fade(y);

    const int a = PERM[xi] + yi;
    const int aa = PERM[a];
    const int ab = PERM[a + 1];
    const int b = PERM[xi + 1] + yi;
    const int ba = PERM[b];
    const int bb = PERM[b + 1];

    return NoLerp(
        NoLerp(Grad(PERM[aa], x, y), Grad(PERM[ba], x - 1.0f, y), u),
        NoLerp(Grad(PERM[ab], x, y - 1.0f), Grad(PERM[bb], x - 1.0f, y - 1.0f), u),
        v
    );
}

float Math::SimplexNoise2D(float x, float y) noexcept {
    const float s = (x + y) * F2;
    const int i = static_cast<int>(std::floor(x + s));
    const int j = static_cast<int>(std::floor(y + s));

    const float t = static_cast<float>(i + j) * G2;
    const float X0 = i - t;
    const float Y0 = j - t;
    const float x0 = x - X0;
    const float y0 = y - Y0;

    int i1, j1;
    if (x0 > y0) {
        i1 = 1; j1 = 0;
    }
    else {
        i1 = 0; j1 = 1;
    }

    const float x1 = x0 - i1 + G2;
    const float y1 = y0 - j1 + G2;
    const float x2 = x0 - 1.0f + 2.0f * G2;
    const float y2 = y0 - 1.0f + 2.0f * G2;

    const int ii = i & 255;
    const int jj = j & 255;
    const int gi0 = PERM[ii + PERM[jj]] & 7;
    const int gi1 = PERM[ii + i1 + PERM[jj + j1]] & 7;
    const int gi2 = PERM[ii + 1 + PERM[jj + 1]] & 7;

    float n0 = 0.0f, n1 = 0.0f, n2 = 0.0f;

    float t0 = 0.5f - x0 * x0 - y0 * y0;
    if (t0 > 0.0f) {
        t0 *= t0;
        n0 = t0 * t0 * SimplexGrad2(gi0, x0, y0);
    }

    float t1 = 0.5f - x1 * x1 - y1 * y1;
    if (t1 > 0.0f) {
        t1 *= t1;
        n1 = t1 * t1 * SimplexGrad2(gi1, x1, y1);
    }

    float t2 = 0.5f - x2 * x2 - y2 * y2;
    if (t2 > 0.0f) {
        t2 *= t2;
        n2 = t2 * t2 * SimplexGrad2(gi2, x2, y2);
    }

    return 70.0f * (n0 + n1 + n2);
}

float Math::RidgedNoise2D(float x, float y) noexcept {
    float noise = PerlinNoise2D(x, y);
    return 1.0f - 2.0f * Abs(noise);
}

float Math::CellularNoise2D(float x, float y) noexcept {
    const int xi = static_cast<int>(std::floor(x));
    const int yi = static_cast<int>(std::floor(y));

    float minDistance = INFINITY_F;

    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            const int cellX = xi + dx;
            const int cellY = yi + dy;

            // Generate pseudo-random point in cell
            const uint32_t hash = Hash(static_cast<uint32_t>(cellX)) ^ Hash(static_cast<uint32_t>(cellY));
            Random cellRandom(hash);

            const float pointX = cellX + cellRandom.NextFloat();
            const float pointY = cellY + cellRandom.NextFloat();

            const float distance = Distance(Vector2(x, y), Vector2(pointX, pointY));
            minDistance = Min(minDistance, distance);
        }
    }

    return minDistance;
}

// =============================================================================
// 3D Noise Functions
// =============================================================================

float Math::PerlinNoise3D(float x, float y, float z) noexcept {
    const int xi = static_cast<int>(std::floor(x)) & 255;
    const int yi = static_cast<int>(std::floor(y)) & 255;
    const int zi = static_cast<int>(std::floor(z)) & 255;

    x -= std::floor(x);
    y -= std::floor(y);
    z -= std::floor(z);

    const float u = Fade(x);
    const float v = Fade(y);
    const float w = Fade(z);

    const int a = PERM[xi] + yi;
    const int aa = PERM[a] + zi;
    const int ab = PERM[a + 1] + zi;
    const int b = PERM[xi + 1] + yi;
    const int ba = PERM[b] + zi;
    const int bb = PERM[b + 1] + zi;

    return NoLerp(
        NoLerp(
            NoLerp(Grad(PERM[aa], x, y, z), Grad(PERM[ba], x - 1, y, z), u),
            NoLerp(Grad(PERM[ab], x, y - 1, z), Grad(PERM[bb], x - 1, y - 1, z), u),
            v
        ),
        NoLerp(
            NoLerp(Grad(PERM[aa + 1], x, y, z - 1), Grad(PERM[ba + 1], x - 1, y, z - 1), u),
            NoLerp(Grad(PERM[ab + 1], x, y - 1, z - 1), Grad(PERM[bb + 1], x - 1, y - 1, z - 1), u),
            v
        ),
        w
    );
}

float Math::SimplexNoise3D(float x, float y, float z) noexcept {
    const float s = (x + y + z) * F3;
    const int i = static_cast<int>(std::floor(x + s));
    const int j = static_cast<int>(std::floor(y + s));
    const int k = static_cast<int>(std::floor(z + s));

    const float t = static_cast<float>(i + j + k) * G3;
    const float X0 = i - t;
    const float Y0 = j - t;
    const float Z0 = k - t;
    const float x0 = x - X0;
    const float y0 = y - Y0;
    const float z0 = z - Z0;

    int i1, j1, k1, i2, j2, k2;

    if (x0 >= y0) {
        if (y0 >= z0) {
            i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 1; k2 = 0;
        }
        else if (x0 >= z0) {
            i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 0; k2 = 1;
        }
        else {
            i1 = 0; j1 = 0; k1 = 1; i2 = 1; j2 = 0; k2 = 1;
        }
    }
    else {
        if (y0 < z0) {
            i1 = 0; j1 = 0; k1 = 1; i2 = 0; j2 = 1; k2 = 1;
        }
        else if (x0 < z0) {
            i1 = 0; j1 = 1; k1 = 0; i2 = 0; j2 = 1; k2 = 1;
        }
        else {
            i1 = 0; j1 = 1; k1 = 0; i2 = 1; j2 = 1; k2 = 0;
        }
    }

    const float x1 = x0 - i1 + G3;
    const float y1 = y0 - j1 + G3;
    const float z1 = z0 - k1 + G3;
    const float x2 = x0 - i2 + 2.0f * G3;
    const float y2 = y0 - j2 + 2.0f * G3;
    const float z2 = z0 - k2 + 2.0f * G3;
    const float x3 = x0 - 1.0f + 3.0f * G3;
    const float y3 = y0 - 1.0f + 3.0f * G3;
    const float z3 = z0 - 1.0f + 3.0f * G3;

    const int ii = i & 255;
    const int jj = j & 255;
    const int kk = k & 255;
    const int gi0 = PERM[ii + PERM[jj + PERM[kk]]] % 12;
    const int gi1 = PERM[ii + i1 + PERM[jj + j1 + PERM[kk + k1]]] % 12;
    const int gi2 = PERM[ii + i2 + PERM[jj + j2 + PERM[kk + k2]]] % 12;
    const int gi3 = PERM[ii + 1 + PERM[jj + 1 + PERM[kk + 1]]] % 12;

    float n0 = 0.0f, n1 = 0.0f, n2 = 0.0f, n3 = 0.0f;

    float t0 = 0.6f - x0 * x0 - y0 * y0 - z0 * z0;
    if (t0 > 0.0f) {
        t0 *= t0;
        n0 = t0 * t0 * SimplexGrad3(gi0, x0, y0, z0);
    }

    float t1 = 0.6f - x1 * x1 - y1 * y1 - z1 * z1;
    if (t1 > 0.0f) {
        t1 *= t1;
        n1 = t1 * t1 * SimplexGrad3(gi1, x1, y1, z1);
    }

    float t2 = 0.6f - x2 * x2 - y2 * y2 - z2 * z2;
    if (t2 > 0.0f) {
        t2 *= t2;
        n2 = t2 * t2 * SimplexGrad3(gi2, x2, y2, z2);
    }

    float t3 = 0.6f - x3 * x3 - y3 * y3 - z3 * z3;
    if (t3 > 0.0f) {
        t3 *= t3;
        n3 = t3 * t3 * SimplexGrad3(gi3, x3, y3, z3);
    }

    return 32.0f * (n0 + n1 + n2 + n3);
}

float Math::RidgedNoise3D(float x, float y, float z) noexcept {
    float noise = PerlinNoise3D(x, y, z);
    return 1.0f - 2.0f * Abs(noise);
}

float Math::CellularNoise3D(float x, float y, float z) noexcept {
    const int xi = static_cast<int>(std::floor(x));
    const int yi = static_cast<int>(std::floor(y));
    const int zi = static_cast<int>(std::floor(z));

    float minDistance = INFINITY_F;

    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dz = -1; dz <= 1; ++dz) {
                const int cellX = xi + dx;
                const int cellY = yi + dy;
                const int cellZ = zi + dz;

                // Generate pseudo-random point in cell
                const uint32_t hash = Hash(static_cast<uint32_t>(cellX)) ^
                    Hash(static_cast<uint32_t>(cellY)) ^
                    Hash(static_cast<uint32_t>(cellZ));
                Random cellRandom(hash);

                const float pointX = cellX + cellRandom.NextFloat();
                const float pointY = cellY + cellRandom.NextFloat();
                const float pointZ = cellZ + cellRandom.NextFloat();

                const float distance = Distance(Vector3(x, y, z), Vector3(pointX, pointY, pointZ));
                minDistance = Min(minDistance, distance);
            }
        }
    }

    return minDistance;
}

// =============================================================================
// Fractal Noise Functions
// =============================================================================

float Math::FractalNoise2D(float x, float y, int octaves, float frequency,
    float amplitude, float lacunarity, float gain) noexcept {
    float total = 0.0f;
    float maxValue = 0.0f;
    float currentAmplitude = amplitude;
    float currentFrequency = frequency;

    for (int i = 0; i < octaves; ++i) {
        total += PerlinNoise2D(x * currentFrequency, y * currentFrequency) * currentAmplitude;
        maxValue += currentAmplitude;
        currentAmplitude *= gain;
        currentFrequency *= lacunarity;
    }

    return total / maxValue;
}

float Math::FractalNoise3D(float x, float y, float z, int octaves, float frequency,
    float amplitude, float lacunarity, float gain) noexcept {
    float total = 0.0f;
    float maxValue = 0.0f;
    float currentAmplitude = amplitude;
    float currentFrequency = frequency;

    for (int i = 0; i < octaves; ++i) {
        total += PerlinNoise3D(x * currentFrequency, y * currentFrequency, z * currentFrequency) * currentAmplitude;
        maxValue += currentAmplitude;
        currentAmplitude *= gain;
        currentFrequency *= lacunarity;
    }

    return total / maxValue;
}

// =============================================================================
// Configurable Noise Generation
// =============================================================================

float Math::GenerateNoise2D(float x, float y, const NoiseConfig& config) noexcept {
    // Apply seed offset
    x += config.seed * 12345.6789f;
    y += config.seed * 98765.4321f;

    float result = 0.0f;

    switch (config.type) {
    case NoiseType::Perlin:
        result = FractalNoise2D(x, y, config.octaves, config.frequency,
            config.amplitude, config.lacunarity, config.gain);
        break;
    case NoiseType::Simplex:
        result = 0.0f;
        {
            float total = 0.0f;
            float maxValue = 0.0f;
            float currentAmplitude = config.amplitude;
            float currentFrequency = config.frequency;

            for (int i = 0; i < config.octaves; ++i) {
                total += SimplexNoise2D(x * currentFrequency, y * currentFrequency) * currentAmplitude;
                maxValue += currentAmplitude;
                currentAmplitude *= config.gain;
                currentFrequency *= config.lacunarity;
            }
            result = total / maxValue;
        }
        break;
    case NoiseType::Ridged:
        result = 0.0f;
        {
            float total = 0.0f;
            float maxValue = 0.0f;
            float currentAmplitude = config.amplitude;
            float currentFrequency = config.frequency;

            for (int i = 0; i < config.octaves; ++i) {
                total += RidgedNoise2D(x * currentFrequency, y * currentFrequency) * currentAmplitude;
                maxValue += currentAmplitude;
                currentAmplitude *= config.gain;
                currentFrequency *= config.lacunarity;
            }
            result = total / maxValue;
        }
        break;
    case NoiseType::Cellular:
        result = CellularNoise2D(x * config.frequency, y * config.frequency) * config.amplitude;
        break;
    default:
        result = FractalNoise2D(x, y, config.octaves, config.frequency,
            config.amplitude, config.lacunarity, config.gain);
        break;
    }

    return result;
}

float Math::GenerateNoise3D(float x, float y, float z, const NoiseConfig& config) noexcept {
    // Apply seed offset
    x += config.seed * 12345.6789f;
    y += config.seed * 98765.4321f;
    z += config.seed * 54321.9876f;

    float result = 0.0f;

    switch (config.type) {
    case NoiseType::Perlin:
        result = FractalNoise3D(x, y, z, config.octaves, config.frequency,
            config.amplitude, config.lacunarity, config.gain);
        break;
    case NoiseType::Simplex:
        result = 0.0f;
        {
            float total = 0.0f;
            float maxValue = 0.0f;
            float currentAmplitude = config.amplitude;
            float currentFrequency = config.frequency;

            for (int i = 0; i < config.octaves; ++i) {
                total += SimplexNoise3D(x * currentFrequency, y * currentFrequency, z * currentFrequency) * currentAmplitude;
                maxValue += currentAmplitude;
                currentAmplitude *= config.gain;
                currentFrequency *= config.lacunarity;
            }
            result = total / maxValue;
        }
        break;
    case NoiseType::Ridged:
        result = 0.0f;
        {
            float total = 0.0f;
            float maxValue = 0.0f;
            float currentAmplitude = config.amplitude;
            float currentFrequency = config.frequency;

            for (int i = 0; i < config.octaves; ++i) {
                total += RidgedNoise3D(x * currentFrequency, y * currentFrequency, z * currentFrequency) * currentAmplitude;
                maxValue += currentAmplitude;
                currentAmplitude *= config.gain;
                currentFrequency *= config.lacunarity;
            }
            result = total / maxValue;
        }
        break;
    case NoiseType::Cellular:
        result = CellularNoise3D(x * config.frequency, y * config.frequency, z * config.frequency) * config.amplitude;
        break;
    default:
        result = FractalNoise3D(x, y, z, config.octaves, config.frequency,
            config.amplitude, config.lacunarity, config.gain);
        break;
    }

    return result;
}



// =============================================================================
// Easing Functions Implementation
// =============================================================================

float Math::EaseInSine(float t) noexcept {
    return 1.0f - std::cos(t * HALF_PI);
}

float Math::EaseOutSine(float t) noexcept {
    return std::sin(t * HALF_PI);
}

float Math::EaseInOutSine(float t) noexcept {
    return -(std::cos(PI * t) - 1.0f) * 0.5f;
}

float Math::EaseInQuad(float t) noexcept {
    return t * t;
}

float Math::EaseOutQuad(float t) noexcept {
    return 1.0f - (1.0f - t) * (1.0f - t);
}

float Math::EaseInOutQuad(float t) noexcept {
    return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) * 0.5f;
}

float Math::EaseInCubic(float t) noexcept {
    return t * t * t;
}

float Math::EaseOutCubic(float t) noexcept {
    return 1.0f - std::pow(1.0f - t, 3.0f);
}

float Math::EaseInOutCubic(float t) noexcept {
    return t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) * 0.5f;
}

float Math::EaseInQuart(float t) noexcept {
    return t * t * t * t;
}

float Math::EaseOutQuart(float t) noexcept {
    return 1.0f - std::pow(1.0f - t, 4.0f);
}

float Math::EaseInOutQuart(float t) noexcept {
    return t < 0.5f ? 8.0f * t * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 4.0f) * 0.5f;
}

float Math::EaseInExpo(float t) noexcept {
    return IsNearlyZero(t) ? 0.0f : std::pow(2.0f, 10.0f * (t - 1.0f));
}

float Math::EaseOutExpo(float t) noexcept {
    return IsNearlyEqual(t, 1.0f) ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
}

float Math::EaseInOutExpo(float t) noexcept {
    if (IsNearlyZero(t)) return 0.0f;
    if (IsNearlyEqual(t, 1.0f)) return 1.0f;

    return t < 0.5f
        ? std::pow(2.0f, 20.0f * t - 10.0f) * 0.5f
        : (2.0f - std::pow(2.0f, -20.0f * t + 10.0f)) * 0.5f;
}

float Math::EaseInBack(float t) noexcept {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;

    return c3 * t * t * t - c1 * t * t;
}

float Math::EaseOutBack(float t) noexcept {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;

    return 1.0f + c3 * std::pow(t - 1.0f, 3.0f) + c1 * std::pow(t - 1.0f, 2.0f);
}

float Math::EaseInOutBack(float t) noexcept {
    const float c1 = 1.70158f;
    const float c2 = c1 * 1.525f;

    return t < 0.5f
        ? (std::pow(2.0f * t, 2.0f) * ((c2 + 1.0f) * 2.0f * t - c2)) * 0.5f
        : (std::pow(2.0f * t - 2.0f, 2.0f) * ((c2 + 1.0f) * (t * 2.0f - 2.0f) + c2) + 2.0f) * 0.5f;
}

float Math::EaseInElastic(float t) noexcept {
    const float c4 = TWO_PI / 3.0f;

    if (IsNearlyZero(t)) return 0.0f;
    if (IsNearlyEqual(t, 1.0f)) return 1.0f;

    return -std::pow(2.0f, 10.0f * (t - 1.0f)) * std::sin((t - 1.0f) * c4 - c4);
}

float Math::EaseOutElastic(float t) noexcept {
    const float c4 = TWO_PI / 3.0f;

    if (IsNearlyZero(t)) return 0.0f;
    if (IsNearlyEqual(t, 1.0f)) return 1.0f;

    return std::pow(2.0f, -10.0f * t) * std::sin(t * c4) + 1.0f;
}

float Math::EaseInOutElastic(float t) noexcept {
    const float c5 = TWO_PI / 4.5f;

    if (IsNearlyZero(t)) return 0.0f;
    if (IsNearlyEqual(t, 1.0f)) return 1.0f;

    return t < 0.5f
        ? -(std::pow(2.0f, 20.0f * t - 10.0f) * std::sin(20.0f * t * c5 - c5)) * 0.5f
        : (std::pow(2.0f, -20.0f * t + 10.0f) * std::sin(20.0f * t * c5 - c5)) * 0.5f + 1.0f;
}

float Math::EaseInBounce(float t) noexcept {
    return 1.0f - EaseOutBounce(1.0f - t);
}

float Math::EaseOutBounce(float t) noexcept {
    const float n1 = 7.5625f;
    const float d1 = 2.75f;

    if (t < 1.0f / d1) {
        return n1 * t * t;
    }
    else if (t < 2.0f / d1) {
        t -= 1.5f / d1;
        return n1 * t * t + 0.75f;
    }
    else if (t < 2.5f / d1) {
        t -= 2.25f / d1;
        return n1 * t * t + 0.9375f;
    }
    else {
        t -= 2.625f / d1;
        return n1 * t * t + 0.984375f;
    }
}

float Math::EaseInOutBounce(float t) noexcept {
    return t < 0.5f
        ? (1.0f - EaseOutBounce(1.0f - 2.0f * t)) * 0.5f
        : (1.0f + EaseOutBounce(2.0f * t - 1.0f)) * 0.5f;
}


// =============================================================================
// Inertia Tensor Calculations
// =============================================================================

Matrix3 Math::SphereInertiaTensor(float mass, float radius) noexcept {
    const float inertia = (2.0f / 5.0f) * mass * radius * radius;
    return Matrix3(
        inertia, 0.0f, 0.0f,
        0.0f, inertia, 0.0f,
        0.0f, 0.0f, inertia
    );
}

Matrix3 Math::BoxInertiaTensor(float mass, const Vector3& dimensions) noexcept {
    const float massDiv12 = mass / 12.0f;
    const float x2 = dimensions.x * dimensions.x;
    const float y2 = dimensions.y * dimensions.y;
    const float z2 = dimensions.z * dimensions.z;

    return Matrix3(
        massDiv12 * (y2 + z2), 0.0f, 0.0f,
        0.0f, massDiv12 * (x2 + z2), 0.0f,
        0.0f, 0.0f, massDiv12 * (x2 + y2)
    );
}

Matrix3 Math::CylinderInertiaTensor(float mass, float radius, float height) noexcept {
    const float r2 = radius * radius;
    const float h2 = height * height;
    const float Ixx = mass * (3.0f * r2 + h2) / 12.0f;
    const float Iyy = mass * r2 / 2.0f;

    return Matrix3(
        Ixx, 0.0f, 0.0f,
        0.0f, Iyy, 0.0f,
        0.0f, 0.0f, Ixx
    );
}

Matrix3 Math::CapsuleInertiaTensor(float mass, float radius, float height) noexcept {
    const float cylinderHeight = height - 2.0f * radius;
    const float cylinderMass = mass * cylinderHeight / height;
    const float hemisphereMass = mass * radius / height;

    // Cylinder component
    const float r2 = radius * radius;
    const float h2 = cylinderHeight * cylinderHeight;
    const float IxxCyl = cylinderMass * (3.0f * r2 + h2) / 12.0f;
    const float IyyCyl = cylinderMass * r2 / 2.0f;

    // Hemisphere components (two hemispheres)
    const float IxxHemi = 2.0f * hemisphereMass * (2.0f * r2 / 5.0f + radius * cylinderHeight / 2.0f + h2 / 4.0f);
    const float IyyHemi = 2.0f * hemisphereMass * 2.0f * r2 / 5.0f;

    return Matrix3(
        IxxCyl + IxxHemi, 0.0f, 0.0f,
        0.0f, IyyCyl + IyyHemi, 0.0f,
        0.0f, 0.0f, IxxCyl + IxxHemi
    );
}

// =============================================================================
// Physics Calculations
// =============================================================================

Vector3 Math::CalculateAngularVelocity(const Quaternion& fromRotation, const Quaternion& toRotation, float deltaTime) noexcept {
    if (IsNearlyZero(deltaTime)) {
        return Vector3::ZERO;
    }

    const Quaternion deltaRotation = toRotation * Conjugate(fromRotation);

    // Convert to axis-angle representation
    const float w = Clamp(deltaRotation.w, -1.0f, 1.0f);
    const float angle = 2.0f * std::acos(Abs(w));

    if (IsNearlyZero(angle)) {
        return Vector3::ZERO;
    }

    const float sinHalfAngle = std::sin(angle * 0.5f);
    Vector3 axis;

    if (IsNearlyZero(sinHalfAngle)) {
        axis = Vector3::UNIT_X; // Arbitrary axis for zero rotation
    }
    else {
        axis = Vector3(deltaRotation.x, deltaRotation.y, deltaRotation.z) / sinHalfAngle;
    }

    // Ensure we take the shorter rotation path
    const float finalAngle = (deltaRotation.w < 0.0f) ? -angle : angle;

    return axis * (finalAngle / deltaTime);
}

Vector3 Math::CalculateCenterOfMass(const std::vector<Vector3>& positions, const std::vector<float>& masses) noexcept {
    if (positions.empty() || masses.empty() || positions.size() != masses.size()) {
        return Vector3::ZERO;
    }

    Vector3 centerOfMass = Vector3::ZERO;
    float totalMass = 0.0f;

    for (size_t i = 0; i < positions.size(); ++i) {
        centerOfMass += positions[i] * masses[i];
        totalMass += masses[i];
    }

    if (IsNearlyZero(totalMass)) {
        return Vector3::ZERO;
    }

    return centerOfMass / totalMass;
}

// =============================================================================
// Collision Detection Helpers
// =============================================================================

bool Math::RayAABBIntersection(const Ray& ray, const AABB& aabb, float& tMin, float& tMax) noexcept {
    const Vector3 invDir = Vector3(
        IsNearlyZero(ray.direction.x) ? INFINITY_F : 1.0f / ray.direction.x,
        IsNearlyZero(ray.direction.y) ? INFINITY_F : 1.0f / ray.direction.y,
        IsNearlyZero(ray.direction.z) ? INFINITY_F : 1.0f / ray.direction.z
    );

    float t1 = (aabb.min.x - ray.origin.x) * invDir.x;
    float t2 = (aabb.max.x - ray.origin.x) * invDir.x;
    float t3 = (aabb.min.y - ray.origin.y) * invDir.y;
    float t4 = (aabb.max.y - ray.origin.y) * invDir.y;
    float t5 = (aabb.min.z - ray.origin.z) * invDir.z;
    float t6 = (aabb.max.z - ray.origin.z) * invDir.z;

    tMin = Max(Max(Min(t1, t2), Min(t3, t4)), Min(t5, t6));
    tMax = Min(Min(Max(t1, t2), Max(t3, t4)), Max(t5, t6));

    // Ray doesn't intersect AABB if tMax < 0 or tMin > tMax
    return tMax >= 0.0f && tMin <= tMax;
}

bool Math::RaySphereIntersection(const Ray& ray, const Sphere& sphere, float& t1, float& t2) noexcept {
    const Vector3 oc = ray.origin - sphere.center;
    const float a = Dot(ray.direction, ray.direction);
    const float b = 2.0f * Dot(oc, ray.direction);
    const float c = Dot(oc, oc) - sphere.radius * sphere.radius;

    const float discriminant = b * b - 4.0f * a * c;

    if (discriminant < 0.0f) {
        return false; // No intersection
    }

    const float sqrtDiscriminant = std::sqrt(discriminant);
    const float invA = 1.0f / (2.0f * a);

    t1 = (-b - sqrtDiscriminant) * invA;
    t2 = (-b + sqrtDiscriminant) * invA;

    return true;
}

bool Math::RayTriangleIntersection(const Ray& ray, const Triangle& triangle, float& t, Vector2& uv) noexcept {
    const Vector3 edge1 = triangle.v1 - triangle.v0;
    const Vector3 edge2 = triangle.v2 - triangle.v0;
    const Vector3 h = Cross(ray.direction, edge2);
    const float a = Dot(edge1, h);

    if (IsNearlyZero(a)) {
        return false; // Ray is parallel to triangle
    }

    const float f = 1.0f / a;
    const Vector3 s = ray.origin - triangle.v0;
    const float u = f * Dot(s, h);

    if (u < 0.0f || u > 1.0f) {
        return false;
    }

    const Vector3 q = Cross(s, edge1);
    const float v = f * Dot(ray.direction, q);

    if (v < 0.0f || u + v > 1.0f) {
        return false;
    }

    t = f * Dot(edge2, q);
    uv = Vector2(u, v);

    return t > EPSILON; // Ray intersection
}

bool Math::SphereSphereIntersection(const Sphere& a, const Sphere& b) noexcept {
    const float distanceSquared = DistanceSquared(a.center, b.center);
    const float radiusSum = a.radius + b.radius;
    return distanceSquared <= radiusSum * radiusSum;
}

bool Math::SphereAABBIntersection(const Sphere& sphere, const AABB& aabb) noexcept {
    return sphere.Intersects(aabb);
}

bool Math::AABBAABBIntersection(const AABB& a, const AABB& b) noexcept {
    return a.Intersects(b);
}

// =============================================================================
// Closest Point Calculations
// =============================================================================

Vector3 Math::ClosestPointOnRay(const Ray& ray, const Vector3& point) noexcept {
    const Vector3 toPoint = point - ray.origin;
    const float t = Max(0.0f, Dot(toPoint, ray.direction) / Dot(ray.direction, ray.direction));
    return ray.origin + ray.direction * t;
}

Vector3 Math::ClosestPointOnSegment(const Vector3& segmentStart, const Vector3& segmentEnd, const Vector3& point) noexcept {
    const Vector3 segment = segmentEnd - segmentStart;
    const Vector3 toPoint = point - segmentStart;

    const float segmentLengthSquared = LengthSquared(segment);
    if (IsNearlyZero(segmentLengthSquared)) {
        return segmentStart; // Degenerate segment
    }

    const float t = Clamp(Dot(toPoint, segment) / segmentLengthSquared, 0.0f, 1.0f);
    return segmentStart + segment * t;
}

Vector3 Math::ClosestPointOnTriangle(const Triangle& triangle, const Vector3& point) noexcept {
    return triangle.ClosestPoint(point);
}

Vector3 Math::ClosestPointOnAABB(const AABB& aabb, const Vector3& point) noexcept {
    return Vector3(
        Clamp(point.x, aabb.min.x, aabb.max.x),
        Clamp(point.y, aabb.min.y, aabb.max.y),
        Clamp(point.z, aabb.min.z, aabb.max.z)
    );
}

Vector3 Math::ClosestPointOnSphere(const Sphere& sphere, const Vector3& point) noexcept {
    const Vector3 direction = point - sphere.center;
    const float distance = Length(direction);

    if (IsNearlyZero(distance)) {
        // Point is at center, return any point on sphere surface
        return sphere.center + Vector3::UNIT_X * sphere.radius;
    }

    return sphere.center + (direction / distance) * sphere.radius;
}


// =============================================================================
// Hash Functions
// =============================================================================

uint32_t Math::Hash(uint32_t value) noexcept {
    // Wang's hash
    value = (value ^ 61) ^ (value >> 16);
    value *= 9;
    value = value ^ (value >> 4);
    value *= 0x27d4eb2d;
    value = value ^ (value >> 15);
    return value;
}

uint32_t Math::Hash(const Vector2& v) noexcept {
    uint32_t result = 0;
    result ^= Hash(*reinterpret_cast<const uint32_t*>(&v.x)) + 0x9e3779b9 + (result << 6) + (result >> 2);
    result ^= Hash(*reinterpret_cast<const uint32_t*>(&v.y)) + 0x9e3779b9 + (result << 6) + (result >> 2);
    return result;
}

uint32_t Math::Hash(const Vector3& v) noexcept {
    uint32_t result = 0;
    result ^= Hash(*reinterpret_cast<const uint32_t*>(&v.x)) + 0x9e3779b9 + (result << 6) + (result >> 2);
    result ^= Hash(*reinterpret_cast<const uint32_t*>(&v.y)) + 0x9e3779b9 + (result << 6) + (result >> 2);
    result ^= Hash(*reinterpret_cast<const uint32_t*>(&v.z)) + 0x9e3779b9 + (result << 6) + (result >> 2);
    return result;
}

uint32_t Math::Hash(const Vector4& v) noexcept {
    uint32_t result = 0;
    result ^= Hash(*reinterpret_cast<const uint32_t*>(&v.x)) + 0x9e3779b9 + (result << 6) + (result >> 2);
    result ^= Hash(*reinterpret_cast<const uint32_t*>(&v.y)) + 0x9e3779b9 + (result << 6) + (result >> 2);
    result ^= Hash(*reinterpret_cast<const uint32_t*>(&v.z)) + 0x9e3779b9 + (result << 6) + (result >> 2);
    result ^= Hash(*reinterpret_cast<const uint32_t*>(&v.w)) + 0x9e3779b9 + (result << 6) + (result >> 2);
    return result;
}

uint64_t Math::Hash64(uint64_t value) noexcept {
    // Splitmix64 hash
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

// =============================================================================
// Compression/Encoding Functions
// =============================================================================

uint32_t Math::PackColor(const Color4& color) noexcept {
    const uint32_t r = static_cast<uint32_t>(Saturate(color.r) * 255.0f + 0.5f);
    const uint32_t g = static_cast<uint32_t>(Saturate(color.g) * 255.0f + 0.5f);
    const uint32_t b = static_cast<uint32_t>(Saturate(color.b) * 255.0f + 0.5f);
    const uint32_t a = static_cast<uint32_t>(Saturate(color.a) * 255.0f + 0.5f);

    return (a << 24) | (r << 16) | (g << 8) | b;
}

Color4 Math::UnpackColor(uint32_t packed) noexcept {
    const float invScale = 1.0f / 255.0f;
    return Color4(
        static_cast<float>((packed >> 16) & 0xFF) * invScale,
        static_cast<float>((packed >> 8) & 0xFF) * invScale,
        static_cast<float>(packed & 0xFF) * invScale,
        static_cast<float>((packed >> 24) & 0xFF) * invScale
    );
}

uint32_t Math::PackNormal(const Vector3& normal) noexcept {
    // Pack normal into 32-bit using spheremap transform
    const Vector3 n = Normalize(normal);
    const float f = std::sqrt(8.0f * n.z + 8.0f);
    const float x = n.x / f + 0.5f;
    const float y = n.y / f + 0.5f;

    const uint32_t ix = static_cast<uint32_t>(Saturate(x) * 65535.0f + 0.5f);
    const uint32_t iy = static_cast<uint32_t>(Saturate(y) * 65535.0f + 0.5f);

    return (ix << 16) | iy;
}

Vector3 Math::UnpackNormal(uint32_t packed) noexcept {
    // Unpack normal from 32-bit spheremap
    const float x = (static_cast<float>((packed >> 16) & 0xFFFF) / 65535.0f) * 2.0f - 1.0f;
    const float y = (static_cast<float>(packed & 0xFFFF) / 65535.0f) * 2.0f - 1.0f;

    const float z = 1.0f - Abs(x) - Abs(y);
    const float t = Saturate(-z);

    return Normalize(Vector3(
        x + (x >= 0.0f ? -t : t),
        y + (y >= 0.0f ? -t : t),
        z
    ));
}

// =============================================================================
// Coordinate System Conversions
// =============================================================================

Vector3 Math::CartesianToSpherical(const Vector3& cartesian) noexcept {
    const float r = Length(cartesian);
    if (IsNearlyZero(r)) {
        return Vector3::ZERO;
    }

    const float theta = std::atan2(cartesian.y, cartesian.x); // Azimuthal angle
    const float phi = std::acos(Clamp(cartesian.z / r, -1.0f, 1.0f)); // Polar angle

    return Vector3(r, theta, phi);
}

Vector3 Math::SphericalToCartesian(const Vector3& spherical) noexcept {
    const float r = spherical.x;
    const float theta = spherical.y;
    const float phi = spherical.z;

    const float sinPhi = std::sin(phi);

    return Vector3(
        r * sinPhi * std::cos(theta),
        r * sinPhi * std::sin(theta),
        r * std::cos(phi)
    );
}

Vector3 Math::CartesianToCylindrical(const Vector3& cartesian) noexcept {
    const float rho = std::sqrt(cartesian.x * cartesian.x + cartesian.y * cartesian.y);
    const float phi = std::atan2(cartesian.y, cartesian.x);

    return Vector3(rho, phi, cartesian.z);
}

Vector3 Math::CylindricalToCartesian(const Vector3& cylindrical) noexcept {
    const float rho = cylindrical.x;
    const float phi = cylindrical.y;
    const float z = cylindrical.z;

    return Vector3(
        rho * std::cos(phi),
        rho * std::sin(phi),
        z
    );
}

// =============================================================================
// Curves and Splines Implementation
// =============================================================================

namespace Math {
    // Bezier curve evaluation using De Casteljau's algorithm
    template<typename T, size_t Degree>
    constexpr T BezierCurve<T, Degree>::Evaluate(float t) const noexcept {
        if constexpr (Degree == 0) {
            return controlPoints[0];
        }
        else if constexpr (Degree == 1) {
            return Lerp(controlPoints[0], controlPoints[1], t);
        }
        else if constexpr (Degree == 2) {
            const T p01 = Lerp(controlPoints[0], controlPoints[1], t);
            const T p12 = Lerp(controlPoints[1], controlPoints[2], t);
            return Lerp(p01, p12, t);
        }
        else if constexpr (Degree == 3) {
            const T p01 = Lerp(controlPoints[0], controlPoints[1], t);
            const T p12 = Lerp(controlPoints[1], controlPoints[2], t);
            const T p23 = Lerp(controlPoints[2], controlPoints[3], t);
            const T p012 = Lerp(p01, p12, t);
            const T p123 = Lerp(p12, p23, t);
            return Lerp(p012, p123, t);
        }
        else {
            // General case for higher-degree curves
            std::array<T, Degree + 1> temp = controlPoints;

            for (size_t j = 1; j <= Degree; ++j) {
                for (size_t i = 0; i <= Degree - j; ++i) {
                    temp[i] = Lerp(temp[i], temp[i + 1], t);
                }
            }

            return temp[0];
        }
    }

    template<typename T, size_t Degree>
    constexpr T BezierCurve<T, Degree>::Derivative(float t) const noexcept {
        if constexpr (Degree == 0) {
            return T{}; // Zero derivative for constant curve
        }
        else {
            std::array<T, Degree> derivativePoints;
            for (size_t i = 0; i < Degree; ++i) {
                derivativePoints[i] = (controlPoints[i + 1] - controlPoints[i]) * static_cast<float>(Degree);
            }

            BezierCurve<T, Degree - 1> derivativeCurve{ derivativePoints };
            return derivativeCurve.Evaluate(t);
        }
    }

    template<typename T, size_t Degree>
    constexpr float BezierCurve<T, Degree>::Length(size_t subdivisions) const noexcept {
        if (subdivisions == 0) subdivisions = 1;

        float totalLength = 0.0f;
        T previousPoint = Evaluate(0.0f);

        for (size_t i = 1; i <= subdivisions; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(subdivisions);
            const T currentPoint = Evaluate(t);
            totalLength += Distance(previousPoint, currentPoint);
            previousPoint = currentPoint;
        }

        return totalLength;
    }

    // Catmull-Rom spline implementation
    template<typename T>
    constexpr T CatmullRomSpline<T>::Evaluate(float t) const noexcept {
        if (controlPoints.size() < 2) {
            return controlPoints.empty() ? T{} : controlPoints[0];
        }

        if (controlPoints.size() == 2) {
            return Lerp(controlPoints[0], controlPoints[1], t);
        }

        // Clamp t to valid range
        t = Clamp(t, 0.0f, 1.0f);

        // Scale t to segment range
        const float scaledT = t * static_cast<float>(controlPoints.size() - 1);
        const size_t segmentIndex = static_cast<size_t>(scaledT);
        const float localT = scaledT - static_cast<float>(segmentIndex);

        // Get control points for current segment
        const size_t i0 = (segmentIndex == 0) ? 0 : segmentIndex - 1;
        const size_t i1 = segmentIndex;
        const size_t i2 = Min(segmentIndex + 1, controlPoints.size() - 1);
        const size_t i3 = Min(segmentIndex + 2, controlPoints.size() - 1);

        const T& p0 = controlPoints[i0];
        const T& p1 = controlPoints[i1];
        const T& p2 = controlPoints[i2];
        const T& p3 = controlPoints[i3];

        // Catmull-Rom interpolation
        const float t2 = localT * localT;
        const float t3 = t2 * localT;

        return 0.5f * (
            (2.0f * p1) +
            (-p0 + p2) * localT +
            (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
            (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
            );
    }

    template<typename T>
    constexpr T CatmullRomSpline<T>::Derivative(float t) const noexcept {
        if (controlPoints.size() < 2) {
            return T{};
        }

        if (controlPoints.size() == 2) {
            return controlPoints[1] - controlPoints[0];
        }

        // Clamp t to valid range
        t = Clamp(t, 0.0f, 1.0f);

        // Scale t to segment range
        const float scaledT = t * static_cast<float>(controlPoints.size() - 1);
        const size_t segmentIndex = static_cast<size_t>(scaledT);
        const float localT = scaledT - static_cast<float>(segmentIndex);

        // Get control points for current segment
        const size_t i0 = (segmentIndex == 0) ? 0 : segmentIndex - 1;
        const size_t i1 = segmentIndex;
        const size_t i2 = Min(segmentIndex + 1, controlPoints.size() - 1);
        const size_t i3 = Min(segmentIndex + 2, controlPoints.size() - 1);

        const T& p0 = controlPoints[i0];
        const T& p1 = controlPoints[i1];
        const T& p2 = controlPoints[i2];
        const T& p3 = controlPoints[i3];

        // Derivative of Catmull-Rom interpolation
        const float t2 = localT * localT;

        return 0.5f * (
            (-p0 + p2) +
            (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * 2.0f * localT +
            (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * 3.0f * t2
            ) * static_cast<float>(controlPoints.size() - 1);
    }

    template<typename T>
    constexpr size_t CatmullRomSpline<T>::GetSegmentCount() const noexcept {
        return controlPoints.size() > 1 ? controlPoints.size() - 1 : 0;
    }

    template<typename T>
    constexpr float CatmullRomSpline<T>::GetTotalLength(size_t subdivisions) const noexcept {
        if (subdivisions == 0) subdivisions = 1;

        float totalLength = 0.0f;
        T previousPoint = Evaluate(0.0f);

        for (size_t i = 1; i <= subdivisions; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(subdivisions);
            const T currentPoint = Evaluate(t);
            totalLength += Distance(previousPoint, currentPoint);
            previousPoint = currentPoint;
        }

        return totalLength;
    }

    // Explicit template instantiations for common types
    template class BezierCurve<Vector2, 2>;
    template class BezierCurve<Vector2, 3>;
    template class BezierCurve<Vector3, 2>;
    template class BezierCurve<Vector3, 3>;
    template class CatmullRomSpline<Vector2>;
    template class CatmullRomSpline<Vector3>;
}