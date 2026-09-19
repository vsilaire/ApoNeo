#pragma once

#include <cmath>
#include <array>
#include <string>

namespace ApoNeo::Core {

/// @brief Represents a 2D affine transformation:
///        x' = a*x + b*y + c
///        y' = d*x + e*y + f
struct Affine2D {
    double a = 1.0;
    double b = 0.0;
    double c = 0.0;
    double d = 0.0;
    double e = 1.0;
    double f = 0.0;

    constexpr Affine2D() noexcept = default;
    constexpr Affine2D(double a_, double b_, double c_, double d_, double e_, double f_) noexcept
        : a(a_), b(b_), c(c_), d(d_), e(e_), f(f_) {}

    /// @brief Create an identity transformation
    static constexpr Affine2D identity() noexcept {
        return Affine2D(1.0, 0.0, 0.0, 0.0, 1.0, 0.0);
    }

    /// @brief Create a translation matrix
    static constexpr Affine2D translation(double tx, double ty) noexcept {
        return Affine2D(1.0, 0.0, tx, 0.0, 1.0, ty);
    }

    /// @brief Create a uniform or non-uniform scaling matrix
    static constexpr Affine2D scaling(double sx, double sy) noexcept {
        return Affine2D(sx, 0.0, 0.0, 0.0, sy, 0.0);
    }

    /// @brief Create a rotation matrix (radians)
    static Affine2D rotation(double angle_rad) noexcept {
        const double cos_a = std::cos(angle_rad);
        const double sin_a = std::sin(angle_rad);
        return Affine2D(cos_a, -sin_a, 0.0, sin_a, cos_a, 0.0);
    }

    /// @brief Apply affine transform to a 2D point (x, y)
    constexpr void transform(double& x, double& y) const noexcept {
        const double nx = a * x + b * y + c;
        const double ny = d * x + e * y + f;
        x = nx;
        y = ny;
    }

    /// @brief Return determinant of the 2x2 linear portion
    constexpr double determinant() const noexcept {
        return a * e - b * d;
    }

    /// @brief Compute inverse affine transform if invertible
    Affine2D inverse() const noexcept {
        const double det = determinant();
        if (std::abs(det) < 1e-15) {
            return identity();
        }
        const double inv_det = 1.0 / det;
        return Affine2D(
             e * inv_det,
            -b * inv_det,
            (b * f - e * c) * inv_det,
            -d * inv_det,
             a * inv_det,
            (d * c - a * f) * inv_det
        );
    }

    /// @brief Compose this * other (applies other first, then this)
    constexpr Affine2D operator*(const Affine2D& o) const noexcept {
        return Affine2D(
            a * o.a + b * o.d,
            a * o.b + b * o.e,
            a * o.c + b * o.f + c,
            d * o.a + e * o.d,
            d * o.b + e * o.e,
            d * o.c + e * o.f + f
        );
    }

    /// @brief Equality operator
    constexpr bool operator==(const Affine2D& o) const noexcept {
        return a == o.a && b == o.b && c == o.c &&
               d == o.d && e == o.e && f == o.f;
    }
};

} // namespace ApoNeo::Core
