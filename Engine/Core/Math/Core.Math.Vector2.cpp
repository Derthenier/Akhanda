// Core.Math.Vector2.cpp
module Akhanda.Core.Math;

using namespace Akhanda::Math;


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
