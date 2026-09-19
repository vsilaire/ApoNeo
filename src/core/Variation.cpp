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

// 15. Bent
void var_bent(const VariationContext& ctx, double weight, const double*, double& ox, double& oy, double& oz) {
    double nx = (ctx.x >= 0.0) ? ctx.x : 2.0 * ctx.x;
    double ny = (ctx.y >= 0.0) ? ctx.y : 0.5 * ctx.y;
    ox += weight * nx;
    oy += weight * ny;
    oz += weight * ctx.z;
}

// 16. Waves
void var_waves(const VariationContext& ctx, double weight, const double* params, double& ox, double& oy, double& oz) {
    // params: b (freq_x), c (scale_x), e (freq_y), f (scale_y)
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

// 17. Fisheye
void var_fisheye(const VariationContext& ctx, double weight, const double*, double& ox, double& oy, double& oz) {
    double factor = 2.0 / (ctx.r + 1.0);
    ox += weight * factor * ctx.y;
    oy += weight * factor * ctx.x;
    oz += weight * ctx.z;
}

// 18. Popcorn
void var_popcorn(const VariationContext& ctx, double weight, const double* params, double& ox, double& oy, double& oz) {
    double c = params ? params[0] : 0.1;
    double f = params ? params[1] : 0.1;
    ox += weight * (ctx.x + c * std::sin(std::tan(3.0 * ctx.y)));
    oy += weight * (ctx.y + f * std::sin(std::tan(3.0 * ctx.x)));
    oz += weight * ctx.z;
}

// 19. Exponential
void var_exponential(const VariationContext& ctx, double weight, const double*, double& ox, double& oy, double& oz) {
    double exp_x = std::exp(ctx.x - 1.0);
    ox += weight * exp_x * std::cos(PI * ctx.y);
    oy += weight * exp_x * std::sin(PI * ctx.y);
    oz += weight * ctx.z;
}

// 20. Power
void var_power(const VariationContext& ctx, double weight, const double*, double& ox, double& oy, double& oz) {
    double r_sin = (ctx.r > 1e-12) ? std::pow(ctx.r, std::sin(ctx.theta)) : 0.0;
    ox += weight * r_sin * std::cos(ctx.theta);
    oy += weight * r_sin * std::sin(ctx.theta);
    oz += weight * ctx.z;
}

// 21. Cosine
void var_cosine(const VariationContext& ctx, double weight, const double*, double& ox, double& oy, double& oz) {
    ox += weight * std::cos(PI * ctx.x) * std::cosh(ctx.y);
    oy += weight * (-std::sin(PI * ctx.x) * std::sinh(ctx.y));
    oz += weight * ctx.z;
}

// 22. Rings
void var_rings(const VariationContext& ctx, double weight, const double* params, double& ox, double& oy, double& oz) {
    double c = params ? params[0] : 0.5;
    double c2 = c * c;
    double val = (c2 > 1e-6) ? std::fmod(ctx.r + c2, 2.0 * c2) - c2 + ctx.r * (1.0 - c2) : ctx.r;
    ox += weight * val * std::cos(ctx.theta);
    oy += weight * val * std::sin(ctx.theta);
    oz += weight * ctx.z;
}

// 23. Bubble
void var_bubble(const VariationContext& ctx, double weight, const double*, double& ox, double& oy, double& oz) {
    double factor = 4.0 / (ctx.r2 + 4.0);
    ox += weight * factor * ctx.x;
    oy += weight * factor * ctx.y;
    oz += weight * factor * ctx.z;
}

// 24. Cylinder
void var_cylinder(const VariationContext& ctx, double weight, const double*, double& ox, double& oy, double& oz) {
    ox += weight * std::sin(ctx.x);
    oy += weight * ctx.y;
    oz += weight * ctx.z;
}

// 25. Gaussian / Blur
void var_gaussian(const VariationContext& ctx, double weight, const double*, double& ox, double& oy, double& oz) {
    // Box-Muller transform for normal distribution
    double u1 = std::max(ctx.rand_val1, 1e-10);
    double u2 = ctx.rand_val2;
    double rad = std::sqrt(-2.0 * std::log(u1));
    double ang = PI2 * u2;
    ox += weight * (rad * std::cos(ang) - 2.0);
    oy += weight * (rad * std::sin(ang) - 2.0);
    oz += weight * ctx.z;
}

// 26. Eyefish
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
    register_variation("linear", 0, {}, var_linear);
    register_variation("sinusoidal", 1, {}, var_sinusoidal);
    register_variation("spherical", 2, {}, var_spherical);
    register_variation("swirl", 3, {}, var_swirl);
    register_variation("horseshoe", 4, {}, var_horseshoe);
    register_variation("polar", 5, {}, var_polar);
    register_variation("handkerchief", 6, {}, var_handkerchief);
    register_variation("heart", 7, {}, var_heart);
    register_variation("disc", 8, {}, var_disc);
    register_variation("spiral", 9, {}, var_spiral);
    register_variation("hyperbolic", 10, {}, var_hyperbolic);
    register_variation("diamond", 11, {}, var_diamond);
    register_variation("ex", 12, {}, var_ex);
    register_variation("julia", 13, {}, var_julia);
    register_variation("bent", 14, {}, var_bent);
    register_variation("waves", 15, {"waves_b", "waves_c", "waves_e", "waves_f"}, var_waves);
    register_variation("fisheye", 16, {}, var_fisheye);
    register_variation("popcorn", 17, {"popcorn_c", "popcorn_f"}, var_popcorn);
    register_variation("exponential", 18, {}, var_exponential);
    register_variation("power", 19, {}, var_power);
    register_variation("cosine", 20, {}, var_cosine);
    register_variation("rings", 21, {"rings_c"}, var_rings);
    register_variation("bubble", 22, {}, var_bubble);
    register_variation("cylinder", 23, {}, var_cylinder);
    register_variation("gaussian", 24, {}, var_gaussian);
    register_variation("eyefish", 25, {}, var_eyefish);
}

void VariationRegistry::register_variation(std::string name, int id, std::vector<std::string> params, VariationFn fn) {
    size_t index = m_variations.size();
    m_name_to_index[name] = index;
    m_id_to_index[id] = index;
    m_variations.push_back(VariationInfo{std::move(name), id, std::move(params), fn});
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
