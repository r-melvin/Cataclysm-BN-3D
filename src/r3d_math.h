#pragma once
#ifndef CATA_SRC_R3D_MATH_H
#define CATA_SRC_R3D_MATH_H

#include <array>
#include <cmath>

// Small float vector/matrix helpers for the 3D renderer. Plain C++, no SDL, so the camera and
// picking code can be unit tested in every build.
//
// World space is right-handed: x east, y north, z up, one unit per tile. Tile coordinates have
// y growing southwards, so world y = -tile y (see r3d::camera for the conversion helpers).
// Matrices are column-major (m[column * 4 + row]) and map to clip space with y up and depth in
// [0, 1], which is what SDL_GPU expects.

namespace r3d
{

struct vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

inline auto operator+( const vec3 &a, const vec3 &b ) -> vec3
{ return { .x = a.x + b.x, .y = a.y + b.y, .z = a.z + b.z }; }
inline auto operator-( const vec3 &a, const vec3 &b ) -> vec3
{ return { .x = a.x - b.x, .y = a.y - b.y, .z = a.z - b.z }; }
inline auto operator*( const vec3 &a, float s ) -> vec3
{ return { .x = a.x * s, .y = a.y * s, .z = a.z * s }; }
inline auto dot( const vec3 &a, const vec3 &b ) -> float
{ return a.x * b.x + a.y * b.y + a.z * b.z; }
inline auto cross( const vec3 &a, const vec3 &b ) -> vec3
{ return { .x = a.y * b.z - a.z * b.y, .y = a.z * b.x - a.x * b.z, .z = a.x * b.y - a.y * b.x }; }
inline auto length( const vec3 &a ) -> float
{ return std::sqrt( dot( a, a ) ); }
inline auto normalize( const vec3 &a ) -> vec3
{
    const auto len = static_cast<float>( length( a ) );
    return len > 0.0f ? a * ( 1.0f / len ) : vec3{};
}

struct mat4 {
    std::array<float, 16> m{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

    auto at( int row, int col ) -> float & {
        return m[col * 4 + row];
    }
    auto at( int row, int col ) const -> float {
        return m[col * 4 + row];
    }
};

auto operator*( const mat4 &a, const mat4 &b ) -> mat4;

/// Transforms a point (w = 1) and returns the resulting clip-space w through *w_out if given.
auto transform_point( const mat4 &mat, const vec3 &p, float *w_out = nullptr ) -> vec3;

/// Right-handed view matrix looking from eye at target.
auto look_at( const vec3 &eye, const vec3 &target, const vec3 &up ) -> mat4;

/// Right-handed perspective projection with depth in [0, 1].
struct perspective_options {
    float fovy_rad;
    float aspect;
    float near_plane;
    float far_plane;
};

auto perspective( const perspective_options &options ) -> mat4;

/// Right-handed orthographic projection with depth in [0, 1]; half_height is in world units.
struct orthographic_options {
    float half_height;
    float aspect;
    float near_plane;
    float far_plane;
};

auto orthographic( const orthographic_options &options ) -> mat4;

/// General 4x4 inverse; returns false if the matrix is singular.
auto invert( const mat4 &in, mat4 &out ) -> bool;

} // namespace r3d

#endif // CATA_SRC_R3D_MATH_H
