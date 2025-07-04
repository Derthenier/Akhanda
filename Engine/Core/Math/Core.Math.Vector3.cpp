// Core.Math.Vector3.cpp
module Core.Math;

using namespace Math;

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
