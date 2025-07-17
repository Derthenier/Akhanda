// Core.Math.Vector3.cpp
module Akhanda.Core.Math;

using namespace Akhanda::Math;

// =============================================================================
// Vector3 Implementation
// =============================================================================
#pragma region Vector3

Vector3 Vector3::operator+(const Vector3& other) const noexcept {
    return Vector3(x + other.x, y + other.y, z + other.z);
}

Vector3 Vector3::operator-(const Vector3& other) const noexcept {
    return Vector3(x - other.x, y - other.y, z - other.z);
}

Vector3 Vector3::operator*(const Vector3& other) const noexcept {
    return Vector3(x * other.x, y * other.y, z * other.z);
}

Vector3 Vector3::operator/(const Vector3& other) const noexcept {
    return Vector3(x / other.x, y / other.y, z / other.z);
}

Vector3 Vector3::operator*(float scalar) const noexcept {
    return Vector3(x * scalar, y * scalar, z * scalar);
}

Vector3 Vector3::operator/(float scalar) const noexcept {
    const float invScalar = 1.0f / scalar;
    return Vector3(x * invScalar, y * invScalar, z * invScalar);
}

Vector3& Vector3::operator+=(const Vector3& other) noexcept {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}

Vector3& Vector3::operator-=(const Vector3& other) noexcept {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
}

Vector3& Vector3::operator*=(const Vector3& other) noexcept {
    x *= other.x;
    y *= other.y;
    z *= other.z;
    return *this;
}

Vector3& Vector3::operator/=(const Vector3& other) noexcept {
    x /= other.x;
    y /= other.y;
    z /= other.z;
    return *this;
}

Vector3& Vector3::operator*=(float scalar) noexcept {
    x *= scalar;
    y *= scalar;
    z *= scalar;
    return *this;
}

Vector3& Vector3::operator/=(float scalar) noexcept {
    const float invScalar = 1.0f / scalar;
    x *= invScalar;
    y *= invScalar;
    z *= invScalar;
    return *this;
}

Vector3 Vector3::operator-() const noexcept {
    return Vector3(-x, -y, -z);
}

Vector3 Vector3::operator+() const noexcept {
    return *this;
}

bool Vector3::operator==(const Vector3& other) const noexcept {
    return IsNearlyEqual(other);
}

bool Vector3::operator!=(const Vector3& other) const noexcept {
    return !(*this == other);
}
// =============================================================================
// Vector3 Member Functions Implementation
// =============================================================================

float Vector3::Length() const noexcept {
    return std::sqrt(x * x + y * y + z * z);
}

float Vector3::LengthSquared() const noexcept {
    return x * x + y * y + z * z;
}

Vector3 Vector3::Normalized() const noexcept {
    const float len = Length();
    if (len > EPSILON) {
        const float invLen = 1.0f / len;
        return Vector3(x * invLen, y * invLen, z * invLen);
    }
    return Vector3::ZERO;
}

Vector3& Vector3::Normalize() noexcept {
    const float len = Length();
    if (len > EPSILON) {
        const float invLen = 1.0f / len;
        x *= invLen;
        y *= invLen;
        z *= invLen;
    }
    else {
        *this = Vector3::ZERO;
    }
    return *this;
}

float Vector3::Dot(const Vector3& other) const noexcept {
    return x * other.x + y * other.y + z * other.z;
}

Vector3 Vector3::Cross(const Vector3& other) const noexcept {
    return Vector3(
        y * other.z - z * other.y,
        z * other.x - x * other.z,
        x * other.y - y * other.x
    );
}

float Vector3::Distance(const Vector3& other) const noexcept {
    return (*this - other).Length();
}

float Vector3::DistanceSquared(const Vector3& other) const noexcept {
    return (*this - other).LengthSquared();
}

Vector3 Vector3::Lerp(const Vector3& other, float t) const noexcept {
    return *this + (other - *this) * t;
}

Vector3 Vector3::Slerp(const Vector3& other, float t) const noexcept {
    const float dot = Clamp(Dot(other), -1.0f, 1.0f);
    const float theta = std::acos(dot) * t;
    const Vector3 relative = (other - *this * dot).Normalized();
    return *this * std::cos(theta) + relative * std::sin(theta);
}

Vector3 Vector3::Project(const Vector3& onto) const noexcept {
    const float ontoLengthSq = onto.LengthSquared();
    if (ontoLengthSq > EPSILON) {
        return onto * (Dot(onto) / ontoLengthSq);
    }
    return Vector3::ZERO;
}

Vector3 Vector3::Reject(const Vector3& from) const noexcept {
    return *this - Project(from);
}

Vector3 Vector3::Reflect(const Vector3& normal) const noexcept {
    return *this - 2.0f * Dot(normal) * normal;
}

bool Vector3::IsNearlyZero(float tolerance) const noexcept {
    return Abs(x) <= tolerance && Abs(y) <= tolerance && Abs(z) <= tolerance;
}

bool Vector3::IsNormalized(float tolerance) const noexcept {
    return Math::IsNearlyEqual(LengthSquared(), 1.0f, tolerance);
}

bool Vector3::IsNearlyEqual(const Vector3& other, float tolerance) const noexcept {
    return Math::IsNearlyEqual(x, other.x, tolerance) &&
        Math::IsNearlyEqual(y, other.y, tolerance) &&
        Math::IsNearlyEqual(z, other.z, tolerance);
}

#pragma endregion
