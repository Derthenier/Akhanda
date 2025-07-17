// Core.Math.Vector4.cpp
module Akhanda.Core.Math;

using namespace Akhanda::Math;


// =============================================================================
// Vector4 Implementation
// =============================================================================
#pragma region Vector4

Vector4 Vector4::operator+(const Vector4& other) const noexcept {
    return Vector4(x + other.x, y + other.y, z + other.z, w + other.w);
}

Vector4 Vector4::operator-(const Vector4& other) const noexcept {
    return Vector4(x - other.x, y - other.y, z - other.z, w - other.w);
}

Vector4 Vector4::operator*(const Vector4& other) const noexcept {
    return Vector4(x * other.x, y * other.y, z * other.z, w * other.w);
}

Vector4 Vector4::operator/(const Vector4& other) const noexcept {
    return Vector4(x / other.x, y / other.y, z / other.z, w / other.w);
}

Vector4 Vector4::operator*(float scalar) const noexcept {
    return Vector4(x * scalar, y * scalar, z * scalar, w * scalar);
}

Vector4 Vector4::operator/(float scalar) const noexcept {
    const float invScalar = 1.0f / scalar;
    return Vector4(x * invScalar, y * invScalar, z * invScalar, w * invScalar);
}

Vector4& Vector4::operator+=(const Vector4& other) noexcept {
    x += other.x;
    y += other.y;
    z += other.z;
    w += other.w;
    return *this;
}

Vector4& Vector4::operator-=(const Vector4& other) noexcept {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    w -= other.w;
    return *this;
}

Vector4& Vector4::operator*=(const Vector4& other) noexcept {
    x *= other.x;
    y *= other.y;
    z *= other.z;
    w *= other.w;
    return *this;
}

Vector4& Vector4::operator/=(const Vector4& other) noexcept {
    x /= other.x;
    y /= other.y;
    z /= other.z;
    w /= other.w;
    return *this;
}

Vector4& Vector4::operator*=(float scalar) noexcept {
    x *= scalar;
    y *= scalar;
    z *= scalar;
    w *= scalar;
    return *this;
}

Vector4& Vector4::operator/=(float scalar) noexcept {
    const float invScalar = 1.0f / scalar;
    x *= invScalar;
    y *= invScalar;
    z *= invScalar;
    w *= invScalar;
    return *this;
}

Vector4 Vector4::operator-() const noexcept {
    return Vector4(-x, -y, -z, -w);
}

Vector4 Vector4::operator+() const noexcept {
    return *this;
}

bool Vector4::operator==(const Vector4& other) const noexcept {
    return IsNearlyEqual(other);
}

bool Vector4::operator!=(const Vector4& other) const noexcept {
    return !(*this == other);
}

// =============================================================================
// Vector4 Member Functions Implementation
// =============================================================================

float Vector4::Length() const noexcept {
    return std::sqrt(x * x + y * y + z * z + w * w);
}

float Vector4::LengthSquared() const noexcept {
    return x * x + y * y + z * z + w * w;
}

Vector4 Vector4::Normalized() const noexcept {
    const float len = Length();
    if (len > EPSILON) {
        const float invLen = 1.0f / len;
        return Vector4(x * invLen, y * invLen, z * invLen, w * invLen);
    }
    return Vector4::ZERO;
}

Vector4& Vector4::Normalize() noexcept {
    const float len = Length();
    if (len > EPSILON) {
        const float invLen = 1.0f / len;
        x *= invLen;
        y *= invLen;
        z *= invLen;
        w *= invLen;
    }
    else {
        *this = Vector4::ZERO;
    }
    return *this;
}

float Vector4::Dot(const Vector4& other) const noexcept {
    return x * other.x + y * other.y + z * other.z + w * other.w;
}

float Vector4::Distance(const Vector4& other) const noexcept {
    return (*this - other).Length();
}

float Vector4::DistanceSquared(const Vector4& other) const noexcept {
    return (*this - other).LengthSquared();
}

Vector4 Vector4::Lerp(const Vector4& other, float t) const noexcept {
    return *this + (other - *this) * t;
}

Vector4 Vector4::Slerp(const Vector4& other, float t) const noexcept {
    const float dot = Clamp(Dot(other), -1.0f, 1.0f);
    const float theta = std::acos(dot) * t;
    const Vector4 relative = (other - *this * dot).Normalized();
    return *this * std::cos(theta) + relative * std::sin(theta);
}

Vector4 Vector4::Project(const Vector4& onto) const noexcept {
    const float ontoLengthSq = onto.LengthSquared();
    if (ontoLengthSq > EPSILON) {
        return onto * (Dot(onto) / ontoLengthSq);
    }
    return Vector4::ZERO;
}

Vector4 Vector4::Reject(const Vector4& from) const noexcept {
    return *this - Project(from);
}

Vector4 Vector4::Reflect(const Vector4& normal) const noexcept {
    return *this - 2.0f * Dot(normal) * normal;
}

bool Vector4::IsNearlyZero(float tolerance) const noexcept {
    return Abs(x) <= tolerance && Abs(y) <= tolerance && Abs(z) <= tolerance;
}

bool Vector4::IsNormalized(float tolerance) const noexcept {
    return Math::IsNearlyEqual(LengthSquared(), 1.0f, tolerance);
}

bool Vector4::IsNearlyEqual(const Vector4& other, float tolerance) const noexcept {
    return Math::IsNearlyEqual(x, other.x, tolerance) &&
        Math::IsNearlyEqual(y, other.y, tolerance) &&
        Math::IsNearlyEqual(z, other.z, tolerance) &&
        Math::IsNearlyEqual(w, other.w, tolerance);
}

#pragma endregion
