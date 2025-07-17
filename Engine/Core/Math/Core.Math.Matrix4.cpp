// Core.Math.Matrix4.cpp
module Akhanda.Core.Math;

using namespace Akhanda::Math;


// =============================================================================
// Matrix4 Implementation
// =============================================================================
#pragma region Matrix4

Matrix4 Matrix4::operator+(const Matrix4& other) const noexcept {
    Matrix4 result;
    for (int i = 0; i < 16; ++i) {
        result.m[i] = m[i] + other.m[i];
    }
    return result;
}

Matrix4 Matrix4::operator-(const Matrix4& other) const noexcept {
    Matrix4 result;
    for (int i = 0; i < 16; ++i) {
        result.m[i] = m[i] - other.m[i];
    }
    return result;
}

Matrix4 Matrix4::operator*(const Matrix4& other) const noexcept {
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

Matrix4 Matrix4::operator*(float scalar) const noexcept {
    Matrix4 result;
    for (int i = 0; i < 16; ++i) {
        result.m[i] = m[i] * scalar;
    }
    return result;
}

Vector4 Matrix4::operator*(const Vector4& vector) const noexcept {
    return Vector4(
        (*this)(0, 0) * vector.x + (*this)(0, 1) * vector.y + (*this)(0, 2) * vector.z + (*this)(0, 3) * vector.w,
        (*this)(1, 0) * vector.x + (*this)(1, 1) * vector.y + (*this)(1, 2) * vector.z + (*this)(1, 3) * vector.w,
        (*this)(2, 0) * vector.x + (*this)(2, 1) * vector.y + (*this)(2, 2) * vector.z + (*this)(2, 3) * vector.w,
        (*this)(3, 0) * vector.x + (*this)(3, 1) * vector.y + (*this)(3, 2) * vector.z + (*this)(3, 3) * vector.w
    );
}

Vector3 Matrix4::TransformPoint(const Vector3& point) const noexcept {
    const Vector4 result = *this * Vector4(point, 1.0f);
    return Vector3(result.x, result.y, result.z) / result.w;
}

Vector3 Matrix4::TransformVector(const Vector3& vector) const noexcept {
    const Vector4 result = *this * Vector4(vector, 0.0f);
    return Vector3(result.x, result.y, result.z);
}

Matrix4& Matrix4::operator+=(const Matrix4& other) noexcept {
    for (int i = 0; i < 16; ++i) {
        m[i] += other.m[i];
    }
    return *this;
}

Matrix4& Matrix4::operator-=(const Matrix4& other) noexcept {
    for (int i = 0; i < 16; ++i) {
        m[i] -= other.m[i];
    }
    return *this;
}

Matrix4& Matrix4::operator*=(const Matrix4& other) noexcept {
    *this = *this * other;
    return *this;
}

Matrix4& Matrix4::operator*=(float scalar) noexcept {
    for (int i = 0; i < 16; ++i) {
        m[i] *= scalar;
    }
    return *this;
}

bool Matrix4::operator==(const Matrix4& other) const noexcept {
    for (int i = 0; i < 16; ++i) {
        if (!IsNearlyEqual(m[i], other.m[i])) {
            return false;
        }
    }
    return true;
}

bool Matrix4::operator!=(const Matrix4& other) const noexcept {
    return !(*this == other);
}

#pragma endregion
