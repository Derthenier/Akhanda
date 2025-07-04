// Core.Math.Vector4.cpp
module Core.Math;

using namespace Math;


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
