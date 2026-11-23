#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

constexpr float PI  = 3.14159265358979323846f;
constexpr float TAU = 6.28318530717958647692f;
constexpr float DEG2RAD = PI / 180.0f;
constexpr float RAD2DEG = 180.0f / PI;

inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
inline float smoothstepf(float e0, float e1, float x) {
    float t = clampf((x - e0) / (e1 - e0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}
inline float expDecay(float a, float b, float lambda, float dt) {
    return lerpf(a, b, 1.0f - std::exp(-lambda * dt));
}

struct Vec2 {
    float x = 0, y = 0;
    Vec2() = default;
    Vec2(float x_, float y_) : x(x_), y(y_) {}
    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    float lenSq() const { return x * x + y * y; }
    float len() const { return std::sqrt(lenSq()); }
};

struct Vec3 {
    float x = 0, y = 0, z = 0;
    Vec3() = default;
    Vec3(float v) : x(v), y(v), z(v) {}
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator-() const { return {-x, -y, -z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vec3 operator*(const Vec3& o) const { return {x * o.x, y * o.y, z * o.z}; }
    Vec3 operator/(float s) const { float inv = 1.0f / s; return {x * inv, y * inv, z * inv}; }
    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
    float lenSq() const { return x * x + y * y + z * z; }
    float len() const { return std::sqrt(lenSq()); }
};

inline Vec3 operator*(float s, const Vec3& v) { return {v.x * s, v.y * s, v.z * s}; }
inline float dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline Vec3 normalize(const Vec3& v) {
    float l = v.len();
    return l > 1e-6f ? v / l : Vec3{0, 1, 0};
}
inline Vec3 lerp(const Vec3& a, const Vec3& b, float t) {
    return a + (b - a) * t;
}
inline Vec3 expDecay(const Vec3& a, const Vec3& b, float lambda, float dt) {
    return lerp(a, b, 1.0f - std::exp(-lambda * dt));
}

struct Vec4 {
    float x = 0, y = 0, z = 0, w = 1;
    Vec4() = default;
    Vec4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
    Vec4(const Vec3& v, float w_) : x(v.x), y(v.y), z(v.z), w(w_) {}
};

// Column-major 4x4 matrix (OpenGL native layout)
struct Mat4 {
    float m[16] = {};

    static Mat4 identity() {
        Mat4 r;
        r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
        return r;
    }

    static Mat4 translate(const Vec3& t) {
        Mat4 r = identity();
        r.m[12] = t.x; r.m[13] = t.y; r.m[14] = t.z;
        return r;
    }

    static Mat4 scale(const Vec3& s) {
        Mat4 r = identity();
        r.m[0] = s.x; r.m[5] = s.y; r.m[10] = s.z;
        return r;
    }

    static Mat4 rotateX(float a) {
        Mat4 r = identity();
        float c = std::cos(a), s = std::sin(a);
        r.m[5] = c; r.m[6] = s; r.m[9] = -s; r.m[10] = c;
        return r;
    }

    static Mat4 rotateY(float a) {
        Mat4 r = identity();
        float c = std::cos(a), s = std::sin(a);
        r.m[0] = c; r.m[2] = -s; r.m[8] = s; r.m[10] = c;
        return r;
    }

    static Mat4 rotateZ(float a) {
        Mat4 r = identity();
        float c = std::cos(a), s = std::sin(a);
        r.m[0] = c; r.m[1] = s; r.m[4] = -s; r.m[5] = c;
        return r;
    }

    static Mat4 fromBasis(const Vec3& right, const Vec3& up, const Vec3& forward, const Vec3& pos) {
        // Local -Z is forward in OpenGL convention, or local +Z?
        // Our models face -Z (forward = -Z, right = +X, up = +Y)
        Mat4 r = identity();
        r.m[0] = right.x;    r.m[1] = right.y;    r.m[2] = right.z;
        r.m[4] = up.x;       r.m[5] = up.y;       r.m[6] = up.z;
        r.m[8] = -forward.x; r.m[9] = -forward.y; r.m[10] = -forward.z;
        r.m[12] = pos.x;     r.m[13] = pos.y;     r.m[14] = pos.z;
        return r;
    }

    static Mat4 perspective(float fovRad, float aspect, float nearZ, float farZ) {
        Mat4 r;
        float f = 1.0f / std::tan(fovRad * 0.5f);
        r.m[0] = f / aspect;
        r.m[5] = f;
        r.m[10] = (farZ + nearZ) / (nearZ - farZ);
        r.m[11] = -1.0f;
        r.m[14] = (2.0f * farZ * nearZ) / (nearZ - farZ);
        return r;
    }

    static Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
        Vec3 f = normalize(center - eye);
        Vec3 s = normalize(cross(f, up));
        Vec3 u = cross(s, f);
        Mat4 r = identity();
        r.m[0] = s.x;  r.m[4] = s.y;  r.m[8]  = s.z;
        r.m[1] = u.x;  r.m[5] = u.y;  r.m[9]  = u.z;
        r.m[2] = -f.x; r.m[6] = -f.y; r.m[10] = -f.z;
        r.m[12] = -dot(s, eye);
        r.m[13] = -dot(u, eye);
        r.m[14] = dot(f, eye);
        return r;
    }

    Mat4 operator*(const Mat4& b) const {
        Mat4 r;
        for (int c = 0; c < 4; ++c) {
            for (int row = 0; row < 4; ++row) {
                r.m[c * 4 + row] =
                    m[0 * 4 + row] * b.m[c * 4 + 0] +
                    m[1 * 4 + row] * b.m[c * 4 + 1] +
                    m[2 * 4 + row] * b.m[c * 4 + 2] +
                    m[3 * 4 + row] * b.m[c * 4 + 3];
            }
        }
        return r;
    }

    Vec4 mulVec4(const Vec4& v) const {
        return {
            m[0]*v.x + m[4]*v.y + m[8]*v.z  + m[12]*v.w,
            m[1]*v.x + m[5]*v.y + m[9]*v.z  + m[13]*v.w,
            m[2]*v.x + m[6]*v.y + m[10]*v.z + m[14]*v.w,
            m[3]*v.x + m[7]*v.y + m[11]*v.z + m[15]*v.w
        };
    }

    Vec3 transformPoint(const Vec3& p) const {
        Vec4 r = mulVec4({p.x, p.y, p.z, 1.0f});
        return {r.x, r.y, r.z};
    }

    Vec3 transformDir(const Vec3& d) const {
        return {
            m[0]*d.x + m[4]*d.y + m[8]*d.z,
            m[1]*d.x + m[5]*d.y + m[9]*d.z,
            m[2]*d.x + m[6]*d.y + m[10]*d.z
        };
    }
};

struct Quat {
    float x = 0, y = 0, z = 0, w = 1;
    static Quat identity() { return {0, 0, 0, 1}; }
    static Quat fromAxisAngle(const Vec3& axis, float rad) {
        Vec3 a = normalize(axis);
        float half = rad * 0.5f;
        float s = std::sin(half);
        return {a.x * s, a.y * s, a.z * s, std::cos(half)};
    }
    Quat operator*(const Quat& b) const {
        return {
            w*b.x + x*b.w + y*b.z - z*b.y,
            w*b.y - x*b.z + y*b.w + z*b.x,
            w*b.z + x*b.y - y*b.x + z*b.w,
            w*b.w - x*b.x - y*b.y - z*b.z
        };
    }
    Quat normalized() const {
        float l = std::sqrt(x*x + y*y + z*z + w*w);
        if (l < 1e-6f) return identity();
        return {x/l, y/l, z/l, w/l};
    }
    Vec3 rotate(const Vec3& v) const {
        Vec3 qv{x, y, z};
        Vec3 t = cross(qv, v) * 2.0f;
        return v + t * w + cross(qv, t);
    }
    Vec3 forward() const { return rotate({0, 0, -1}); }
    Vec3 right() const   { return rotate({1, 0, 0}); }
    Vec3 up() const      { return rotate({0, 1, 0}); }

    Mat4 toMat4(const Vec3& pos = {0,0,0}) const {
        return Mat4::fromBasis(right(), up(), forward(), pos);
    }
};

inline Quat slerp(const Quat& a, const Quat& b_, float t) {
    Quat b = b_;
    float d = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
    if (d < 0.0f) { d = -d; b = {-b.x, -b.y, -b.z, -b.w}; }
    if (d > 0.9995f) {
        return Quat{lerpf(a.x,b.x,t), lerpf(a.y,b.y,t), lerpf(a.z,b.z,t), lerpf(a.w,b.w,t)}.normalized();
    }
    float theta = std::acos(clampf(d, -1.0f, 1.0f));
    float sinT = std::sin(theta);
    float w1 = std::sin((1.0f - t) * theta) / sinT;
    float w2 = std::sin(t * theta) / sinT;
    return {a.x*w1 + b.x*w2, a.y*w1 + b.y*w2, a.z*w1 + b.z*w2, a.w*w1 + b.w*w2};
}

struct Rng {
    uint32_t state = 0x1337BEEFu;
    explicit Rng(uint32_t seed = 0x1337BEEFu) : state(seed ? seed : 0x1337BEEFu) {}
    uint32_t nextU32() {
        uint32_t x = state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        return state = x;
    }
    float next01() { return (nextU32() & 0xFFFFFFu) / float(0x1000000u); }
    float range(float lo, float hi) { return lo + (hi - lo) * next01(); }
    int rangeInt(int lo, int hi) { return lo + int(nextU32() % uint32_t(hi - lo + 1)); }
    Vec3 unitSphere() {
        float u = range(-1.0f, 1.0f);
        float phi = range(0.0f, TAU);
        float r = std::sqrt(std::max(0.0f, 1.0f - u * u));
        return {r * std::cos(phi), u, r * std::sin(phi)};
    }
};

// Fast deterministic 2D value noise for procedural terrain/islands
inline float hash2D(int x, int z) {
    uint32_t h = uint32_t(x) * 374761393u + uint32_t(z) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return float((h ^ (h >> 16)) & 0xFFFFu) / 65535.0f;
}

inline float noise2D(float x, float z) {
    int xi = int(std::floor(x));
    int zi = int(std::floor(z));
    float xf = x - xi, zf = z - zi;
    float u = xf * xf * (3.0f - 2.0f * xf);
    float v = zf * zf * (3.0f - 2.0f * zf);
    float a = hash2D(xi, zi),     b = hash2D(xi + 1, zi);
    float c = hash2D(xi, zi + 1), d = hash2D(xi + 1, zi + 1);
    return lerpf(lerpf(a, b, u), lerpf(c, d, u), v);
}

inline float fbm2D(float x, float z, int octaves = 4) {
    float val = 0.0f, amp = 0.5f, freq = 1.0f;
    for (int i = 0; i < octaves; ++i) {
        val += amp * noise2D(x * freq, z * freq);
        freq *= 2.03f;
        amp *= 0.5f;
    }
    return val;
}
