// Core.Math.ixx - Comprehensive Math Module Interface
module;

#include <limits>

export module Core.Math;

import <algorithm>;
import <array>;
import <cmath>;
import <concepts>;
import <vector>;

// =============================================================================
// Forward Declarations
// =============================================================================

export namespace Math {
    // Basic Types
    struct Vector2;
    struct Vector3;
    struct Vector4;
    struct Matrix3;
    struct Matrix4;
    struct Quaternion;

    // Color Types
    struct Color3;
    struct Color4;

    // Geometric Primitives
    struct Ray;
    struct Plane;
    struct AABB;
    struct OBB;
    struct Sphere;
    struct Frustum;
    struct Triangle;

    // Advanced Math
    template<typename T, size_t Degree>
    struct BezierCurve;

    template<typename T>
    struct CatmullRomSpline;

    // Transform Types
    struct Transform;
    struct Transform2D;
}

// =============================================================================
// Constants
// =============================================================================

export namespace Math {
    inline constexpr float PI = 3.14159265358979323846f;
    inline constexpr float TWO_PI = 2.0f * PI;
    inline constexpr float HALF_PI = PI * 0.5f;
    inline constexpr float QUARTER_PI = PI * 0.25f;
    inline constexpr float INV_PI = 1.0f / PI;
    inline constexpr float INV_TWO_PI = 1.0f / TWO_PI;

    inline constexpr float DEG_TO_RAD = PI / 180.0f;
    inline constexpr float RAD_TO_DEG = 180.0f / PI;

    inline constexpr float EPSILON = 1e-6f;
    inline constexpr float LARGE_EPSILON = 1e-4f;
    inline constexpr float SMALL_EPSILON = 1e-8f;

    inline constexpr float INFINITY_F = std::numeric_limits<float>::infinity();
    inline constexpr float NEG_INFINITY_F = -std::numeric_limits<float>::infinity();
}

// =============================================================================
// Concepts
// =============================================================================

export namespace Math {
    template<typename T>
    concept Arithmetic = std::is_arithmetic_v<T>;

    template<typename T>
    concept FloatingPoint = std::floating_point<T>;

    template<typename T>
    concept Integral = std::integral<T>;
}

// =============================================================================
// Vector Types
// =============================================================================

export namespace Math {
    struct Vector2 {
        float x, y;

        // Constructors
        constexpr Vector2() noexcept : x(0), y(0) {}
        constexpr Vector2(float x, float y) noexcept : x(x), y(y) {}
        explicit constexpr Vector2(float scalar) noexcept : x(scalar), y(scalar) {}

        // Element access
        constexpr float& operator[](size_t index) noexcept;
        constexpr const float& operator[](size_t index) const noexcept;

        // Arithmetic operators
        constexpr Vector2 operator+(const Vector2& other) const noexcept;
        constexpr Vector2 operator-(const Vector2& other) const noexcept;
        constexpr Vector2 operator*(const Vector2& other) const noexcept;
        constexpr Vector2 operator/(const Vector2& other) const noexcept;
        constexpr Vector2 operator*(float scalar) const noexcept;
        constexpr Vector2 operator/(float scalar) const noexcept;

        // Assignment operators
        constexpr Vector2& operator+=(const Vector2& other) noexcept;
        constexpr Vector2& operator-=(const Vector2& other) noexcept;
        constexpr Vector2& operator*=(const Vector2& other) noexcept;
        constexpr Vector2& operator/=(const Vector2& other) noexcept;
        constexpr Vector2& operator*=(float scalar) noexcept;
        constexpr Vector2& operator/=(float scalar) noexcept;

        // Unary operators
        constexpr Vector2 operator-() const noexcept;
        constexpr Vector2 operator+() const noexcept;

        // Comparison operators
        constexpr bool operator==(const Vector2& other) const noexcept;
        constexpr bool operator!=(const Vector2& other) const noexcept;

        // Static constants
        static const Vector2 ZERO;
        static const Vector2 ONE;
        static const Vector2 UNIT_X;
        static const Vector2 UNIT_Y;
    };

    struct Vector3 {
        float x, y, z;

        // Constructors
        constexpr Vector3() noexcept : x(0), y(0), z(0) {}
        constexpr Vector3(float x, float y, float z) noexcept : x(x), y(y), z(z) {}
        constexpr Vector3(const Vector2& xy, float z) noexcept : x(xy.x), y(xy.y), z(z) {}
        explicit constexpr Vector3(float scalar) noexcept : x(scalar), y(scalar), z(scalar) {}

        // Element access
        constexpr float& operator[](size_t index) noexcept;
        constexpr const float& operator[](size_t index) const noexcept;

        // Arithmetic operators
        constexpr Vector3 operator+(const Vector3& other) const noexcept;
        constexpr Vector3 operator-(const Vector3& other) const noexcept;
        constexpr Vector3 operator*(const Vector3& other) const noexcept;
        constexpr Vector3 operator/(const Vector3& other) const noexcept;
        constexpr Vector3 operator*(float scalar) const noexcept;
        constexpr Vector3 operator/(float scalar) const noexcept;

        // Assignment operators
        constexpr Vector3& operator+=(const Vector3& other) noexcept;
        constexpr Vector3& operator-=(const Vector3& other) noexcept;
        constexpr Vector3& operator*=(const Vector3& other) noexcept;
        constexpr Vector3& operator/=(const Vector3& other) noexcept;
        constexpr Vector3& operator*=(float scalar) noexcept;
        constexpr Vector3& operator/=(float scalar) noexcept;

        // Unary operators
        constexpr Vector3 operator-() const noexcept;
        constexpr Vector3 operator+() const noexcept;

        // Comparison operators
        constexpr bool operator==(const Vector3& other) const noexcept;
        constexpr bool operator!=(const Vector3& other) const noexcept;

        // Swizzling
        constexpr Vector2 xy() const noexcept { return Vector2(x, y); }
        constexpr Vector2 xz() const noexcept { return Vector2(x, z); }
        constexpr Vector2 yz() const noexcept { return Vector2(y, z); }

        // Static constants
        static const Vector3 ZERO;
        static const Vector3 ONE;
        static const Vector3 UNIT_X;
        static const Vector3 UNIT_Y;
        static const Vector3 UNIT_Z;
        static const Vector3 UP;
        static const Vector3 DOWN;
        static const Vector3 FORWARD;
        static const Vector3 BACK;
        static const Vector3 LEFT;
        static const Vector3 RIGHT;
    };

    struct Vector4 {
        float x, y, z, w;

        // Constructors
        constexpr Vector4() noexcept : x(0), y(0), z(0), w(0) {}
        constexpr Vector4(float x, float y, float z, float w) noexcept : x(x), y(y), z(z), w(w) {}
        constexpr Vector4(const Vector3& xyz, float w) noexcept : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}
        constexpr Vector4(const Vector2& xy, const Vector2& zw) noexcept : x(xy.x), y(xy.y), z(zw.x), w(zw.y) {}
        explicit constexpr Vector4(float scalar) noexcept : x(scalar), y(scalar), z(scalar), w(scalar) {}

        // Element access
        constexpr float& operator[](size_t index) noexcept;
        constexpr const float& operator[](size_t index) const noexcept;

        // Arithmetic operators
        constexpr Vector4 operator+(const Vector4& other) const noexcept;
        constexpr Vector4 operator-(const Vector4& other) const noexcept;
        constexpr Vector4 operator*(const Vector4& other) const noexcept;
        constexpr Vector4 operator/(const Vector4& other) const noexcept;
        constexpr Vector4 operator*(float scalar) const noexcept;
        constexpr Vector4 operator/(float scalar) const noexcept;

        // Assignment operators
        constexpr Vector4& operator+=(const Vector4& other) noexcept;
        constexpr Vector4& operator-=(const Vector4& other) noexcept;
        constexpr Vector4& operator*=(const Vector4& other) noexcept;
        constexpr Vector4& operator/=(const Vector4& other) noexcept;
        constexpr Vector4& operator*=(float scalar) noexcept;
        constexpr Vector4& operator/=(float scalar) noexcept;

        // Unary operators
        constexpr Vector4 operator-() const noexcept;
        constexpr Vector4 operator+() const noexcept;

        // Comparison operators
        constexpr bool operator==(const Vector4& other) const noexcept;
        constexpr bool operator!=(const Vector4& other) const noexcept;

        // Swizzling
        constexpr Vector2 xy() const noexcept { return Vector2(x, y); }
        constexpr Vector2 zw() const noexcept { return Vector2(z, w); }
        constexpr Vector3 xyz() const noexcept { return Vector3(x, y, z); }

        // Static constants
        static const Vector4 ZERO;
        static const Vector4 ONE;
        static const Vector4 UNIT_X;
        static const Vector4 UNIT_Y;
        static const Vector4 UNIT_Z;
        static const Vector4 UNIT_W;
    };
}

// =============================================================================
// Matrix Types
// =============================================================================

export namespace Math {
    struct Matrix3 {
        float m[9]; // Column-major order

        // Constructors
        constexpr Matrix3() noexcept;
        constexpr Matrix3(float m00, float m01, float m02,
            float m10, float m11, float m12,
            float m20, float m21, float m22) noexcept;
        explicit constexpr Matrix3(float diagonal) noexcept;

        // Element access
        constexpr float& operator()(size_t row, size_t col) noexcept;
        constexpr const float& operator()(size_t row, size_t col) const noexcept;
        constexpr float* operator[](size_t row) noexcept;
        constexpr const float* operator[](size_t row) const noexcept;

        // Matrix operations
        constexpr Matrix3 operator+(const Matrix3& other) const noexcept;
        constexpr Matrix3 operator-(const Matrix3& other) const noexcept;
        constexpr Matrix3 operator*(const Matrix3& other) const noexcept;
        constexpr Matrix3 operator*(float scalar) const noexcept;
        constexpr Vector3 operator*(const Vector3& vector) const noexcept;

        constexpr Matrix3& operator+=(const Matrix3& other) noexcept;
        constexpr Matrix3& operator-=(const Matrix3& other) noexcept;
        constexpr Matrix3& operator*=(const Matrix3& other) noexcept;
        constexpr Matrix3& operator*=(float scalar) noexcept;

        // Comparison
        constexpr bool operator==(const Matrix3& other) const noexcept;
        constexpr bool operator!=(const Matrix3& other) const noexcept;

        // Static constructors
        static const Matrix3 ZERO;
        static const Matrix3 IDENTITY;
    };

    struct Matrix4 {
        float m[16]; // Column-major order

        // Constructors
        constexpr Matrix4() noexcept;
        constexpr Matrix4(float m00, float m01, float m02, float m03,
            float m10, float m11, float m12, float m13,
            float m20, float m21, float m22, float m23,
            float m30, float m31, float m32, float m33) noexcept;
        explicit constexpr Matrix4(float diagonal) noexcept;
        constexpr Matrix4(const Matrix3& mat3) noexcept;

        // Element access
        constexpr float& operator()(size_t row, size_t col) noexcept;
        constexpr const float& operator()(size_t row, size_t col) const noexcept;
        constexpr float* operator[](size_t row) noexcept;
        constexpr const float* operator[](size_t row) const noexcept;

        // Matrix operations
        constexpr Matrix4 operator+(const Matrix4& other) const noexcept;
        constexpr Matrix4 operator-(const Matrix4& other) const noexcept;
        constexpr Matrix4 operator*(const Matrix4& other) const noexcept;
        constexpr Matrix4 operator*(float scalar) const noexcept;
        constexpr Vector4 operator*(const Vector4& vector) const noexcept;
        constexpr Vector3 TransformPoint(const Vector3& point) const noexcept;
        constexpr Vector3 TransformVector(const Vector3& vector) const noexcept;

        constexpr Matrix4& operator+=(const Matrix4& other) noexcept;
        constexpr Matrix4& operator-=(const Matrix4& other) noexcept;
        constexpr Matrix4& operator*=(const Matrix4& other) noexcept;
        constexpr Matrix4& operator*=(float scalar) noexcept;

        // Comparison
        constexpr bool operator==(const Matrix4& other) const noexcept;
        constexpr bool operator!=(const Matrix4& other) const noexcept;

        // Static constructors
        static const Matrix4 ZERO;
        static const Matrix4 IDENTITY;
    };
}

// =============================================================================
// Quaternion
// =============================================================================

export namespace Math {
    struct Quaternion {
        float x, y, z, w;

        // Constructors
        constexpr Quaternion() noexcept : x(0), y(0), z(0), w(1) {}
        constexpr Quaternion(float x, float y, float z, float w) noexcept : x(x), y(y), z(z), w(w) {}
        Quaternion(const Vector3& axis, float angle) noexcept;
        Quaternion(const Vector3& eulerAngles) noexcept;

        // Operations
        constexpr Quaternion operator+(const Quaternion& other) const noexcept;
        constexpr Quaternion operator-(const Quaternion& other) const noexcept;
        constexpr Quaternion operator*(const Quaternion& other) const noexcept;
        constexpr Quaternion operator*(float scalar) const noexcept;
        constexpr Vector3 operator*(const Vector3& vector) const noexcept;

        constexpr Quaternion& operator+=(const Quaternion& other) noexcept;
        constexpr Quaternion& operator-=(const Quaternion& other) noexcept;
        constexpr Quaternion& operator*=(const Quaternion& other) noexcept;
        constexpr Quaternion& operator*=(float scalar) noexcept;

        constexpr bool operator==(const Quaternion& other) const noexcept;
        constexpr bool operator!=(const Quaternion& other) const noexcept;

        // Static constants
        static const Quaternion IDENTITY;
    };
}

// =============================================================================
// Color Types
// =============================================================================

export namespace Math {
    struct Color3 {
        float r, g, b;

        constexpr Color3() noexcept : r(0), g(0), b(0) {}
        constexpr Color3(float r, float g, float b) noexcept : r(r), g(g), b(b) {}
        explicit constexpr Color3(float intensity) noexcept : r(intensity), g(intensity), b(intensity) {}

        // Conversion from/to Vector3
        constexpr Color3(const Vector3& vec) noexcept : r(vec.x), g(vec.y), b(vec.z) {}
        constexpr operator Vector3() const noexcept { return Vector3(r, g, b); }

        // Operations
        constexpr Color3 operator+(const Color3& other) const noexcept;
        constexpr Color3 operator-(const Color3& other) const noexcept;
        constexpr Color3 operator*(const Color3& other) const noexcept;
        constexpr Color3 operator*(float scalar) const noexcept;

        // Static colors
        static const Color3 BLACK;
        static const Color3 WHITE;
        static const Color3 RED;
        static const Color3 GREEN;
        static const Color3 BLUE;
        static const Color3 YELLOW;
        static const Color3 MAGENTA;
        static const Color3 CYAN;
    };

    struct Color4 {
        float r, g, b, a;

        constexpr Color4() noexcept : r(0), g(0), b(0), a(1) {}
        constexpr Color4(float r, float g, float b, float a = 1.0f) noexcept : r(r), g(g), b(b), a(a) {}
        constexpr Color4(const Color3& rgb, float a = 1.0f) noexcept : r(rgb.r), g(rgb.g), b(rgb.b), a(a) {}
        explicit constexpr Color4(float intensity, float a = 1.0f) noexcept : r(intensity), g(intensity), b(intensity), a(a) {}

        // Conversion from/to Vector4
        constexpr Color4(const Vector4& vec) noexcept : r(vec.x), g(vec.y), b(vec.z), a(vec.w) {}
        constexpr operator Vector4() const noexcept { return Vector4(r, g, b, a); }

        // Operations
        constexpr Color4 operator+(const Color4& other) const noexcept;
        constexpr Color4 operator-(const Color4& other) const noexcept;
        constexpr Color4 operator*(const Color4& other) const noexcept;
        constexpr Color4 operator*(float scalar) const noexcept;

        // RGB extraction
        constexpr Color3 rgb() const noexcept { return Color3(r, g, b); }

        // Static colors
        static const Color4 BLACK;
        static const Color4 WHITE;
        static const Color4 RED;
        static const Color4 GREEN;
        static const Color4 BLUE;
        static const Color4 YELLOW;
        static const Color4 MAGENTA;
        static const Color4 CYAN;
        static const Color4 TRANSPARENT;
    };
}

// =============================================================================
// Geometric Primitives
// =============================================================================

export namespace Math {
    struct Ray {
        Vector3 origin;
        Vector3 direction;

        constexpr Ray() noexcept = default;
        constexpr Ray(const Vector3& origin, const Vector3& direction) noexcept
            : origin(origin), direction(direction) {
        }

        constexpr Vector3 GetPoint(float t) const noexcept;
    };

    struct Plane {
        Vector3 normal;
        float distance;

        constexpr Plane() noexcept : normal(Vector3::UNIT_Y), distance(0) {}
        constexpr Plane(const Vector3& normal, float distance) noexcept : normal(normal), distance(distance) {}
        Plane(const Vector3& normal, const Vector3& point) noexcept;
        Plane(const Vector3& p1, const Vector3& p2, const Vector3& p3) noexcept;

        constexpr float DistanceToPoint(const Vector3& point) const noexcept;
        constexpr Vector3 ClosestPoint(const Vector3& point) const noexcept;
    };

    struct AABB {
        Vector3 min;
        Vector3 max;

        constexpr AABB() noexcept : min(Vector3::ZERO), max(Vector3::ZERO) {}
        constexpr AABB(const Vector3& min, const Vector3& max) noexcept : min(min), max(max) {}
        constexpr AABB(const Vector3& center, float radius) noexcept;

        constexpr Vector3 Center() const noexcept;
        constexpr Vector3 Size() const noexcept;
        constexpr Vector3 Extents() const noexcept;
        constexpr float Volume() const noexcept;
        constexpr bool Contains(const Vector3& point) const noexcept;
        constexpr bool Intersects(const AABB& other) const noexcept;
        constexpr AABB Union(const AABB& other) const noexcept;
        constexpr AABB Intersection(const AABB& other) const noexcept;
        constexpr void Expand(const Vector3& point) noexcept;
        constexpr void Expand(float amount) noexcept;
    };

    struct OBB {
        Vector3 center;
        Vector3 extents;
        Matrix3 rotation;

        constexpr OBB() noexcept : center(Vector3::ZERO), extents(Vector3::ONE), rotation(Matrix3::IDENTITY) {}
        constexpr OBB(const Vector3& center, const Vector3& extents, const Matrix3& rotation) noexcept
            : center(center), extents(extents), rotation(rotation) {
        }

        constexpr bool Contains(const Vector3& point) const noexcept;
        constexpr bool Intersects(const OBB& other) const noexcept;
        constexpr AABB ToAABB() const noexcept;
    };

    struct Sphere {
        Vector3 center;
        float radius;

        constexpr Sphere() noexcept : center(Vector3::ZERO), radius(1.0f) {}
        constexpr Sphere(const Vector3& center, float radius) noexcept : center(center), radius(radius) {}

        constexpr bool Contains(const Vector3& point) const noexcept;
        constexpr bool Intersects(const Sphere& other) const noexcept;
        constexpr bool Intersects(const AABB& aabb) const noexcept;
        constexpr AABB ToAABB() const noexcept;
        constexpr float Volume() const noexcept;
        constexpr float SurfaceArea() const noexcept;
    };

    struct Triangle {
        Vector3 v0, v1, v2;

        constexpr Triangle() noexcept = default;
        constexpr Triangle(const Vector3& v0, const Vector3& v1, const Vector3& v2) noexcept
            : v0(v0), v1(v1), v2(v2) {
        }

        Vector3 Normal() const noexcept;
        constexpr Vector3 Center() const noexcept;
        float Area() const noexcept;
        constexpr bool Contains(const Vector3& point) const noexcept;
        constexpr Vector3 ClosestPoint(const Vector3& point) const noexcept;
    };

    struct Frustum {
        std::array<Plane, 6> planes; // Left, Right, Bottom, Top, Near, Far

        enum class PlaneIndex : size_t {
            Left = 0, Right = 1, Bottom = 2, Top = 3, Near = 4, Far = 5
        };

        constexpr Frustum() noexcept = default;
        constexpr Frustum(const Matrix4& viewProjectionMatrix) noexcept;

        constexpr bool Contains(const Vector3& point) const noexcept;
        constexpr bool Intersects(const Sphere& sphere) const noexcept;
        constexpr bool Intersects(const AABB& aabb) const noexcept;
    };
}

// =============================================================================
// Advanced Math - Curves and Splines
// =============================================================================

export namespace Math {
    template<typename T, size_t Degree>
    struct BezierCurve {
        std::array<T, Degree + 1> controlPoints;

        constexpr BezierCurve() noexcept = default;
        constexpr BezierCurve(const std::array<T, Degree + 1>& points) noexcept : controlPoints(points) {}

        constexpr T Evaluate(float t) const noexcept;
        constexpr T Derivative(float t) const noexcept;
        constexpr float Length(size_t subdivisions = 100) const noexcept;
    };

    // Common Bezier curve types
    using QuadraticBezier = BezierCurve<Vector3, 2>;
    using CubicBezier = BezierCurve<Vector3, 3>;
    using QuadraticBezier2D = BezierCurve<Vector2, 2>;
    using CubicBezier2D = BezierCurve<Vector2, 3>;

    template<typename T>
    struct CatmullRomSpline {
        std::vector<T> controlPoints;

        constexpr CatmullRomSpline() noexcept = default;
        constexpr CatmullRomSpline(const std::vector<T>& points) noexcept : controlPoints(points) {}

        constexpr T Evaluate(float t) const noexcept;
        constexpr T Derivative(float t) const noexcept;
        constexpr size_t GetSegmentCount() const noexcept;
        constexpr float GetTotalLength(size_t subdivisions = 100) const noexcept;
    };

    using CatmullRomSpline3D = CatmullRomSpline<Vector3>;
    using CatmullRomSpline2D = CatmullRomSpline<Vector2>;
}

// =============================================================================
// Transform Types
// =============================================================================

export namespace Math {
    struct Transform {
        Vector3 position;
        Quaternion rotation;
        Vector3 scale;

        constexpr Transform() noexcept
            : position(Vector3::ZERO), rotation(Quaternion::IDENTITY), scale(Vector3::ONE) {
        }
        constexpr Transform(const Vector3& position, const Quaternion& rotation = Quaternion::IDENTITY,
            const Vector3& scale = Vector3::ONE) noexcept
            : position(position), rotation(rotation), scale(scale) {
        }

        Matrix4 ToMatrix() const noexcept;
        constexpr Transform Inverse() const noexcept;
        constexpr Vector3 TransformPoint(const Vector3& point) const noexcept;
        constexpr Vector3 TransformVector(const Vector3& vector) const noexcept;
        constexpr Vector3 InverseTransformPoint(const Vector3& point) const noexcept;
        constexpr Vector3 InverseTransformVector(const Vector3& vector) const noexcept;

        constexpr Transform operator*(const Transform& other) const noexcept;
        constexpr Transform& operator*=(const Transform& other) noexcept;

        static const Transform IDENTITY;
    };

    struct Transform2D {
        Vector2 position;
        float rotation;
        Vector2 scale;

        constexpr Transform2D() noexcept
            : position(Vector2::ZERO), rotation(0.0f), scale(Vector2::ONE) {
        }
        constexpr Transform2D(const Vector2& position, float rotation = 0.0f,
            const Vector2& scale = Vector2::ONE) noexcept
            : position(position), rotation(rotation), scale(scale) {
        }

        Matrix3 ToMatrix() const noexcept;
        Transform2D Inverse() const noexcept;
        Vector2 TransformPoint(const Vector2& point) const noexcept;
        Vector2 TransformVector(const Vector2& vector) const noexcept;

        Transform2D operator*(const Transform2D& other) const noexcept;
        Transform2D& operator*=(const Transform2D& other) noexcept;

        static const Transform2D IDENTITY;
    };
}

// =============================================================================
// Mathematical Functions
// =============================================================================

export namespace Math {
    // Basic functions
    constexpr float Abs(float x) noexcept;
    constexpr float Sign(float x) noexcept;
    constexpr float Min(float a, float b) noexcept;
    constexpr float Max(float a, float b) noexcept;
    constexpr float Clamp(float value, float min, float max) noexcept;
    constexpr float Saturate(float value) noexcept;

    // Angle functions
    constexpr float ToRadians(float degrees) noexcept;
    constexpr float ToDegrees(float radians) noexcept;
    float WrapAngle(float angle) noexcept;
    float AngleDifference(float a, float b) noexcept;

    // Interpolation
    constexpr float Lerp(float a, float b, float t) noexcept;
    constexpr float InverseLerp(float a, float b, float value) noexcept;
    constexpr float SmoothStep(float edge0, float edge1, float x) noexcept;
    constexpr float SmootherStep(float edge0, float edge1, float x) noexcept;

    Vector2 Lerp(const Vector2& a, const Vector2& b, float t) noexcept;
    Vector3 Lerp(const Vector3& a, const Vector3& b, float t) noexcept;
    Vector4 Lerp(const Vector4& a, const Vector4& b, float t) noexcept;
    Quaternion Slerp(const Quaternion& a, const Quaternion& b, float t) noexcept;
    Quaternion Nlerp(const Quaternion& a, const Quaternion& b, float t) noexcept;

    // Easing functions
    float EaseInSine(float t) noexcept;
    float EaseOutSine(float t) noexcept;
    float EaseInOutSine(float t) noexcept;
    float EaseInQuad(float t) noexcept;
    float EaseOutQuad(float t) noexcept;
    float EaseInOutQuad(float t) noexcept;
    float EaseInCubic(float t) noexcept;
    float EaseOutCubic(float t) noexcept;
    float EaseInOutCubic(float t) noexcept;
    float EaseInQuart(float t) noexcept;
    float EaseOutQuart(float t) noexcept;
    float EaseInOutQuart(float t) noexcept;
    float EaseInExpo(float t) noexcept;
    float EaseOutExpo(float t) noexcept;
    float EaseInOutExpo(float t) noexcept;
    float EaseInBack(float t) noexcept;
    float EaseOutBack(float t) noexcept;
    float EaseInOutBack(float t) noexcept;
    float EaseInElastic(float t) noexcept;
    float EaseOutElastic(float t) noexcept;
    float EaseInOutElastic(float t) noexcept;
    float EaseInBounce(float t) noexcept;
    float EaseOutBounce(float t) noexcept;
    float EaseInOutBounce(float t) noexcept;
}

// =============================================================================
// Vector Operations
// =============================================================================

export namespace Math {
    // Vector2 operations
    constexpr float Dot(const Vector2& a, const Vector2& b) noexcept;
    constexpr float Cross(const Vector2& a, const Vector2& b) noexcept;
    float Length(const Vector2& v) noexcept;
    constexpr float LengthSquared(const Vector2& v) noexcept;
    Vector2 Normalize(const Vector2& v) noexcept;
    float Distance(const Vector2& a, const Vector2& b) noexcept;
    constexpr float DistanceSquared(const Vector2& a, const Vector2& b) noexcept;
    constexpr Vector2 Reflect(const Vector2& incident, const Vector2& normal) noexcept;
    constexpr Vector2 Project(const Vector2& a, const Vector2& b) noexcept;
    constexpr Vector2 Perpendicular(const Vector2& v) noexcept;
    float Angle(const Vector2& a, const Vector2& b) noexcept;

    // Vector3 operations
    constexpr float Dot(const Vector3& a, const Vector3& b) noexcept;
    constexpr Vector3 Cross(const Vector3& a, const Vector3& b) noexcept;
    float Length(const Vector3& v) noexcept;
    constexpr float LengthSquared(const Vector3& v) noexcept;
    Vector3 Normalize(const Vector3& v) noexcept;
    float Distance(const Vector3& a, const Vector3& b) noexcept;
    constexpr float DistanceSquared(const Vector3& a, const Vector3& b) noexcept;
    constexpr Vector3 Reflect(const Vector3& incident, const Vector3& normal) noexcept;
    constexpr Vector3 Refract(const Vector3& incident, const Vector3& normal, float ior) noexcept;
    constexpr Vector3 Project(const Vector3& a, const Vector3& b) noexcept;
    float Angle(const Vector3& a, const Vector3& b) noexcept;

    // Vector4 operations
    constexpr float Dot(const Vector4& a, const Vector4& b) noexcept;
    float Length(const Vector4& v) noexcept;
    constexpr float LengthSquared(const Vector4& v) noexcept;
    Vector4 Normalize(const Vector4& v) noexcept;
    float Distance(const Vector4& a, const Vector4& b) noexcept;
    constexpr float DistanceSquared(const Vector4& a, const Vector4& b) noexcept;
}

// =============================================================================
// Matrix Operations
// =============================================================================

export namespace Math {
    // Matrix3 operations
    constexpr float Determinant(const Matrix3& m) noexcept;
    constexpr Matrix3 Transpose(const Matrix3& m) noexcept;
    constexpr Matrix3 Inverse(const Matrix3& m) noexcept;
    constexpr Matrix3 Scale(const Vector2& scale) noexcept;
    Matrix3 Rotation(float angle) noexcept;
    constexpr Matrix3 Translation(const Vector2& translation) noexcept;

    // Matrix4 operations
    constexpr float Determinant(const Matrix4& m) noexcept;
    constexpr Matrix4 Transpose(const Matrix4& m) noexcept;
    constexpr Matrix4 Inverse(const Matrix4& m) noexcept;
    constexpr Matrix4 Scale(const Vector3& scale) noexcept;
    Matrix4 RotationX(float angle) noexcept;
    Matrix4 RotationY(float angle) noexcept;
    Matrix4 RotationZ(float angle) noexcept;
    Matrix4 Rotation(const Vector3& axis, float angle) noexcept;
    constexpr Matrix4 Translation(const Vector3& translation) noexcept;
    Matrix4 LookAt(const Vector3& eye, const Vector3& target, const Vector3& up) noexcept;

    // Projection matrices
    Matrix4 Perspective(float fovY, float aspect, float near, float far) noexcept;
    Matrix4 PerspectiveReversedZ(float fovY, float aspect, float near, float far) noexcept;
    constexpr Matrix4 Orthographic(float left, float right, float bottom, float top, float near, float far) noexcept;
    constexpr Matrix4 OrthographicReversedZ(float left, float right, float bottom, float top, float near, float far) noexcept;

    // Matrix decomposition
    bool Decompose(const Matrix4& matrix, Vector3& translation, Quaternion& rotation, Vector3& scale) noexcept;
    Matrix4 Compose(const Vector3& translation, const Quaternion& rotation, const Vector3& scale) noexcept;
}

// =============================================================================
// Quaternion Operations
// =============================================================================

export namespace Math {
    constexpr float Dot(const Quaternion& a, const Quaternion& b) noexcept;
    float Length(const Quaternion& q) noexcept;
    constexpr float LengthSquared(const Quaternion& q) noexcept;
    Quaternion Normalize(const Quaternion& q) noexcept;
    constexpr Quaternion Conjugate(const Quaternion& q) noexcept;
    constexpr Quaternion Inverse(const Quaternion& q) noexcept;
    Matrix4 ToMatrix(const Quaternion& q) noexcept;
    Matrix3 ToMatrix3(const Quaternion& q) noexcept;
    Vector3 ToEulerAngles(const Quaternion& q) noexcept;
    Quaternion FromEulerAngles(const Vector3& eulerAngles) noexcept;
    Quaternion FromAxisAngle(const Vector3& axis, float angle) noexcept;
    constexpr Quaternion FromRotationMatrix(const Matrix3& matrix) noexcept;
    Quaternion LookRotation(const Vector3& forward, const Vector3& up = Vector3::UP) noexcept;
    float Angle(const Quaternion& a, const Quaternion& b) noexcept;
}



// =============================================================================
// Additional Transform Utilities
// =============================================================================

export namespace Math {
    // Transform interpolation
    Transform LerpTransform(const Transform& a, const Transform& b, float t) noexcept;
    Transform2D LerpTransform2D(const Transform2D& a, const Transform2D& b, float t) noexcept;

    // Transform creation helpers
    Transform CreateTransform(const Vector3& position, const Vector3& eulerAngles, const Vector3& scale = Vector3::ONE) noexcept;
    Transform CreateLookAtTransform(const Vector3& position, const Vector3& target, const Vector3& up = Vector3::UP, const Vector3& scale = Vector3::ONE) noexcept;

    // Transform hierarchy operations
    Transform CombineTransforms(const Transform& parent, const Transform& child) noexcept;
    Transform GetRelativeTransform(const Transform& from, const Transform& to) noexcept;

    // Transform validation
    bool IsValidTransform(const Transform& transform) noexcept;
    bool IsValidTransform2D(const Transform2D& transform) noexcept;

    // Transform normalization
    Transform NormalizeTransform(const Transform& transform) noexcept;
    Transform2D NormalizeTransform2D(const Transform2D& transform) noexcept;

    // Transform bounding calculations
    AABB TransformAABB(const AABB& aabb, const Transform& transform) noexcept;
    Sphere TransformSphere(const Sphere& sphere, const Transform& transform) noexcept;

    // Transform comparison
    bool AreTransformsEqual(const Transform& a, const Transform& b, float epsilon = EPSILON) noexcept;
    bool AreTransforms2DEqual(const Transform2D& a, const Transform2D& b, float epsilon = EPSILON) noexcept;
}


// =============================================================================
// Noise Functions
// =============================================================================

export namespace Math {
    // Noise types
    enum class NoiseType {
        Perlin,
        Simplex,
        Ridged,
        Fractal,
        Cellular
    };

    // 1D Noise
    float PerlinNoise1D(float x) noexcept;
    float SimplexNoise1D(float x) noexcept;

    // 2D Noise
    float PerlinNoise2D(float x, float y) noexcept;
    float SimplexNoise2D(float x, float y) noexcept;
    float RidgedNoise2D(float x, float y) noexcept;
    float CellularNoise2D(float x, float y) noexcept;

    // 3D Noise
    float PerlinNoise3D(float x, float y, float z) noexcept;
    float SimplexNoise3D(float x, float y, float z) noexcept;
    float RidgedNoise3D(float x, float y, float z) noexcept;
    float CellularNoise3D(float x, float y, float z) noexcept;

    // Fractal noise (multiple octaves)
    float FractalNoise2D(float x, float y, int octaves = 4, float frequency = 1.0f,
        float amplitude = 1.0f, float lacunarity = 2.0f, float gain = 0.5f) noexcept;
    float FractalNoise3D(float x, float y, float z, int octaves = 4, float frequency = 1.0f,
        float amplitude = 1.0f, float lacunarity = 2.0f, float gain = 0.5f) noexcept;

    // Noise configuration
    struct NoiseConfig {
        NoiseType type = NoiseType::Perlin;
        int octaves = 4;
        float frequency = 1.0f;
        float amplitude = 1.0f;
        float lacunarity = 2.0f;
        float gain = 0.5f;
        int seed = 0;
    };

    float GenerateNoise2D(float x, float y, const NoiseConfig& config) noexcept;
    float GenerateNoise3D(float x, float y, float z, const NoiseConfig& config) noexcept;
}

// =============================================================================
// Physics Math Helpers
// =============================================================================

export namespace Math {
    // Inertia tensors
    Matrix3 SphereInertiaTensor(float mass, float radius) noexcept;
    Matrix3 BoxInertiaTensor(float mass, const Vector3& dimensions) noexcept;
    Matrix3 CylinderInertiaTensor(float mass, float radius, float height) noexcept;
    Matrix3 CapsuleInertiaTensor(float mass, float radius, float height) noexcept;

    // Physics calculations
    Vector3 CalculateAngularVelocity(const Quaternion& fromRotation, const Quaternion& toRotation, float deltaTime) noexcept;
    Vector3 CalculateCenterOfMass(const std::vector<Vector3>& positions, const std::vector<float>& masses) noexcept;

    // Collision detection helpers
    bool RayAABBIntersection(const Ray& ray, const AABB& aabb, float& tMin, float& tMax) noexcept;
    bool RaySphereIntersection(const Ray& ray, const Sphere& sphere, float& t1, float& t2) noexcept;
    bool RayTriangleIntersection(const Ray& ray, const Triangle& triangle, float& t, Vector2& uv) noexcept;
    bool SphereSphereIntersection(const Sphere& a, const Sphere& b) noexcept;
    bool SphereAABBIntersection(const Sphere& sphere, const AABB& aabb) noexcept;
    bool AABBAABBIntersection(const AABB& a, const AABB& b) noexcept;

    // Closest point calculations
    Vector3 ClosestPointOnRay(const Ray& ray, const Vector3& point) noexcept;
    Vector3 ClosestPointOnSegment(const Vector3& segmentStart, const Vector3& segmentEnd, const Vector3& point) noexcept;
    Vector3 ClosestPointOnTriangle(const Triangle& triangle, const Vector3& point) noexcept;
    Vector3 ClosestPointOnAABB(const AABB& aabb, const Vector3& point) noexcept;
    Vector3 ClosestPointOnSphere(const Sphere& sphere, const Vector3& point) noexcept;
}

// =============================================================================
// Random Number Generation
// =============================================================================

export namespace Math {
    class Random {
    public:
        Random() noexcept;
        explicit Random(uint32_t seed) noexcept;

        void SetSeed(uint32_t seed) noexcept;
        uint32_t NextUInt() noexcept;
        int32_t NextInt() noexcept;
        int32_t NextInt(int32_t min, int32_t max) noexcept;
        float NextFloat() noexcept;
        float NextFloat(float min, float max) noexcept;
        double NextDouble() noexcept;
        double NextDouble(double min, double max) noexcept;
        bool NextBool() noexcept;

        Vector2 NextVector2() noexcept;
        Vector2 NextVector2(const Vector2& min, const Vector2& max) noexcept;
        Vector3 NextVector3() noexcept;
        Vector3 NextVector3(const Vector3& min, const Vector3& max) noexcept;
        Vector3 NextUnitVector3() noexcept;
        Vector2 NextPointInCircle() noexcept;
        Vector3 NextPointInSphere() noexcept;
        Vector2 NextPointOnCircle() noexcept;
        Vector3 NextPointOnSphere() noexcept;

        Quaternion NextRotation() noexcept;
        Color3 NextColor3() noexcept;
        Color4 NextColor4() noexcept;

    private:
        uint32_t state_;
    };

    // Global random instance
    Random& GlobalRandom() noexcept;

    // Convenience functions using global random
    float RandomFloat() noexcept;
    float RandomFloat(float min, float max) noexcept;
    int32_t RandomInt(int32_t min, int32_t max) noexcept;
    bool RandomBool() noexcept;
    Vector3 RandomUnitVector3() noexcept;
    Vector3 RandomPointInSphere() noexcept;
    Vector3 RandomPointOnSphere() noexcept;
}

// =============================================================================
// Utility Functions
// =============================================================================

export namespace Math {
    // Floating point utilities
    constexpr bool IsNearlyEqual(float a, float b, float epsilon = EPSILON) noexcept;
    constexpr bool IsNearlyZero(float value, float epsilon = EPSILON) noexcept;
     bool IsFinite(float value) noexcept;
    bool IsInfinite(float value) noexcept;
    bool IsNaN(float value) noexcept;

    // Power of 2 utilities
    constexpr bool IsPowerOfTwo(uint32_t value) noexcept;
    constexpr uint32_t NextPowerOfTwo(uint32_t value) noexcept;
    constexpr uint32_t PreviousPowerOfTwo(uint32_t value) noexcept;

    // Bit manipulation
    constexpr uint32_t CountSetBits(uint32_t value) noexcept;
    constexpr uint32_t CountLeadingZeros(uint32_t value) noexcept;
    constexpr uint32_t CountTrailingZeros(uint32_t value) noexcept;

    // Hash functions
    uint32_t Hash(uint32_t value) noexcept;
    uint32_t Hash(const Vector2& v) noexcept;
    uint32_t Hash(const Vector3& v) noexcept;
    uint32_t Hash(const Vector4& v) noexcept;
    uint64_t Hash64(uint64_t value) noexcept;

    // Compression/encoding
    uint32_t PackColor(const Color4& color) noexcept;
    Color4 UnpackColor(uint32_t packed) noexcept;
    uint32_t PackNormal(const Vector3& normal) noexcept;
    Vector3 UnpackNormal(uint32_t packed) noexcept;

    // Coordinate system conversions
    Vector3 CartesianToSpherical(const Vector3& cartesian) noexcept;
    Vector3 SphericalToCartesian(const Vector3& spherical) noexcept;
    Vector3 CartesianToCylindrical(const Vector3& cartesian) noexcept;
    Vector3 CylindricalToCartesian(const Vector3& cylindrical) noexcept;

    // Frustum extraction
    void ExtractFrustumPlanes(const Matrix4& viewProjectionMatrix, std::array<Plane, 6>& planes) noexcept;
}

// =============================================================================
// SIMD Optimized Functions (Internal)
// =============================================================================

namespace Math::SIMD {
    // Internal SIMD implementations - not exported
    void VectorAdd(const float* a, const float* b, float* result, size_t count) noexcept;
    void VectorMultiply(const float* a, const float* b, float* result, size_t count) noexcept;
    void VectorScale(const float* a, float scalar, float* result, size_t count) noexcept;
    void MatrixMultiply4x4(const float* a, const float* b, float* result) noexcept;
    void VectorTransform(const float* vector, const float* matrix, float* result) noexcept;
}


// =============================================================================
// Curves and Splines Implementation
// =============================================================================

export namespace Math {
    // BezierCurve member function implementations
    template<typename T, size_t Degree>
    constexpr T BezierCurve<T, Degree>::Evaluate(float t) const noexcept {
        if constexpr (Degree == 0) {
            return controlPoints[0];
        }
        else if constexpr (Degree == 1) {
            return Lerp(controlPoints[0], controlPoints[1], t);
        }
        else if constexpr (Degree == 2) {
            const T p01 = Lerp(controlPoints[0], controlPoints[1], t);
            const T p12 = Lerp(controlPoints[1], controlPoints[2], t);
            return Lerp(p01, p12, t);
        }
        else if constexpr (Degree == 3) {
            const T p01 = Lerp(controlPoints[0], controlPoints[1], t);
            const T p12 = Lerp(controlPoints[1], controlPoints[2], t);
            const T p23 = Lerp(controlPoints[2], controlPoints[3], t);
            const T p012 = Lerp(p01, p12, t);
            const T p123 = Lerp(p12, p23, t);
            return Lerp(p012, p123, t);
        }
        else {
            // General case for higher-degree curves
            std::array<T, Degree + 1> temp = controlPoints;

            for (size_t j = 1; j <= Degree; ++j) {
                for (size_t i = 0; i <= Degree - j; ++i) {
                    temp[i] = Lerp(temp[i], temp[i + 1], t);
                }
            }

            return temp[0];
        }
    }

    template<typename T, size_t Degree>
    constexpr T BezierCurve<T, Degree>::Derivative(float t) const noexcept {
        if constexpr (Degree == 0) {
            return T{}; // Zero derivative for constant curve
        }
        else {
            std::array<T, Degree> derivativePoints;
            for (size_t i = 0; i < Degree; ++i) {
                derivativePoints[i] = (controlPoints[i + 1] - controlPoints[i]) * static_cast<float>(Degree);
            }

            BezierCurve<T, Degree - 1> derivativeCurve{ derivativePoints };
            return derivativeCurve.Evaluate(t);
        }
    }

    template<typename T, size_t Degree>
    constexpr float BezierCurve<T, Degree>::Length(size_t subdivisions) const noexcept {
        if (subdivisions == 0) subdivisions = 1;

        float totalLength = 0.0f;
        T previousPoint = Evaluate(0.0f);

        for (size_t i = 1; i <= subdivisions; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(subdivisions);
            const T currentPoint = Evaluate(t);
            totalLength += Distance(previousPoint, currentPoint);
            previousPoint = currentPoint;
        }

        return totalLength;
    }

    // CatmullRomSpline member function implementations
    template<typename T>
    constexpr T CatmullRomSpline<T>::Evaluate(float t) const noexcept {
        if (controlPoints.size() < 2) {
            return controlPoints.empty() ? T{} : controlPoints[0];
        }

        if (controlPoints.size() == 2) {
            return Lerp(controlPoints[0], controlPoints[1], t);
        }

        // Clamp t to valid range
        t = Clamp(t, 0.0f, 1.0f);

        // Scale t to segment range
        const float scaledT = t * static_cast<float>(controlPoints.size() - 1);
        const size_t segmentIndex = static_cast<size_t>(scaledT);
        const float localT = scaledT - static_cast<float>(segmentIndex);

        // Get control points for current segment
        const size_t i0 = (segmentIndex == 0) ? 0 : segmentIndex - 1;
        const size_t i1 = segmentIndex;
        const size_t i2 = Min(segmentIndex + 1, controlPoints.size() - 1);
        const size_t i3 = Min(segmentIndex + 2, controlPoints.size() - 1);

        const T& p0 = controlPoints[i0];
        const T& p1 = controlPoints[i1];
        const T& p2 = controlPoints[i2];
        const T& p3 = controlPoints[i3];

        // Catmull-Rom interpolation
        const float t2 = localT * localT;
        const float t3 = t2 * localT;

        return 0.5f * (
            (2.0f * p1) +
            (-p0 + p2) * localT +
            (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
            (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
            );
    }

    template<typename T>
    constexpr T CatmullRomSpline<T>::Derivative(float t) const noexcept {
        if (controlPoints.size() < 2) {
            return T{};
        }

        if (controlPoints.size() == 2) {
            return controlPoints[1] - controlPoints[0];
        }

        // Clamp t to valid range
        t = Clamp(t, 0.0f, 1.0f);

        // Scale t to segment range
        const float scaledT = t * static_cast<float>(controlPoints.size() - 1);
        const size_t segmentIndex = static_cast<size_t>(scaledT);
        const float localT = scaledT - static_cast<float>(segmentIndex);

        // Get control points for current segment
        const size_t i0 = (segmentIndex == 0) ? 0 : segmentIndex - 1;
        const size_t i1 = segmentIndex;
        const size_t i2 = Min(segmentIndex + 1, controlPoints.size() - 1);
        const size_t i3 = Min(segmentIndex + 2, controlPoints.size() - 1);

        const T& p0 = controlPoints[i0];
        const T& p1 = controlPoints[i1];
        const T& p2 = controlPoints[i2];
        const T& p3 = controlPoints[i3];

        // Derivative of Catmull-Rom interpolation
        const float t2 = localT * localT;

        return 0.5f * (
            (-p0 + p2) +
            (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * 2.0f * localT +
            (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * 3.0f * t2
            ) * static_cast<float>(controlPoints.size() - 1);
    }

    template<typename T>
    constexpr size_t CatmullRomSpline<T>::GetSegmentCount() const noexcept {
        return controlPoints.size() > 1 ? controlPoints.size() - 1 : 0;
    }

    template<typename T>
    constexpr float CatmullRomSpline<T>::GetTotalLength(size_t subdivisions) const noexcept {
        if (subdivisions == 0) subdivisions = 1;

        float totalLength = 0.0f;
        T previousPoint = Evaluate(0.0f);

        for (size_t i = 1; i <= subdivisions; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(subdivisions);
            const T currentPoint = Evaluate(t);
            totalLength += Distance(previousPoint, currentPoint);
            previousPoint = currentPoint;
        }

        return totalLength;
    }

}