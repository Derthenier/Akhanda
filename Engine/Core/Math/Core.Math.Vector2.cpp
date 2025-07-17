// Core.Math.Vector2.cpp
module Akhanda.Core.Math;

using namespace Akhanda::Math;


// =============================================================================
// Vector2 Implementation
// =============================================================================

#pragma region Vector2

float Vector2::Length() const noexcept {
    return std::sqrt(x * x + y * y);
}

float Vector2::LengthSquared() const noexcept {
    return x * x + y * y;
}

Vector2 Vector2::Normalized() const noexcept {
    const float len = Length();
    if (len > EPSILON) {
        const float invLen = 1.0f / len;
        return Vector2(x * invLen, y * invLen);
    }
    return Vector2::ZERO;
}
Vector2& Vector2::Normalize() noexcept {
    const float len = Length();
    if (len > EPSILON) {
        const float invLen = 1.0f / len;
        x *= invLen;
        y *= invLen;
    }
    else {
        *this = Vector2::ZERO;
    }
    return *this;
}

float Vector2::Dot(const Vector2& other) const noexcept {
    return x * other.x + y * other.y;
}

float Vector2::Distance(const Vector2& other) const noexcept {
    return (*this - other).Length();
}

float Vector2::DistanceSquared(const Vector2& other) const noexcept {
    return (*this - other).LengthSquared();
}

Vector2 Vector2::Lerp(const Vector2& other, float t) const noexcept {
    return *this + (other - *this) * t;
}

Vector2 Vector2::Slerp(const Vector2& other, float t) const noexcept {
    const float dot = Clamp(Dot(other), -1.0f, 1.0f);
    const float theta = std::acos(dot) * t;
    const Vector2 relative = (other - *this * dot).Normalized();
    return *this * std::cos(theta) + relative * std::sin(theta);
}

Vector2 Vector2::Project(const Vector2& onto) const noexcept {
    const float ontoLengthSq = onto.LengthSquared();
    if (ontoLengthSq > EPSILON) {
        return onto * (Dot(onto) / ontoLengthSq);
    }
    return Vector2::ZERO;
}

Vector2 Vector2::Reject(const Vector2& from) const noexcept {
    return *this - Project(from);
}

Vector2 Vector2::Reflect(const Vector2& normal) const noexcept {
    return *this - 2.0f * Dot(normal) * normal;
}

bool Vector2::IsNearlyZero(float tolerance) const noexcept {
    return Math::Abs(x) <= tolerance && Math::Abs(y) <= tolerance;
}

bool Vector2::IsNormalized(float tolerance) const noexcept {
    return Math::IsNearlyEqual(LengthSquared(), 1.0f, tolerance);
}

bool Vector2::IsNearlyEqual(const Vector2& other, float tolerance) const noexcept {
    return Math::IsNearlyEqual(this->x, other.x, tolerance) && Math::IsNearlyEqual(this->y, other.y, tolerance);
}

Vector2 Vector2::operator+(const Vector2& other) const noexcept {
    return Vector2(x + other.x, y + other.y);
}

Vector2 Vector2::operator-(const Vector2& other) const noexcept {
    return Vector2(x - other.x, y - other.y);
}

Vector2 Vector2::operator*(const Vector2& other) const noexcept {
    return Vector2(x * other.x, y * other.y);
}

Vector2 Vector2::operator/(const Vector2& other) const noexcept {
    return Vector2(x / other.x, y / other.y);
}

Vector2 Vector2::operator*(float scalar) const noexcept {
    return Vector2(x * scalar, y * scalar);
}

Vector2 Vector2::operator/(float scalar) const noexcept {
    const float invScalar = 1.0f / scalar;
    return Vector2(x * invScalar, y * invScalar);
}

Vector2& Vector2::operator+=(const Vector2& other) noexcept {
    x += other.x;
    y += other.y;
    return *this;
}

Vector2& Vector2::operator-=(const Vector2& other) noexcept {
    x -= other.x;
    y -= other.y;
    return *this;
}

Vector2& Vector2::operator*=(const Vector2& other) noexcept {
    x *= other.x;
    y *= other.y;
    return *this;
}

Vector2& Vector2::operator/=(const Vector2& other) noexcept {
    x /= other.x;
    y /= other.y;
    return *this;
}

Vector2& Vector2::operator*=(float scalar) noexcept {
    x *= scalar;
    y *= scalar;
    return *this;
}

Vector2& Vector2::operator/=(float scalar) noexcept {
    const float invScalar = 1.0f / scalar;
    x *= invScalar;
    y *= invScalar;
    return *this;
}

Vector2 Vector2::operator-() const noexcept {
    return Vector2(-x, -y);
}

Vector2 Vector2::operator+() const noexcept {
    return *this;
}

bool Vector2::operator==(const Vector2& other) const noexcept {
    return IsNearlyEqual(other);
}

bool Vector2::operator!=(const Vector2& other) const noexcept {
    return !(*this == other);
}

#pragma endregion
