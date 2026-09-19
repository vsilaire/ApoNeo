#include "FlameGenome.hpp"
#include <algorithm>
#include <cmath>

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

} // namespace ApoNeo::Core
