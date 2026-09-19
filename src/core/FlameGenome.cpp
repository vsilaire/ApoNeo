#include "FlameGenome.hpp"
#include <algorithm>
#include <cmath>
#include <set>

namespace ApoNeo::Core {

FlameGenome::FlameGenome() {
    palette = Palette::preset_fire();
    compile();
}

void FlameGenome::compile() {
    m_cumulative_weights.clear();
    m_total_weight = 0.0;

    for (auto& xf : xforms) {
        xf.compile_variations();
        m_total_weight += std::max(0.0, xf.weight);
        m_cumulative_weights.push_back(m_total_weight);
    }

    if (has_final_xform) {
        final_xform.compile_variations();
    }
}

FlameGenome FlameGenome::preset_sierpinski() {
    FlameGenome g;
    g.name = "Sierpinski Gasket";
    g.width = 800;
    g.height = 600;
    g.scale = 220.0;
    g.center_x = 0.0;
    g.center_y = -0.15;
    g.palette = Palette::preset_electric_blue();
    g.xforms.clear();

    // 3 transforms with scale 0.5 situated at the vertices of an equilateral triangle
    Xform xf1;
    xf1.affine = Affine2D(0.5, 0.0, -0.5, 0.0, 0.5, -0.288675);
    xf1.set_variation("linear", 1.0);
    xf1.color_index = 0.0;
    xf1.weight = 1.0;

    Xform xf2;
    xf2.affine = Affine2D(0.5, 0.0, 0.5, 0.0, 0.5, -0.288675);
    xf2.set_variation("linear", 1.0);
    xf2.color_index = 0.5;
    xf2.weight = 1.0;

    Xform xf3;
    xf3.affine = Affine2D(0.5, 0.0, 0.0, 0.0, 0.5, 0.57735);
    xf3.set_variation("linear", 1.0);
    xf3.color_index = 1.0;
    xf3.weight = 1.0;

    g.xforms.push_back(xf1);
    g.xforms.push_back(xf2);
    g.xforms.push_back(xf3);
    g.compile();
    return g;
}

FlameGenome FlameGenome::preset_barnsley_fern() {
    FlameGenome g;
    g.name = "Barnsley Fern";
    g.width = 800;
    g.height = 600;
    g.scale = 55.0;
    g.center_x = 0.0;
    g.center_y = 5.0;
    g.palette = Palette::preset_aurora();
    g.xforms.clear();

    // Stem
    Xform xf1;
    xf1.affine = Affine2D(0.0, 0.0, 0.0, 0.0, 0.16, 0.0);
    xf1.set_variation("linear", 1.0);
    xf1.weight = 0.01;
    xf1.color_index = 0.1;

    // Successively smaller leaflets
    Xform xf2;
    xf2.affine = Affine2D(0.85, 0.04, 0.0, -0.04, 0.85, 1.6);
    xf2.set_variation("linear", 1.0);
    xf2.weight = 0.85;
    xf2.color_index = 0.4;

    // Largest left leaflet
    Xform xf3;
    xf3.affine = Affine2D(0.2, -0.26, 0.0, 0.23, 0.22, 1.6);
    xf3.set_variation("linear", 1.0);
    xf3.weight = 0.07;
    xf3.color_index = 0.7;

    // Largest right leaflet
    Xform xf4;
    xf4.affine = Affine2D(-0.15, 0.28, 0.0, 0.26, 0.24, 0.44);
    xf4.set_variation("linear", 1.0);
    xf4.weight = 0.07;
    xf4.color_index = 0.9;

    g.xforms.push_back(xf1);
    g.xforms.push_back(xf2);
    g.xforms.push_back(xf3);
    g.xforms.push_back(xf4);
    g.compile();
    return g;
}

FlameGenome FlameGenome::preset_swirl_flame() {
    FlameGenome g;
    g.name = "Swirl Vortex";
    g.width = 800;
    g.height = 600;
    g.scale = 180.0;
    g.center_x = 0.0;
    g.center_y = 0.0;
    g.palette = Palette::preset_fire();
    g.xforms.clear();

    Xform xf1;
    xf1.affine = Affine2D(0.7071, -0.7071, 0.1, 0.7071, 0.7071, -0.1);
    xf1.set_variation("swirl", 0.8);
    xf1.set_variation("linear", 0.3);
    xf1.color_index = 0.2;
    xf1.weight = 1.0;

    Xform xf2;
    xf2.affine = Affine2D(0.5, 0.3, -0.2, -0.3, 0.5, 0.2);
    xf2.set_variation("sinusoidal", 0.6);
    xf2.set_variation("spherical", 0.4);
    xf2.color_index = 0.6;
    xf2.weight = 0.8;

    Xform xf3;
    xf3.affine = Affine2D(-0.4, 0.6, 0.0, -0.6, -0.4, 0.0);
    xf3.set_variation("horseshoe", 0.7);
    xf3.color_index = 0.95;
    xf3.weight = 0.5;

    g.xforms.push_back(xf1);
    g.xforms.push_back(xf2);
    g.xforms.push_back(xf3);
    g.compile();
    return g;
}

FlameGenome FlameGenome::preset_julia_vortex() {
    FlameGenome g;
    g.name = "Julia Vortex";
    g.width = 800;
    g.height = 600;
    g.scale = 200.0;
    g.center_x = 0.0;
    g.center_y = 0.0;
    g.palette = Palette::preset_sunset();
    g.xforms.clear();

    Xform xf1;
    xf1.affine = Affine2D(0.8, -0.2, 0.0, 0.2, 0.8, 0.0);
    xf1.set_variation("julia", 0.9);
    xf1.set_variation("swirl", 0.2);
    xf1.color_index = 0.15;
    xf1.weight = 1.0;

    Xform xf2;
    xf2.affine = Affine2D(0.6, 0.4, 0.2, -0.4, 0.6, -0.2);
    xf2.set_variation("polar", 0.5);
    xf2.set_variation("disc", 0.3);
    xf2.color_index = 0.8;
    xf2.weight = 0.75;

    g.xforms.push_back(xf1);
    g.xforms.push_back(xf2);
    g.compile();
    return g;
}

FlameGenome FlameGenome::interpolate(const FlameGenome& g1, const FlameGenome& g2, double t) {
    t = std::clamp(t, 0.0, 1.0);
    if (t <= 0.0) return g1;
    if (t >= 1.0) return g2;

    FlameGenome result;
    result.name = g1.name + " -> " + g2.name;
    result.width = g1.width;
    result.height = g1.height;
    result.supersample = (t < 0.5) ? g1.supersample : g2.supersample;
    result.quality = (1.0 - t) * g1.quality + t * g2.quality;
    result.fuse_iterations = static_cast<int>(std::round((1.0 - t) * g1.fuse_iterations + t * g2.fuse_iterations));

    result.scale = (1.0 - t) * g1.scale + t * g2.scale;
    result.center_x = (1.0 - t) * g1.center_x + t * g2.center_x;
    result.center_y = (1.0 - t) * g1.center_y + t * g2.center_y;
    result.zoom = (1.0 - t) * g1.zoom + t * g2.zoom;
    result.rotate = (1.0 - t) * g1.rotate + t * g2.rotate;

    result.tone_map.gamma = (1.0 - t) * g1.tone_map.gamma + t * g2.tone_map.gamma;
    result.tone_map.brightness = (1.0 - t) * g1.tone_map.brightness + t * g2.tone_map.brightness;
    result.tone_map.vibrancy = (1.0 - t) * g1.tone_map.vibrancy + t * g2.tone_map.vibrancy;
    result.tone_map.highlight_power = (1.0 - t) * g1.tone_map.highlight_power + t * g2.tone_map.highlight_power;

    result.camera.pitch = (1.0 - t) * g1.camera.pitch + t * g2.camera.pitch;
    result.camera.yaw = (1.0 - t) * g1.camera.yaw + t * g2.camera.yaw;
    result.camera.perspective = (1.0 - t) * g1.camera.perspective + t * g2.camera.perspective;
    result.camera.z_pos = (1.0 - t) * g1.camera.z_pos + t * g2.camera.z_pos;
    result.camera.depth_blur = (1.0 - t) * g1.camera.depth_blur + t * g2.camera.depth_blur;

    result.background.r = static_cast<float>((1.0 - t) * g1.background.r + t * g2.background.r);
    result.background.g = static_cast<float>((1.0 - t) * g1.background.g + t * g2.background.g);
    result.background.b = static_cast<float>((1.0 - t) * g1.background.b + t * g2.background.b);
    result.background.a = static_cast<float>((1.0 - t) * g1.background.a + t * g2.background.a);

    // Palette interpolation (256 colors)
    for (int i = 0; i < 256; ++i) {
        ColorRGBA c1 = g1.palette.get_color(i);
        ColorRGBA c2 = g2.palette.get_color(i);
        ColorRGBA blended(
            static_cast<float>((1.0 - t) * c1.r + t * c2.r),
            static_cast<float>((1.0 - t) * c1.g + t * c2.g),
            static_cast<float>((1.0 - t) * c1.b + t * c2.b),
            static_cast<float>((1.0 - t) * c1.a + t * c2.a)
        );
        result.palette.set_color(i, blended);
    }

    // Xforms interpolation
    size_t num_xforms = std::max(g1.xforms.size(), g2.xforms.size());
    result.xforms.clear();
    result.xforms.reserve(num_xforms);

    for (size_t i = 0; i < num_xforms; ++i) {
        bool in_g1 = (i < g1.xforms.size());
        bool in_g2 = (i < g2.xforms.size());

        if (in_g1 && in_g2) {
            const auto& xf1 = g1.xforms[i];
            const auto& xf2 = g2.xforms[i];
            Xform xf;
            xf.name = (t < 0.5) ? xf1.name : xf2.name;
            xf.weight = (1.0 - t) * xf1.weight + t * xf2.weight;
            xf.color_index = (1.0 - t) * xf1.color_index + t * xf2.color_index;
            xf.color_speed = (1.0 - t) * xf1.color_speed + t * xf2.color_speed;
            xf.opacity = (1.0 - t) * xf1.opacity + t * xf2.opacity;
            xf.visible = (t < 0.5) ? xf1.visible : xf2.visible;

            // Pre-affine
            xf.affine = Affine2D(
                (1.0 - t) * xf1.affine.a + t * xf2.affine.a,
                (1.0 - t) * xf1.affine.b + t * xf2.affine.b,
                (1.0 - t) * xf1.affine.c + t * xf2.affine.c,
                (1.0 - t) * xf1.affine.d + t * xf2.affine.d,
                (1.0 - t) * xf1.affine.e + t * xf2.affine.e,
                (1.0 - t) * xf1.affine.f + t * xf2.affine.f
            );

            // Post-affine
            xf.has_post_affine = (xf1.has_post_affine || xf2.has_post_affine);
            if (xf.has_post_affine) {
                Affine2D p1 = xf1.has_post_affine ? xf1.post_affine : Affine2D::identity();
                Affine2D p2 = xf2.has_post_affine ? xf2.post_affine : Affine2D::identity();
                xf.post_affine = Affine2D(
                    (1.0 - t) * p1.a + t * p2.a,
                    (1.0 - t) * p1.b + t * p2.b,
                    (1.0 - t) * p1.c + t * p2.c,
                    (1.0 - t) * p1.d + t * p2.d,
                    (1.0 - t) * p1.e + t * p2.e,
                    (1.0 - t) * p1.f + t * p2.f
                );
            }

            // Variations
            std::set<std::string> var_names;
            for (const auto& [name, _] : xf1.variation_weights) var_names.insert(name);
            for (const auto& [name, _] : xf2.variation_weights) var_names.insert(name);

            for (const auto& vname : var_names) {
                double w1 = xf1.get_variation(vname);
                double w2 = xf2.get_variation(vname);
                double w = (1.0 - t) * w1 + t * w2;
                if (std::abs(w) > 1e-6) {
                    xf.set_variation(vname, w);
                }
            }

            // Params
            std::set<std::string> param_names;
            for (const auto& [name, _] : xf1.params) param_names.insert(name);
            for (const auto& [name, _] : xf2.params) param_names.insert(name);

            for (const auto& pname : param_names) {
                double p1 = xf1.get_param(pname, 0.0);
                double p2 = xf2.get_param(pname, 0.0);
                xf.set_param(pname, (1.0 - t) * p1 + t * p2);
            }

            xf.compile_variations();
            result.xforms.push_back(xf);
        } else if (in_g1) {
            // Fading out
            Xform xf = g1.xforms[i];
            xf.weight = (1.0 - t) * xf.weight;
            xf.compile_variations();
            result.xforms.push_back(xf);
        } else {
            // Fading in
            Xform xf = g2.xforms[i];
            xf.weight = t * xf.weight;
            xf.compile_variations();
            result.xforms.push_back(xf);
        }
    }

    // Final Xform
    if (g1.has_final_xform || g2.has_final_xform) {
        result.has_final_xform = true;
        const auto& fx1 = g1.has_final_xform ? g1.final_xform : Xform();
        const auto& fx2 = g2.has_final_xform ? g2.final_xform : Xform();
        result.final_xform.affine = Affine2D(
            (1.0 - t) * fx1.affine.a + t * fx2.affine.a,
            (1.0 - t) * fx1.affine.b + t * fx2.affine.b,
            (1.0 - t) * fx1.affine.c + t * fx2.affine.c,
            (1.0 - t) * fx1.affine.d + t * fx2.affine.d,
            (1.0 - t) * fx1.affine.e + t * fx2.affine.e,
            (1.0 - t) * fx1.affine.f + t * fx2.affine.f
        );
        std::set<std::string> var_names;
        for (const auto& [name, _] : fx1.variation_weights) var_names.insert(name);
        for (const auto& [name, _] : fx2.variation_weights) var_names.insert(name);
        for (const auto& vname : var_names) {
            double w = (1.0 - t) * fx1.get_variation(vname) + t * fx2.get_variation(vname);
            if (std::abs(w) > 1e-6) {
                result.final_xform.set_variation(vname, w);
            }
        }
        result.final_xform.compile_variations();
    }

    result.compile();
    return result;
}

} // namespace ApoNeo::Core
