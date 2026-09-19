#pragma once

#include <cmath>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <numbers>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ApoNeo::Core {

/// @brief Precomputed polar and Euclidean coordinates passed to variation functions
struct VariationContext {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double r = 0.0;       // sqrt(x^2 + y^2)
    double r2 = 0.0;      // x^2 + y^2
    double theta = 0.0;   // atan2(y, x)
    double phi = 0.0;     // 3D polar angle

    // Fast PRNG values in [0, 1) if variation requires randomness
    double rand_val1 = 0.0;
    double rand_val2 = 0.0;

    void update(double nx, double ny, double nz = 0.0, double rv1 = 0.0, double rv2 = 0.0) noexcept {
        x = nx;
        y = ny;
        z = nz;
        r2 = x * x + y * y;
        r = std::sqrt(r2);
        theta = std::atan2(y, x);
        phi = (r2 + z * z > 1e-12) ? std::atan2(z, r) : 0.0;
        rand_val1 = rv1;
        rand_val2 = rv2;
    }
};

/// @brief Variation evaluator signature: adds (dx * weight, dy * weight, dz * weight) to (out_x, out_y, out_z)
using VariationFn = void (*)(const VariationContext& ctx, double weight,
                             const double* params, double& out_x, double& out_y, double& out_z);

struct VariationParamDef {
    std::string name;
    double default_val = 0.0;
    double min_val = -1000.0;
    double max_val = 1000.0;
    std::string description;
};

struct VariationInfo {
    std::string name;
    int id = 0;
    std::string category = "Standard";
    std::vector<VariationParamDef> params;
    VariationFn fn = nullptr;

    bool has_params() const noexcept { return !params.empty(); }
};

class VariationRegistry {
public:
    static VariationRegistry& instance();

    const std::vector<VariationInfo>& all_variations() const noexcept { return m_variations; }
    const VariationInfo* find_by_name(std::string_view name) const noexcept;
    const VariationInfo* find_by_id(int id) const noexcept;

private:
    VariationRegistry();
    void register_variation(std::string name, int id, std::string category,
                            std::vector<VariationParamDef> params, VariationFn fn);

    std::vector<VariationInfo> m_variations;
    std::unordered_map<std::string, size_t> m_name_to_index;
    std::unordered_map<int, size_t> m_id_to_index;
};

} // namespace ApoNeo::Core
