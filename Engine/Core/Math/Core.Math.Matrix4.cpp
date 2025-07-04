// Core.Math.Matrix4.cpp
module Akhanda.Core.Math;

using namespace Akhanda::Math;


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
