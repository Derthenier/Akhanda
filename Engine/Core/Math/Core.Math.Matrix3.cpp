// Core.Math.Matrix.cpp
module Akhanda.Core.Math;

using namespace Akhanda::Math;


// =============================================================================
// Matrix3 Implementation
// =============================================================================
#pragma region Matrix3

Matrix3 Matrix3::operator+(const Matrix3& other) const noexcept {
    Matrix3 result;
    for (int i = 0; i < 9; ++i) {
        result.m[i] = m[i] + other.m[i];
    }
    return result;
}

Matrix3 Matrix3::operator-(const Matrix3& other) const noexcept {
    Matrix3 result;
    for (int i = 0; i < 9; ++i) {
        result.m[i] = m[i] - other.m[i];
    }
    return result;
}

Matrix3 Matrix3::operator*(const Matrix3& other) const noexcept {
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

Matrix3 Matrix3::operator*(float scalar) const noexcept {
    Matrix3 result;
    for (int i = 0; i < 9; ++i) {
        result.m[i] = m[i] * scalar;
    }
    return result;
}

Vector3 Matrix3::operator*(const Vector3& vector) const noexcept {
    return Vector3(
        (*this)(0, 0) * vector.x + (*this)(0, 1) * vector.y + (*this)(0, 2) * vector.z,
        (*this)(1, 0) * vector.x + (*this)(1, 1) * vector.y + (*this)(1, 2) * vector.z,
        (*this)(2, 0) * vector.x + (*this)(2, 1) * vector.y + (*this)(2, 2) * vector.z
    );
}

Matrix3& Matrix3::operator+=(const Matrix3& other) noexcept {
    for (int i = 0; i < 9; ++i) {
        m[i] += other.m[i];
    }
    return *this;
}

Matrix3& Matrix3::operator-=(const Matrix3& other) noexcept {
    for (int i = 0; i < 9; ++i) {
        m[i] -= other.m[i];
    }
    return *this;
}

Matrix3& Matrix3::operator*=(const Matrix3& other) noexcept {
    *this = *this * other;
    return *this;
}

Matrix3& Matrix3::operator*=(float scalar) noexcept {
    for (int i = 0; i < 9; ++i) {
        m[i] *= scalar;
    }
    return *this;
}

bool Matrix3::operator==(const Matrix3& other) const noexcept {
    for (int i = 0; i < 9; ++i) {
        if (!IsNearlyEqual(m[i], other.m[i])) {
            return false;
        }
    }
    return true;
}

bool Matrix3::operator!=(const Matrix3& other) const noexcept {
    return !(*this == other);
}

#pragma endregion
