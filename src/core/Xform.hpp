#pragma once

#include "Affine2D.hpp"
#include "Variation.hpp"
#include <map>
#include <string>
#include <vector>

namespace ApoNeo::Core {

/// @brief Pre-compiled active variation instance for fast dispatch during rendering loop
struct CompiledVariation {
    VariationFn fn = nullptr;
    double weight = 0.0;
    std::vector<double> params;
};

class Xform {
public:
    std::string name;
    Affine2D affine = Affine2D::identity();
    Affine2D post_affine = Affine2D::identity();
    bool has_post_affine = false;

    double weight = 1.0;
    double color_index = 0.0;
    double color_speed = 0.5;
    double opacity = 1.0;
    bool visible = true;

    // Map variation name -> weight
    std::map<std::string, double> variation_weights;
    // Map parameter name -> value (e.g. "waves_b" -> 0.5)
    std::map<std::string, double> params;
    // Chaos weights: transition probability multipliers to target xforms
    std::vector<double> chaos_weights;

    Xform();

    void set_variation(const std::string& name, double weight);
    double get_variation(const std::string& name) const;
    void remove_variation(const std::string& name);

    void set_param(const std::string& param_name, double value);
    double get_param(const std::string& param_name, double default_val = 0.0) const;

    /// @brief Compile active variations into contiguous function pointers for fast chaos iteration
    void compile_variations();

    /// @brief Apply transform in single-step chaos iteration
    void apply(VariationContext& ctx, double& x, double& y, double& z, double rv1, double rv2) const noexcept;

    const std::vector<CompiledVariation>& compiled_variations() const noexcept {
        return m_compiled;
    }

private:
    std::vector<CompiledVariation> m_compiled;
};

} // namespace ApoNeo::Core
