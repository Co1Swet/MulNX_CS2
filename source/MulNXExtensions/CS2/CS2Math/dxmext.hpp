#pragma once

#include <string>
#include <DirectXMath.h>

// 扩展 DirectX 命名空间
namespace DirectX {
    // 自由函数扩展
    inline float Dot(const XMFLOAT3& a, const XMFLOAT3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    inline XMFLOAT3 Cross(const XMFLOAT3& a, const XMFLOAT3& b) {
        return XMFLOAT3(
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        );
    }

    inline float Length(const XMFLOAT3& v) {
        return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    }

    inline XMFLOAT3 Normalize(const XMFLOAT3& v) {
        float len = Length(v);
        if (len > 0.0f) {
            return XMFLOAT3(v.x / len, v.y / len, v.z / len);
        }
        return v;
    }

    // 运算符重载
    inline XMFLOAT3 operator+(const XMFLOAT3& a, const XMFLOAT3& b) {
        return XMFLOAT3(a.x + b.x, a.y + b.y, a.z + b.z);
    }

    inline XMFLOAT3 operator-(const XMFLOAT3& a, const XMFLOAT3& b) {
        return XMFLOAT3(a.x - b.x, a.y - b.y, a.z - b.z);
    }

    inline XMFLOAT3 operator*(const XMFLOAT3& v, const float scalar) {
        return XMFLOAT3(v.x * scalar, v.y * scalar, v.z * scalar);
    }

    inline XMFLOAT3 operator*(float scalar, const XMFLOAT3& v) {
        return v * scalar;
    }
    inline XMFLOAT3 operator*(XMFLOAT3& a, const XMFLOAT3& b) {
        return XMFLOAT3(a.x * b.x, a.y * b.y, a.z * b.z);
    }
    inline XMFLOAT3 operator/(const XMFLOAT3& v, const float scalar) {
        return XMFLOAT3(v.x / scalar, v.y / scalar, v.z / scalar);
    }
    inline XMFLOAT3& operator+=(XMFLOAT3& a, const XMFLOAT3& b) {
        a.x += b.x;
        a.y += b.y;
        a.z += b.z;
        return a;
    }
    inline XMFLOAT3& operator-=(XMFLOAT3& a, const XMFLOAT3& b) {
        a.x -= b.x;
        a.y -= b.y;
        a.z -= b.z;
        return a;
    }
    inline XMFLOAT3& operator*=(XMFLOAT3& v, const XMFLOAT3& b) {
        v.x *= b.x;
        v.y *= b.y;
        v.z *= b.z;
        return v;
    }
    inline XMFLOAT3& operator*=(XMFLOAT3& v, const float scalar) {
        v.x *= scalar;
        v.y *= scalar;
        v.z *= scalar;
        return v;
    }
    inline XMFLOAT3& operator/=(XMFLOAT3& v, const XMFLOAT3& b) {
        v.x /= b.x;
        v.y /= b.y;
        v.z /= b.z;
        return v;
    }
    inline XMFLOAT3& operator/=(XMFLOAT3& v, const float scalar) {
        v.x /= scalar;
        v.y /= scalar;
        v.z /= scalar;
        return v;
    }

    // 实用函数
    inline float Distance(const XMFLOAT3& a, const XMFLOAT3& b) {
        XMFLOAT3 diff = a - b;
        return Length(diff);
    }

    inline XMFLOAT3 Lerp(const XMFLOAT3& a, const XMFLOAT3& b, float t) {
        t = (t < 0.0f) ? 0.0f : (t > 1.0f) ? 1.0f : t;
        return a + (b - a) * t;
    }

    inline std::string to_string(const XMFLOAT3& v) {
        return "X: " + std::to_string(v.x) + "  Y: " + std::to_string(v.y) + "  Z: " + std::to_string(v.z);
    }
}