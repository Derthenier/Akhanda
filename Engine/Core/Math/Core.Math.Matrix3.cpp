// Core.Math.Matrix.cpp
module Core.Math;

using namespace Math;


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
