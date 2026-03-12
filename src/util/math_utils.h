#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <math.h>

#define PI 3.14159265358979323846f
#define DEG2RAD(d) ((d) * PI / 180.0f)
#define RAD2DEG(r) ((r) * 180.0f / PI)

typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    float x, y;
} Vec2;

// 4x4 column-major matrix
typedef struct {
    float m[16];
} Mat4;

// Vec3 operations
static inline Vec3 vec3(float x, float y, float z) {
    return (Vec3){x, y, z};
}

static inline Vec3 vec3_add(Vec3 a, Vec3 b) {
    return (Vec3){a.x + b.x, a.y + b.y, a.z + b.z};
}

static inline Vec3 vec3_sub(Vec3 a, Vec3 b) {
    return (Vec3){a.x - b.x, a.y - b.y, a.z - b.z};
}

static inline Vec3 vec3_scale(Vec3 v, float s) {
    return (Vec3){v.x * s, v.y * s, v.z * s};
}

static inline float vec3_dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline Vec3 vec3_cross(Vec3 a, Vec3 b) {
    return (Vec3){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

static inline float vec3_length(Vec3 v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static inline float vec3_length_sq(Vec3 v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

static inline Vec3 vec3_normalize(Vec3 v) {
    float len = vec3_length(v);
    if (len > 0.0001f) {
        float inv = 1.0f / len;
        return (Vec3){v.x * inv, v.y * inv, v.z * inv};
    }
    return (Vec3){0, 0, 0};
}

static inline Vec3 vec3_lerp(Vec3 a, Vec3 b, float t) {
    return (Vec3){
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    };
}

static inline float clampf(float val, float min, float max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

// Mat4 operations
static inline Mat4 mat4_identity(void) {
    Mat4 m = {0};
    m.m[0] = 1; m.m[5] = 1; m.m[10] = 1; m.m[15] = 1;
    return m;
}

static inline Mat4 mat4_perspective(float fov_deg, float aspect, float near_plane, float far_plane) {
    Mat4 m = {0};
    float f = 1.0f / tanf(DEG2RAD(fov_deg) * 0.5f);
    m.m[0] = f / aspect;
    m.m[5] = f;
    m.m[10] = (far_plane + near_plane) / (near_plane - far_plane);
    m.m[11] = -1.0f;
    m.m[14] = (2.0f * far_plane * near_plane) / (near_plane - far_plane);
    return m;
}

static inline Mat4 mat4_look_at(Vec3 eye, Vec3 center, Vec3 up) {
    Vec3 f = vec3_normalize(vec3_sub(center, eye));
    Vec3 s = vec3_normalize(vec3_cross(f, up));
    Vec3 u = vec3_cross(s, f);

    Mat4 m = mat4_identity();
    m.m[0] = s.x;  m.m[4] = s.y;  m.m[8]  = s.z;
    m.m[1] = u.x;  m.m[5] = u.y;  m.m[9]  = u.z;
    m.m[2] = -f.x; m.m[6] = -f.y; m.m[10] = -f.z;
    m.m[12] = -vec3_dot(s, eye);
    m.m[13] = -vec3_dot(u, eye);
    m.m[14] = vec3_dot(f, eye);
    return m;
}

static inline Mat4 mat4_multiply(Mat4 a, Mat4 b) {
    Mat4 result = {0};
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            float sum = 0;
            for (int k = 0; k < 4; k++) {
                sum += a.m[k * 4 + row] * b.m[col * 4 + k];
            }
            result.m[col * 4 + row] = sum;
        }
    }
    return result;
}

static inline Mat4 mat4_translate(Vec3 t) {
    Mat4 m = mat4_identity();
    m.m[12] = t.x;
    m.m[13] = t.y;
    m.m[14] = t.z;
    return m;
}

static inline Mat4 mat4_scale(Vec3 s) {
    Mat4 m = {0};
    m.m[0] = s.x;
    m.m[5] = s.y;
    m.m[10] = s.z;
    m.m[15] = 1.0f;
    return m;
}

static inline Mat4 mat4_rotate_y(float angle_rad) {
    Mat4 m = mat4_identity();
    float c = cosf(angle_rad);
    float s = sinf(angle_rad);
    m.m[0] = c;  m.m[8] = s;
    m.m[2] = -s; m.m[10] = c;
    return m;
}

// AABB for collision detection
typedef struct {
    Vec3 min;
    Vec3 max;
} AABB;

static inline int aabb_intersect(AABB a, AABB b) {
    return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
           (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
           (a.min.z <= b.max.z && a.max.z >= b.min.z);
}

static inline int aabb_contains_point(AABB box, Vec3 p) {
    return (p.x >= box.min.x && p.x <= box.max.x) &&
           (p.y >= box.min.y && p.y <= box.max.y) &&
           (p.z >= box.min.z && p.z <= box.max.z);
}

// Ray for hitscan
typedef struct {
    Vec3 origin;
    Vec3 direction;
} Ray;

// Returns t parameter of intersection, -1 if no hit
static inline float ray_aabb_intersect(Ray ray, AABB box) {
    float tmin = -1e30f, tmax = 1e30f;

    if (fabsf(ray.direction.x) > 0.0001f) {
        float t1 = (box.min.x - ray.origin.x) / ray.direction.x;
        float t2 = (box.max.x - ray.origin.x) / ray.direction.x;
        if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
        if (t1 > tmin) tmin = t1;
        if (t2 < tmax) tmax = t2;
    } else if (ray.origin.x < box.min.x || ray.origin.x > box.max.x) {
        return -1.0f;
    }

    if (fabsf(ray.direction.y) > 0.0001f) {
        float t1 = (box.min.y - ray.origin.y) / ray.direction.y;
        float t2 = (box.max.y - ray.origin.y) / ray.direction.y;
        if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
        if (t1 > tmin) tmin = t1;
        if (t2 < tmax) tmax = t2;
    } else if (ray.origin.y < box.min.y || ray.origin.y > box.max.y) {
        return -1.0f;
    }

    if (fabsf(ray.direction.z) > 0.0001f) {
        float t1 = (box.min.z - ray.origin.z) / ray.direction.z;
        float t2 = (box.max.z - ray.origin.z) / ray.direction.z;
        if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
        if (t1 > tmin) tmin = t1;
        if (t2 < tmax) tmax = t2;
    } else if (ray.origin.z < box.min.z || ray.origin.z > box.max.z) {
        return -1.0f;
    }

    if (tmin > tmax || tmax < 0) return -1.0f;
    return tmin >= 0 ? tmin : tmax;
}

#endif // MATH_UTILS_H
