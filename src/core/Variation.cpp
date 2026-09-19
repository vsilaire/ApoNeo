#include "Variation.hpp"
#include <algorithm>
#include <cmath>

namespace ApoNeo::Core {

namespace {

constexpr double PI = 3.14159265358979323846;
constexpr double PI2 = PI * 2.0;
constexpr double INV_PI = 1.0 / PI;

// 1. Linear
void var_linear(const VariationContext& ctx, double weight, const double*, double& ox, double& oy, double& oz) {
    ox += weight * ctx.x;
    oy += weight * ctx.y;
    oz += weight * ctx.z;
}

// 2. Sinusoidal
void var_sinusoidal(const VariationContext& ctx, double weight, const double*, double& ox, double& oy, double& oz) {
    ox += weight * std::sin(ctx.x);
    oy += weight * std::sin(ctx.y);
    oz += weight * std::sin(ctx.z);
}

// 3. Spherical
void var_spherical(const VariationContext& ctx, double weight, const double*, double& ox, double& oy, double& oz) {
    double inv_r2 = (ctx.r2 > 1e-12) ? (1.0 / ctx.r2) : 1.0;
    ox += weight * ctx.x * inv_r2;
    oy += weight * ctx.y * inv_r2;
    oz += weight * ctx.z * inv_r2;
}

// 4. Swirl
void var_swirl(const VariationContext& ctx, double weight, const double*, double& ox, double& oy, double& oz) {
    double sin_r2 = std::sin(ctx.r2);
    double cos_r2 = std::cos(ctx.r2);
    ox += weight * (ctx.x * sin_r2 - ctx.y * cos_r2);
    oy += weight * (ctx.x * cos_r2 + ctx.y * sin_r2);
    oz += weight * ctx.z;
}

// 5. Horseshoe
void var_horseshoe(const VariationContext& ctx, double weight, const double*, double& ox, double& oy, double& oz) {
    double inv_r = (ctx.r > 1e-12) ? (1.0 / ctx.r) : 1.0;
    ox += weight * (ctx.x - ctx.y) * (ctx.x + ctx.y) * inv_r;
    oy += weight * 2.0 * ctx.x * ctx.y * inv_r;
    oz += weight * ctx.z;
}

// 6. Polar
void var_polar(const VariationContext& ctx, double weight, const double*, double& ox, double& oy, double& oz) {
    ox += weight * (ctx.theta * INV_PI);
    oy += weight * (ctx.r - 1.0);
    oz += weight * ctx.z;
}

// 7. Handkerchief
void var_handkerchief(const VariationContext& ctx, double weight, const double*, double& ox, double& oy, double& oz) {
    ox += weight * ctx.r * std::sin(ctx.theta + ctx.r);
    oy += weight * ctx.r * std::cos(ctx.theta - ctx.r);
    oz += weight * ctx.z;
}

// 8. Heart
void var_heart(const VariationContext& ctx, double weight, const double*, double& ox, double& oy, double& oz) {
    ox += weight * ctx.r * std::sin(ctx.theta * ctx.r);
    oy += weight * (-ctx.r * std::cos(ctx.theta * ctx.r));
    oz += weight * ctx.z;
}

// 9. Disc
void var_disc(const VariationContext& ctx, double weight, const double*, double& ox, double& oy, double& oz) {
    double factor = ctx.theta * INV_PI;
    double pi_r = PI * ctx.r;
    ox += weight * factor * std::sin(pi_r);
    oy += weight * factor * std::cos(pi_r);
    oz += weight * ctx.z;
}

// 10. Spiral
void var_spiral(const VariationContext& ctx, double weight, const double*, double& ox, double& oy, double& oz) {
    double inv_r = (ctx.r > 1e-12) ? (1.0 / ctx.r) : 1.0;
    ox += weight * (std::cos(ctx.theta) + std::sin(ctx.r)) * inv_r;
    oy += weight * (std::sin(ctx.theta) - std::cos(ctx.r)) * inv_r;
    oz += weight * ctx.z;
}

// 11. Hyperbolic
void var_hyperbolic(const VariationContext& ctx, double weight, const double*, double& ox, double& oy, double& oz) {
    double inv_r = (ctx.r > 1e-12) ? (1.0 / ctx.r) : 1.0;
    ox += weight * std::sin(ctx.theta) * inv_r;
    oy += weight * ctx.r * std::cos(ctx.theta);
    oz += weight * ctx.z;
}

// 12. Diamond
void var_diamond(const VariationContext& ctx, double weight, const double*, double& ox, double& oy, double& oz) {
    ox += weight * std::sin(ctx.theta) * std::cos(ctx.r);
    oy += weight * std::cos(ctx.theta) * std::sin(ctx.r);
    oz += weight * ctx.z;
}

// 13. Ex
void var_ex(const VariationContext& ctx, double weight, const double*, double& ox, double& oy, double& oz) {
    double p0 = std::sin(ctx.theta + ctx.r);
    double p1 = std::cos(ctx.theta - ctx.r);
    double p0_3 = p0 * p0 * p0;
    double p1_3 = p1 * p1 * p1;
    ox += weight * ctx.r * (p0_3 + p1_3);
    oy += weight * ctx.r * (p0_3 - p1_3);
    oz += weight * ctx.z;
}

// 14. Julia
void var_julia(const VariationContext& ctx, double weight, const double*, double& ox, double& oy, double& oz) {
    double sqrt_r = std::sqrt(ctx.r);
    double omega = (ctx.rand_val1 < 0.5) ? 0.0 : PI;
    double half_theta = 0.5 * ctx.theta + omega;
    ox += weight * sqrt_r * std::cos(half_theta);
    oy += weight * sqrt_r * std::sin(half_theta);
    oz += weight * ctx.z;
}

// 15. Julian (Parametric: julian_power, julian_dist)
void var_julian(const VariationContext& ctx, double weight, const double* params, double& ox, double& oy, double& oz) {
    double power = params ? params[0] : 2.0;
    double dist = params ? params[1] : 1.0;
    int p_int = std::max(1, static_cast<int>(std::abs(std::round(power))));

    double p_abs = static_cast<double>(p_int);
    double rnd_k = std::floor(ctx.rand_val1 * p_abs);
    double alpha = (ctx.theta + PI2 * rnd_k) / power;
    double r_pow = (ctx.r > 1e-12) ? std::pow(ctx.r, dist / power) : 0.0;

    ox += weight * r_pow * std::cos(alpha);
    oy += weight * r_pow * std::sin(alpha);
    oz += weight * ctx.z;
}

// 16. Julia3D (Parametric: julia3d_power, julia3d_dist)
void var_julia3d(const VariationContext& ctx, double weight, const double* params, double& ox, double& oy, double& oz) {
    double power = params ? params[0] : 2.0;
    double dist = params ? params[1] : 1.0;
    int p_int = std::max(1, static_cast<int>(std::abs(std::round(power))));

    double p_abs = static_cast<double>(p_int);
    double rnd_k = std::floor(ctx.rand_val1 * p_abs);
    double alpha = (ctx.theta + PI2 * rnd_k) / power;
    double rad3d = std::sqrt(ctx.r2 + ctx.z * ctx.z);
    double r_pow = (rad3d > 1e-12) ? std::pow(rad3d, dist / power) : 0.0;
    double phi_scaled = ctx.phi / power;

    ox += weight * r_pow * std::cos(phi_scaled) * std::cos(alpha);
    oy += weight * r_pow * std::cos(phi_scaled) * std::sin(alpha);
    oz += weight * r_pow * std::sin(phi_scaled);
}

// 17. Bent
void var_bent(const VariationContext& ctx, double weight, const double*, double& ox, double& oy, double& oz) {
    double nx = (ctx.x >= 0.0) ? ctx.x : 2.0 * ctx.x;
    double ny = (ctx.y >= 0.0) ? ctx.y : 0.5 * ctx.y;
    ox += weight * nx;
    oy += weight * ny;
    oz += weight * ctx.z;
}

// 18. Waves (Parametric: waves_b, waves_c, waves_e, waves_f)
void var_waves(const VariationContext& ctx, double weight, const double* params, double& ox, double& oy, double& oz) {
    double b = params ? params[0] : 1.0;
    double c = params ? params[1] : 1.0;
    double e = params ? params[2] : 1.0;
    double f = params ? params[3] : 1.0;
    double c2 = (std::abs(c) > 1e-6) ? (c * c) : 1.0;
    double f2 = (std::abs(f) > 1e-6) ? (f * f) : 1.0;

    ox += weight * (ctx.x + b * std::sin(ctx.y / c2));
    oy += weight * (ctx.y + e * std::sin(ctx.x / f2));
    oz += weight * ctx.z;
}

// 19. Fisheye
void var_fisheye(const VariationContext& ctx, double weight, const double*, double& ox, double& oy, double& oz) {
    double factor = 2.0 / (ctx.r + 1.0);
    ox += weight * factor * ctx.y;
    oy += weight * factor * ctx.x;
    oz += weight * ctx.z;
}

// 20. Popcorn (Parametric: popcorn_c, popcorn_f)
void var_popcorn(const VariationContext& ctx, double weight, const double* params, double& ox, double& oy, double& oz) {
    double c = params ? params[0] : 0.1;
    double f = params ? params[1] : 0.1;
    ox += weight * (ctx.x + c * std::sin(std::tan(3.0 * ctx.y)));
    oy += weight * (ctx.y + f * std::sin(std::tan(3.0 * ctx.x)));
    oz += weight * ctx.z;
}

// 21. Exponential
void var_exponential(const VariationContext& ctx, double weight, const double*, double& ox, double& oy, double& oz) {
    double exp_x = std::exp(ctx.x - 1.0);
    ox += weight * exp_x * std::cos(PI * ctx.y);
    oy += weight * exp_x * std::sin(PI * ctx.y);
    oz += weight * ctx.z;
}

// 22. Power
void var_power(const VariationContext& ctx, double weight, const double*, double& ox, double& oy, double& oz) {
    double r_sin = (ctx.r > 1e-12) ? std::pow(ctx.r, std::sin(ctx.theta)) : 0.0;
    ox += weight * r_sin * std::cos(ctx.theta);
    oy += weight * r_sin * std::sin(ctx.theta);
    oz += weight * ctx.z;
}

// 23. Cosine
void var_cosine(const VariationContext& ctx, double weight, const double*, double& ox, double& oy, double& oz) {
    ox += weight * std::cos(PI * ctx.x) * std::cosh(ctx.y);
    oy += weight * (-std::sin(PI * ctx.x) * std::sinh(ctx.y));
    oz += weight * ctx.z;
}

// 24. Rings (Parametric: rings_c)
void var_rings(const VariationContext& ctx, double weight, const double* params, double& ox, double& oy, double& oz) {
    double c = params ? params[0] : 0.5;
    double c2 = c * c;
    double val = (c2 > 1e-6) ? std::fmod(ctx.r + c2, 2.0 * c2) - c2 + ctx.r * (1.0 - c2) : ctx.r;
    ox += weight * val * std::cos(ctx.theta);
    oy += weight * val * std::sin(ctx.theta);
    oz += weight * ctx.z;
}

// 25. Rings2 (Parametric: rings2_val)
void var_rings2(const VariationContext& ctx, double weight, const double* params, double& ox, double& oy, double& oz) {
    double val = params ? params[0] : 0.5;
    double val2 = val * val;
    double p = (val2 > 1e-6) ? (ctx.r - 2.0 * val2 * std::floor((ctx.r + val2) / (2.0 * val2))) : ctx.r;
    ox += weight * p * std::sin(ctx.theta);
    oy += weight * p * std::cos(ctx.theta);
    oz += weight * ctx.z;
}

// 26. Blob (Parametric: blob_low, blob_high, blob_waves)
void var_blob(const VariationContext& ctx, double weight, const double* params, double& ox, double& oy, double& oz) {
    double low = params ? params[0] : 0.2;
    double high = params ? params[1] : 0.8;
    double waves = params ? params[2] : 4.0;

    double factor = ctx.r * (low + 0.5 * (high - low) * (1.0 + std::sin(waves * ctx.theta)));
    ox += weight * factor * std::cos(ctx.theta);
    oy += weight * factor * std::sin(ctx.theta);
    oz += weight * ctx.z;
}

// 27. PDJ (Parametric: pdj_a, pdj_b, pdj_c, pdj_d)
void var_pdj(const VariationContext& ctx, double weight, const double* params, double& ox, double& oy, double& oz) {
    double a = params ? params[0] : 1.0;
    double b = params ? params[1] : 1.0;
    double c = params ? params[2] : 1.0;
    double d = params ? params[3] : 1.0;

    ox += weight * (std::sin(a * ctx.y) - std::cos(b * ctx.x));
    oy += weight * (std::sin(c * ctx.x) - std::cos(d * ctx.y));
    oz += weight * ctx.z;
}

// 28. Perspective (Parametric: perspective_angle, perspective_dist)
void var_perspective(const VariationContext& ctx, double weight, const double* params, double& ox, double& oy, double& oz) {
    double angle = params ? params[0] : 0.5;
    double dist = params ? params[1] : 1.0;
    double denom = dist - ctx.y * std::sin(angle);
    double factor = (std::abs(denom) > 1e-6) ? (dist / denom) : 1.0;

    ox += weight * factor * ctx.x;
    oy += weight * factor * ctx.y * std::cos(angle);
    oz += weight * ctx.z;
}

// 29. Curl (Parametric: curl_c1, curl_c2)
void var_curl(const VariationContext& ctx, double weight, const double* params, double& ox, double& oy, double& oz) {
    double c1 = params ? params[0] : 0.1;
    double c2 = params ? params[1] : 0.1;

    double re = 1.0 + c1 * ctx.x + c2 * (ctx.x * ctx.x - ctx.y * ctx.y);
    double im = c1 * ctx.y + 2.0 * c2 * ctx.x * ctx.y;
    double denom = (re * re + im * im > 1e-12) ? (1.0 / (re * re + im * im)) : 1.0;

    ox += weight * (ctx.x * re + ctx.y * im) * denom;
    oy += weight * (ctx.y * re - ctx.x * im) * denom;
    oz += weight * ctx.z;
}

// 30. Pie (Parametric: pie_slices, pie_rotation)
void var_pie(const VariationContext& ctx, double weight, const double* params, double& ox, double& oy, double& oz) {
    double slices = params ? params[0] : 6.0;
    double rotation = params ? params[1] : 0.0;
    double slice_angle = PI2 / std::max(1.0, slices);

    double rnd_slice = std::floor(ctx.rand_val1 * slices);
    double ang = rotation + slice_angle * (rnd_slice + ctx.rand_val2);

    ox += weight * ctx.r * std::cos(ang);
    oy += weight * ctx.r * std::sin(ang);
    oz += weight * ctx.z;
}

// 31. Bubble
void var_bubble(const VariationContext& ctx, double weight, const double*, double& ox, double& oy, double& oz) {
    double factor = 4.0 / (ctx.r2 + 4.0);
    ox += weight * factor * ctx.x;
    oy += weight * factor * ctx.y;
    oz += weight * factor * ctx.z;
}

// 32. Cylinder
void var_cylinder(const VariationContext& ctx, double weight, const double*, double& ox, double& oy, double& oz) {
    ox += weight * std::sin(ctx.x);
    oy += weight * ctx.y;
    oz += weight * ctx.z;
}

// 33. Gaussian / Blur
void var_gaussian(const VariationContext& ctx, double weight, const double*, double& ox, double& oy, double& oz) {
    double u1 = std::max(ctx.rand_val1, 1e-10);
    double u2 = ctx.rand_val2;
    double rad = std::sqrt(-2.0 * std::log(u1));
    double ang = PI2 * u2;
    ox += weight * (rad * std::cos(ang) - 2.0);
    oy += weight * (rad * std::sin(ang) - 2.0);
    oz += weight * ctx.z;
}

// 34. Eyefish
void var_eyefish(const VariationContext& ctx, double weight, const double*, double& ox, double& oy, double& oz) {
    double factor = 2.0 / (ctx.r + 1.0);
    ox += weight * factor * ctx.x;
    oy += weight * factor * ctx.y;
    oz += weight * ctx.z;
}

} // namespace

VariationRegistry& VariationRegistry::instance() {
    static VariationRegistry reg;
    return reg;
}

VariationRegistry::VariationRegistry() {
    // 2D Classic
    register_variation("linear", 0, "Classic", {}, var_linear);
    register_variation("sinusoidal", 1, "Classic", {}, var_sinusoidal);
    register_variation("spherical", 2, "Classic", {}, var_spherical);
    register_variation("swirl", 3, "Classic", {}, var_swirl);
    register_variation("horseshoe", 4, "Classic", {}, var_horseshoe);
    register_variation("polar", 5, "Classic", {}, var_polar);
    register_variation("handkerchief", 6, "Classic", {}, var_handkerchief);
    register_variation("heart", 7, "Classic", {}, var_heart);
    register_variation("disc", 8, "Classic", {}, var_disc);
    register_variation("spiral", 9, "Classic", {}, var_spiral);
    register_variation("hyperbolic", 10, "Classic", {}, var_hyperbolic);
    register_variation("diamond", 11, "Classic", {}, var_diamond);
    register_variation("ex", 12, "Classic", {}, var_ex);
    register_variation("julia", 13, "Complex", {}, var_julia);
    register_variation("bent", 14, "Classic", {}, var_bent);
    register_variation("fisheye", 16, "Optical", {}, var_fisheye);
    register_variation("exponential", 18, "Classic", {}, var_exponential);
    register_variation("power", 19, "Classic", {}, var_power);
    register_variation("cosine", 20, "Classic", {}, var_cosine);
    register_variation("bubble", 22, "Optical", {}, var_bubble);
    register_variation("cylinder", 23, "3D", {}, var_cylinder);
    register_variation("gaussian", 24, "Blur/Noise", {}, var_gaussian);
    register_variation("eyefish", 25, "Optical", {}, var_eyefish);

    // Parametric Variations
    register_variation("julian", 15, "Complex", {
        {"julian_power", 2.0, 1.0, 100.0, "Root power exponent"},
        {"julian_dist", 1.0, -10.0, 10.0, "Radial scaling distance"}
    }, var_julian);

    register_variation("julia3d", 26, "3D", {
        {"julia3d_power", 2.0, 1.0, 100.0, "3D Root power exponent"},
        {"julia3d_dist", 1.0, -10.0, 10.0, "3D Radial scaling distance"}
    }, var_julia3d);

    register_variation("waves", 17, "Periodic", {
        {"waves_b", 1.0, -10.0, 10.0, "X amplitude factor"},
        {"waves_c", 1.0, 0.01, 10.0, "X frequency divisor"},
        {"waves_e", 1.0, -10.0, 10.0, "Y amplitude factor"},
        {"waves_f", 1.0, 0.01, 10.0, "Y frequency divisor"}
    }, var_waves);

    register_variation("popcorn", 21, "Periodic", {
        {"popcorn_c", 0.1, -5.0, 5.0, "X tan-sin amplitude"},
        {"popcorn_f", 0.1, -5.0, 5.0, "Y tan-sin amplitude"}
    }, var_popcorn);

    register_variation("rings", 27, "Radial", {
        {"rings_c", 0.5, 0.01, 10.0, "Ring radius spacing"}
    }, var_rings);

    register_variation("rings2", 28, "Radial", {
        {"rings2_val", 0.5, 0.01, 10.0, "Ring2 radius spacing"}
    }, var_rings2);

    register_variation("blob", 29, "Radial", {
        {"blob_low", 0.2, -5.0, 5.0, "Inner radius factor"},
        {"blob_high", 0.8, -5.0, 5.0, "Outer radius factor"},
        {"blob_waves", 4.0, 1.0, 50.0, "Wave count around perimeter"}
    }, var_blob);

    register_variation("pdj", 30, "Complex", {
        {"pdj_a", 1.0, -10.0, 10.0, "A coefficient"},
        {"pdj_b", 1.0, -10.0, 10.0, "B coefficient"},
        {"pdj_c", 1.0, -10.0, 10.0, "C coefficient"},
        {"pdj_d", 1.0, -10.0, 10.0, "D coefficient"}
    }, var_pdj);

    register_variation("perspective", 31, "3D", {
        {"perspective_angle", 0.5, -3.14, 3.14, "Perspective tilt angle"},
        {"perspective_dist", 1.0, 0.1, 50.0, "Camera distance"}
    }, var_perspective);

    register_variation("curl", 32, "Fluid", {
        {"curl_c1", 0.1, -5.0, 5.0, "Linear curl factor"},
        {"curl_c2", 0.1, -5.0, 5.0, "Quadratic curl factor"}
    }, var_curl);

    register_variation("pie", 33, "Radial", {
        {"pie_slices", 6.0, 1.0, 64.0, "Number of angular pie slices"},
        {"pie_rotation", 0.0, -3.14, 3.14, "Initial rotation angle"}
    }, var_pie);
}

void VariationRegistry::register_variation(std::string name, int id, std::string category,
                                          std::vector<VariationParamDef> params, VariationFn fn) {
    size_t index = m_variations.size();
    m_name_to_index[name] = index;
    m_id_to_index[id] = index;
    m_variations.push_back(VariationInfo{std::move(name), id, std::move(category), std::move(params), fn});
}

const VariationInfo* VariationRegistry::find_by_name(std::string_view name) const noexcept {
    auto it = m_name_to_index.find(std::string(name));
    if (it != m_name_to_index.end()) {
        return &m_variations[it->second];
    }
    return nullptr;
}

const VariationInfo* VariationRegistry::find_by_id(int id) const noexcept {
    auto it = m_id_to_index.find(id);
    if (it != m_id_to_index.end()) {
        return &m_variations[it->second];
    }
    return nullptr;
}

} // namespace ApoNeo::Core
