#pragma once

#include "Affine2D.hpp"
#include "Palette.hpp"
#include "Xform.hpp"
#include <memory>
#include <string>
#include <vector>

namespace ApoNeo::Core {

struct CameraParams {
    double pitch = 0.0;
    double yaw = 0.0;
    double perspective = 0.0;
    double z_pos = 0.0;
    double depth_blur = 0.0;
};

struct ToneMapParams {
    double gamma = 4.0;
    double brightness = 1.0;
    double vibrancy = 1.0;
    double highlight_power = 0.0;
};

class FlameGenome {
public:
    std::string name = "Untitled Flame";
    int width = 800;
    int height = 600;
    int supersample = 2;       // Spatial oversampling factor (1x, 2x, 3x)
    double quality = 100.0;    // Iterations / samples per pixel
    int fuse_iterations = 30;  // Warmup iterations before plotting

    double scale = 150.0;      // Pixels per unit
    double center_x = 0.0;
    double center_y = 0.0;
    double zoom = 0.0;
    double rotate = 0.0;       // Radians

    CameraParams camera;
    ToneMapParams tone_map;
    ColorRGBA background = ColorRGBA(0.0f, 0.0f, 0.0f, 1.0f);
    Palette palette;

    std::vector<Xform> xforms;
    bool has_final_xform = false;
    Xform final_xform;

    FlameGenome();

    /// @brief Compile all transform variations and recalculate cumulative selection probabilities
    void compile();

    /// @brief Get pre-calculated cumulative weights for fast alias/binary-search transform selection
    const std::vector<double>& cumulative_weights() const noexcept { return m_cumulative_weights; }
    double total_weight() const noexcept { return m_total_weight; }

    /// @brief Smoothly interpolate between two genomes for animation morphing (flam3-animate style)
    static FlameGenome interpolate(const FlameGenome& g1, const FlameGenome& g2, double t);

    /// @brief Preset flames
    static FlameGenome preset_sierpinski();
    static FlameGenome preset_barnsley_fern();
    static FlameGenome preset_swirl_flame();
    static FlameGenome preset_julia_vortex();

private:
    std::vector<double> m_cumulative_weights;
    double m_total_weight = 0.0;
};

} // namespace ApoNeo::Core
