// Core.Math.Quaternion.cpp
module;

#include <cmath>

module Akhanda.Core.Math;

using namespace Akhanda::Math;


// =============================================================================
// Quaternion Constructors
// =============================================================================

Quaternion::Quaternion(const Vector3& axis, float angle) noexcept {
    const float halfAngle = angle * 0.5f;
    const float sinHalfAngle = std::sin(halfAngle);
    const Vector3 normalizedAxis = Math::Normalize(axis);

    x = normalizedAxis.x * sinHalfAngle;
    y = normalizedAxis.y * sinHalfAngle;
    z = normalizedAxis.z * sinHalfAngle;
    w = std::cos(halfAngle);
}

Quaternion::Quaternion(const Vector3& eulerAngles) noexcept {
    *this = FromEulerAngles(eulerAngles);
}

// =============================================================================
// Quaternion Operators
// =============================================================================

Quaternion Quaternion::operator+(const Quaternion& other) const noexcept {
    return Quaternion(x + other.x, y + other.y, z + other.z, w + other.w);
}

Quaternion Quaternion::operator-(const Quaternion& other) const noexcept {
    return Quaternion(x - other.x, y - other.y, z - other.z, w - other.w);
}

Quaternion Quaternion::operator*(const Quaternion& other) const noexcept {
    return Quaternion(
        w * other.x + x * other.w + y * other.z - z * other.y,
        w * other.y - x * other.z + y * other.w + z * other.x,
        w * other.z + x * other.y - y * other.x + z * other.w,
        w * other.w - x * other.x - y * other.y - z * other.z
    );
}

Quaternion Quaternion::operator*(float scalar) const noexcept {
    return Quaternion(x * scalar, y * scalar, z * scalar, w * scalar);
}

Vector3 Quaternion::operator*(const Vector3& vector) const noexcept {
    // Optimized quaternion-vector multiplication
    const Vector3 qvec(x, y, z);
    const Vector3 uv = Cross(qvec, vector);
    const Vector3 uuv = Cross(qvec, uv);

    return vector + ((uv * w) + uuv) * 2.0f;
}

Quaternion& Quaternion::operator+=(const Quaternion& other) noexcept {
    x += other.x;
    y += other.y;
    z += other.z;
    w += other.w;
    return *this;
}

Quaternion& Quaternion::operator-=(const Quaternion& other) noexcept {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    w -= other.w;
    return *this;
}

Quaternion& Quaternion::operator*=(const Quaternion& other) noexcept {
    *this = *this * other;
    return *this;
}

Quaternion& Quaternion::operator*=(float scalar) noexcept {
    x *= scalar;
    y *= scalar;
    z *= scalar;
    w *= scalar;
    return *this;
}

bool Quaternion::operator==(const Quaternion& other) const noexcept {
    return IsNearlyEqual(x, other.x) && IsNearlyEqual(y, other.y) &&
        IsNearlyEqual(z, other.z) && IsNearlyEqual(w, other.w);
}

bool Quaternion::operator!=(const Quaternion& other) const noexcept {
    return !(*this == other);
}

// =============================================================================
// Quaternion Operations Implementation
// =============================================================================

float Quaternion::Length() noexcept {
    return std::sqrt(x * x + y * y + z * z + w * w);
}

float Quaternion::LengthSquared() noexcept {
    return (x * x + y * y + z * z + w * w);
}

float Quaternion::Dot(const Quaternion& b) noexcept {
    return x * b.x + y * b.y + z * b.z + w * b.w;
}

void Quaternion::Normalize() noexcept {
    const float length = Length();
    if (Akhanda::Math::IsNearlyZero(length)) {
        *this = Quaternion::IDENTITY;
    }
    else {
        *this *= (1.0f / length);
    }
}

Quaternion Quaternion::Conjugate() noexcept {
    return Quaternion(-x, -y, -z, w);
}

Quaternion Quaternion::Inverse() noexcept {
    const float lengthSq = LengthSquared();
    if (IsNearlyZero(lengthSq)) {
        return Quaternion::IDENTITY;
    }

    const Quaternion conjugate = Conjugate();
    return conjugate * (1.0f / lengthSq);
}

Matrix3 Quaternion::ToMatrix3() noexcept {
    const Quaternion normalized = Math::Normalize(*this);

    const float xx = normalized.x * normalized.x;
    const float yy = normalized.y * normalized.y;
    const float zz = normalized.z * normalized.z;
    const float xy = normalized.x * normalized.y;
    const float xz = normalized.x * normalized.z;
    const float yz = normalized.y * normalized.z;
    const float wx = normalized.w * normalized.x;
    const float wy = normalized.w * normalized.y;
    const float wz = normalized.w * normalized.z;

    return Matrix3(
        1.0f - 2.0f * (yy + zz), 2.0f * (xy - wz), 2.0f * (xz + wy),
        2.0f * (xy + wz), 1.0f - 2.0f * (xx + zz), 2.0f * (yz - wx),
        2.0f * (xz - wy), 2.0f * (yz + wx), 1.0f - 2.0f * (xx + yy)
    );
}

Matrix4 Quaternion::ToMatrix4() noexcept {
    const Quaternion normalized = Math::Normalize(*this);

    const float xx = normalized.x * normalized.x;
    const float yy = normalized.y * normalized.y;
    const float zz = normalized.z * normalized.z;
    const float xy = normalized.x * normalized.y;
    const float xz = normalized.x * normalized.z;
    const float yz = normalized.y * normalized.z;
    const float wx = normalized.w * normalized.x;
    const float wy = normalized.w * normalized.y;
    const float wz = normalized.w * normalized.z;

    return Matrix4(
        1.0f - 2.0f * (yy + zz), 2.0f * (xy - wz), 2.0f * (xz + wy), 0.0f,
        2.0f * (xy + wz), 1.0f - 2.0f * (xx + zz), 2.0f * (yz - wx), 0.0f,
        2.0f * (xz - wy), 2.0f * (yz + wx), 1.0f - 2.0f * (xx + yy), 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}




float Akhanda::Math::Dot(const Quaternion& a, const Quaternion& b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

float Akhanda::Math::Length(const Quaternion& q) noexcept {
    return std::sqrt(LengthSquared(q));
}

float Akhanda::Math::LengthSquared(const Quaternion& q) noexcept {
    return Dot(q, q);
}

Quaternion Akhanda::Math::Normalize(const Quaternion& q) noexcept {
    const float length = Length(q);
    return IsNearlyZero(length) ? Quaternion::IDENTITY : q * (1.0f / length);
}

Quaternion Akhanda::Math::Conjugate(const Quaternion& q) noexcept {
    return Quaternion(-q.x, -q.y, -q.z, q.w);
}

Quaternion Akhanda::Math::Inverse(const Quaternion& q) noexcept {
    const float lengthSq = LengthSquared(q);
    if (IsNearlyZero(lengthSq)) {
        return Quaternion::IDENTITY;
    }

    const Quaternion conjugate = Conjugate(q);
    return conjugate * (1.0f / lengthSq);
}

Matrix4 Akhanda::Math::ToMatrix(const Quaternion& q) noexcept {
    const Quaternion normalized = Normalize(q);

    const float xx = normalized.x * normalized.x;
    const float yy = normalized.y * normalized.y;
    const float zz = normalized.z * normalized.z;
    const float xy = normalized.x * normalized.y;
    const float xz = normalized.x * normalized.z;
    const float yz = normalized.y * normalized.z;
    const float wx = normalized.w * normalized.x;
    const float wy = normalized.w * normalized.y;
    const float wz = normalized.w * normalized.z;

    return Matrix4(
        1.0f - 2.0f * (yy + zz), 2.0f * (xy - wz), 2.0f * (xz + wy), 0.0f,
        2.0f * (xy + wz), 1.0f - 2.0f * (xx + zz), 2.0f * (yz - wx), 0.0f,
        2.0f * (xz - wy), 2.0f * (yz + wx), 1.0f - 2.0f * (xx + yy), 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

Matrix3 Akhanda::Math::ToMatrix3(const Quaternion& q) noexcept {
    const Quaternion normalized = Normalize(q);

    const float xx = normalized.x * normalized.x;
    const float yy = normalized.y * normalized.y;
    const float zz = normalized.z * normalized.z;
    const float xy = normalized.x * normalized.y;
    const float xz = normalized.x * normalized.z;
    const float yz = normalized.y * normalized.z;
    const float wx = normalized.w * normalized.x;
    const float wy = normalized.w * normalized.y;
    const float wz = normalized.w * normalized.z;

    return Matrix3(
        1.0f - 2.0f * (yy + zz), 2.0f * (xy - wz), 2.0f * (xz + wy),
        2.0f * (xy + wz), 1.0f - 2.0f * (xx + zz), 2.0f * (yz - wx),
        2.0f * (xz - wy), 2.0f * (yz + wx), 1.0f - 2.0f * (xx + yy)
    );
}

Vector3 Akhanda::Math::ToEulerAngles(const Quaternion& q) noexcept {
    const Quaternion normalized = Normalize(q);

    // Roll (x-axis rotation)
    const float sinr_cosp = 2.0f * (normalized.w * normalized.x + normalized.y * normalized.z);
    const float cosr_cosp = 1.0f - 2.0f * (normalized.x * normalized.x + normalized.y * normalized.y);
    const float roll = std::atan2(sinr_cosp, cosr_cosp);

    // Pitch (y-axis rotation)
    const float sinp = 2.0f * (normalized.w * normalized.y - normalized.z * normalized.x);
    float pitch;
    if (Abs(sinp) >= 1.0f) {
        pitch = std::copysign(HALF_PI, sinp); // Use 90 degrees if out of range
    }
    else {
        pitch = std::asin(sinp);
    }

    // Yaw (z-axis rotation)
    const float siny_cosp = 2.0f * (normalized.w * normalized.z + normalized.x * normalized.y);
    const float cosy_cosp = 1.0f - 2.0f * (normalized.y * normalized.y + normalized.z * normalized.z);
    const float yaw = std::atan2(siny_cosp, cosy_cosp);

    return Vector3(roll, pitch, yaw);
}

Quaternion Akhanda::Math::FromEulerAngles(const Vector3& eulerAngles) noexcept {
    const float halfRoll = eulerAngles.x * 0.5f;
    const float halfPitch = eulerAngles.y * 0.5f;
    const float halfYaw = eulerAngles.z * 0.5f;

    const float cosRoll = std::cos(halfRoll);
    const float sinRoll = std::sin(halfRoll);
    const float cosPitch = std::cos(halfPitch);
    const float sinPitch = std::sin(halfPitch);
    const float cosYaw = std::cos(halfYaw);
    const float sinYaw = std::sin(halfYaw);

    return Quaternion(
        sinRoll * cosPitch * cosYaw - cosRoll * sinPitch * sinYaw,
        cosRoll * sinPitch * cosYaw + sinRoll * cosPitch * sinYaw,
        cosRoll * cosPitch * sinYaw - sinRoll * sinPitch * cosYaw,
        cosRoll * cosPitch * cosYaw + sinRoll * sinPitch * sinYaw
    );
}

Quaternion Akhanda::Math::FromAxisAngle(const Vector3& axis, float angle) noexcept {
    return Quaternion(axis, angle);
}

Quaternion Akhanda::Math::FromRotationMatrix(const Matrix3& matrix) noexcept {
    const float trace = matrix(0, 0) + matrix(1, 1) + matrix(2, 2);

    if (trace > 0.0f) {
        const float s = std::sqrt(trace + 1.0f) * 2.0f; // s = 4 * qw
        return Quaternion(
            (matrix(2, 1) - matrix(1, 2)) / s,
            (matrix(0, 2) - matrix(2, 0)) / s,
            (matrix(1, 0) - matrix(0, 1)) / s,
            0.25f * s
        );
    }
    else if (matrix(0, 0) > matrix(1, 1) && matrix(0, 0) > matrix(2, 2)) {
        const float s = std::sqrt(1.0f + matrix(0, 0) - matrix(1, 1) - matrix(2, 2)) * 2.0f; // s = 4 * qx
        return Quaternion(
            0.25f * s,
            (matrix(0, 1) + matrix(1, 0)) / s,
            (matrix(0, 2) + matrix(2, 0)) / s,
            (matrix(2, 1) - matrix(1, 2)) / s
        );
    }
    else if (matrix(1, 1) > matrix(2, 2)) {
        const float s = std::sqrt(1.0f + matrix(1, 1) - matrix(0, 0) - matrix(2, 2)) * 2.0f; // s = 4 * qy
        return Quaternion(
            (matrix(0, 1) + matrix(1, 0)) / s,
            0.25f * s,
            (matrix(1, 2) + matrix(2, 1)) / s,
            (matrix(0, 2) - matrix(2, 0)) / s
        );
    }
    else {
        const float s = std::sqrt(1.0f + matrix(2, 2) - matrix(0, 0) - matrix(1, 1)) * 2.0f; // s = 4 * qz
        return Quaternion(
            (matrix(0, 2) + matrix(2, 0)) / s,
            (matrix(1, 2) + matrix(2, 1)) / s,
            0.25f * s,
            (matrix(1, 0) - matrix(0, 1)) / s
        );
    }
}

Quaternion Akhanda::Math::LookRotation(const Vector3& forward, const Vector3& up) noexcept {
    const Vector3 normalizedForward = Normalize(forward);
    const Vector3 normalizedUp = Normalize(up);

    const Vector3 right = Normalize(Cross(normalizedUp, normalizedForward));
    const Vector3 newUp = Cross(normalizedForward, right);

    const Matrix3 rotationMatrix(
        right.x, newUp.x, normalizedForward.x,
        right.y, newUp.y, normalizedForward.y,
        right.z, newUp.z, normalizedForward.z
    );

    return FromRotationMatrix(rotationMatrix);
}

float Akhanda::Math::Angle(const Quaternion& a, const Quaternion& b) noexcept {
    const float dot = Abs(Dot(Normalize(a), Normalize(b)));
    const float clampedDot = Clamp(dot, 0.0f, 1.0f);
    return 2.0f * std::acos(clampedDot);
}

// =============================================================================
// Quaternion Interpolation
// =============================================================================

Quaternion Akhanda::Math::Slerp(const Quaternion& a, const Quaternion& b, float t) noexcept {
    const Quaternion normA = Normalize(a);
    Quaternion normB = Normalize(b);

    float dot = Dot(normA, normB);

    // If the dot product is negative, the quaternions are more than 90 degrees apart.
    // So we can invert one to reduce spinning.
    if (dot < 0.0f) {
        normB = Quaternion(-normB.x, -normB.y, -normB.z, -normB.w);
        dot = -dot;
    }

    // If the inputs are too close for comfort, linearly interpolate and normalize
    if (dot > 0.9995f) {
        return Normalize(Quaternion(
            Lerp(normA.x, normB.x, t),
            Lerp(normA.y, normB.y, t),
            Lerp(normA.z, normB.z, t),
            Lerp(normA.w, normB.w, t)
        ));
    }

    // Since dot is in range [0, 1], acos is safe
    const float theta_0 = std::acos(dot);
    const float theta = theta_0 * t;
    const float sin_theta = std::sin(theta);
    const float sin_theta_0 = std::sin(theta_0);

    const float s0 = std::cos(theta) - dot * sin_theta / sin_theta_0;
    const float s1 = sin_theta / sin_theta_0;

    return Quaternion(
        s0 * normA.x + s1 * normB.x,
        s0 * normA.y + s1 * normB.y,
        s0 * normA.z + s1 * normB.z,
        s0 * normA.w + s1 * normB.w
    );
}

Quaternion Akhanda::Math::Nlerp(const Quaternion& a, const Quaternion& b, float t) noexcept {
    const Quaternion normA = Normalize(a);
    Quaternion normB = Normalize(b);

    float dot = Dot(normA, normB);

    // Take the shorter path
    if (dot < 0.0f) {
        normB = Quaternion(-normB.x, -normB.y, -normB.z, -normB.w);
    }

    return Normalize(Quaternion(
        Lerp(normA.x, normB.x, t),
        Lerp(normA.y, normB.y, t),
        Lerp(normA.z, normB.z, t),
        Lerp(normA.w, normB.w, t)
    ));
}

// =============================================================================
// Vector Interpolation Implementation
// =============================================================================

Vector2 Akhanda::Math::LerpV2(const Vector2& a, const Vector2& b, float t) noexcept {
    return Vector2(
        Lerp(a.x, b.x, t),
        Lerp(a.y, b.y, t)
    );
}

Vector3 Akhanda::Math::LerpV3(const Vector3& a, const Vector3& b, float t) noexcept {
    return Vector3(
        Lerp(a.x, b.x, t),
        Lerp(a.y, b.y, t),
        Lerp(a.z, b.z, t)
    );
}

Vector4 Akhanda::Math::LerpV4(const Vector4& a, const Vector4& b, float t) noexcept {
    return Vector4(
        Lerp(a.x, b.x, t),
        Lerp(a.y, b.y, t),
        Lerp(a.z, b.z, t),
        Lerp(a.w, b.w, t)
    );
}