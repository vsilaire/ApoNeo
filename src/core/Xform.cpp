#include "Xform.hpp"

namespace ApoNeo::Core {

Xform::Xform() {
    // Default transform: linear variation with weight 1.0
    variation_weights["linear"] = 1.0;
    compile_variations();
}

void Xform::set_variation(const std::string& name, double weight) {
    if (std::abs(weight) < 1e-12) {
        variation_weights.erase(name);
    } else {
        variation_weights[name] = weight;
    }
    compile_variations();
}

double Xform::get_variation(const std::string& name) const {
    auto it = variation_weights.find(name);
    return (it != variation_weights.end()) ? it->second : 0.0;
}

void Xform::remove_variation(const std::string& name) {
    variation_weights.erase(name);
    compile_variations();
}

void Xform::set_param(const std::string& param_name, double value) {
    params[param_name] = value;
    compile_variations();
}

double Xform::get_param(const std::string& param_name, double default_val) const {
    auto it = params.find(param_name);
    return (it != params.end()) ? it->second : default_val;
}

void Xform::compile_variations() {
    m_compiled.clear();
    auto& reg = VariationRegistry::instance();

    for (const auto& [var_name, var_weight] : variation_weights) {
        if (std::abs(var_weight) < 1e-12) continue;

        const auto* info = reg.find_by_name(var_name);
        if (!info || !info->fn) continue;

        CompiledVariation cv;
        cv.fn = info->fn;
        cv.weight = var_weight;

        for (const auto& p_name : info->param_names) {
            cv.params.push_back(get_param(p_name, 0.0));
        }

        m_compiled.push_back(std::move(cv));
    }
}

void Xform::apply(VariationContext& ctx, double& x, double& y, double& z, double rv1, double rv2) const noexcept {
    // 1. Pre-affine transform
    affine.transform(x, y);

    // 2. Precompute polar/Euclidean coordinates
    ctx.update(x, y, z, rv1, rv2);

    // 3. Accumulate variations
    double ox = 0.0;
    double oy = 0.0;
    double oz = 0.0;

    for (const auto& cv : m_compiled) {
        const double* param_ptr = cv.params.empty() ? nullptr : cv.params.data();
        cv.fn(ctx, cv.weight, param_ptr, ox, oy, oz);
    }

    x = ox;
    y = oy;
    z = oz;

    // 4. Post-affine transform (if active)
    if (has_post_affine) {
        post_affine.transform(x, y);
    }
}

} // namespace ApoNeo::Core
