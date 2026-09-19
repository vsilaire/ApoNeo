#include "RandomFlameGenerator.hpp"
#include "Variation.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>

namespace ApoNeo::Core {

RandomFlameGenerator::RandomFlameGenerator(uint64_t seed) {
    if (seed == 0) {
        seed = static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    }
    m_rng.seed(seed);
}

void RandomFlameGenerator::set_seed(uint64_t seed) {
    m_rng.seed(seed);
}

Palette RandomFlameGenerator::generate_random_palette(std::mt19937_64& rng) {
    std::uniform_real_distribution<double> dist01(0.0, 1.0);
    std::uniform_int_distribution<int> stop_count_dist(3, 6);

    int num_stops = stop_count_dist(rng);
    struct ColorStop {
        double pos;
        double r, g, b;
    };
    std::vector<ColorStop> stops;

    // Generate random palette theme (base hue + spread)
    double base_hue = dist01(rng);
    double hue_spread = 0.2 + 0.6 * dist01(rng);

    for (int i = 0; i < num_stops; ++i) {
        double pos = static_cast<double>(i) / (num_stops - 1);
        double h = std::fmod(base_hue + pos * hue_spread + dist01(rng) * 0.1, 1.0);
        double s = 0.6 + 0.4 * dist01(rng); // Rich saturation
        double v = (i == 0 || i == num_stops - 1) ? (0.2 + 0.7 * dist01(rng)) : (0.7 + 0.3 * dist01(rng));

        // HSV to RGB conversion
        double c = v * s;
        double x = c * (1.0 - std::abs(std::fmod(h * 6.0, 2.0) - 1.0));
        double m = v - c;
        double r = 0, g = 0, b = 0;

        int h_i = static_cast<int>(h * 6.0) % 6;
        switch (h_i) {
            case 0: r = c; g = x; b = 0; break;
            case 1: r = x; g = c; b = 0; break;
            case 2: r = 0; g = c; b = x; break;
            case 3: r = 0; g = x; b = c; break;
            case 4: r = x; g = 0; b = c; break;
            case 5: r = c; g = 0; b = x; break;
        }
        stops.push_back({pos, r + m, g + m, b + m});
    }

    // Interpolate 256 colors
    Palette pal;
    for (int i = 0; i < 256; ++i) {
        double t = static_cast<double>(i) / 255.0;
        // Find interval
        size_t idx = 0;
        while (idx + 1 < stops.size() && stops[idx + 1].pos < t) {
            idx++;
        }
        if (idx + 1 >= stops.size()) {
            pal.set_color(i, ColorRGBA(static_cast<float>(stops.back().r), static_cast<float>(stops.back().g), static_cast<float>(stops.back().b), 1.0f));
        } else {
            double local_t = (t - stops[idx].pos) / (stops[idx + 1].pos - stops[idx].pos);
            // Smoothstep
            double smooth = local_t * local_t * (3.0 - 2.0 * local_t);
            double r = stops[idx].r + smooth * (stops[idx + 1].r - stops[idx].r);
            double g = stops[idx].g + smooth * (stops[idx + 1].g - stops[idx].g);
            double b = stops[idx].b + smooth * (stops[idx + 1].b - stops[idx].b);
            pal.set_color(i, ColorRGBA(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b), 1.0f));
        }
    }
    return pal;
}

FlameGenome RandomFlameGenerator::generate(const std::string& name) {
    FlameGenome g;
    g.name = name;
    g.width = 800;
    g.height = 600;
    g.supersample = 2;
    g.quality = 100.0;
    g.scale = 160.0;
    g.center_x = 0.0;
    g.center_y = 0.0;
    g.tone_map.gamma = 4.0;
    g.tone_map.brightness = 1.0;
    g.tone_map.vibrancy = 1.0;
    g.background = ColorRGBA(0.0f, 0.0f, 0.0f, 1.0f);
    g.palette = generate_random_palette(m_rng);

    std::uniform_real_distribution<double> dist01(0.0, 1.0);
    std::uniform_real_distribution<double> angle_dist(0.0, 2.0 * M_PI);
    std::uniform_real_distribution<double> scale_dist(0.4, 1.25);
    std::uniform_real_distribution<double> trans_dist(-0.75, 0.75);
    std::uniform_int_distribution<int> num_xforms_dist(2, 4);

    int num_xforms = num_xforms_dist(m_rng);
    g.xforms.clear();

    // Pool of rich variations for procedural discovery
    static const std::vector<std::string> variation_pool = {
        "linear", "sinusoidal", "spherical", "swirl", "horseshoe",
        "polar", "julian", "bent", "waves", "fisheye", "popcorn",
        "bubble", "pdj", "cylinder", "gaussian", "eyefish",
        "exponential", "blob", "curl", "pie", "rings"
    };

    std::uniform_int_distribution<size_t> var_idx_dist(0, variation_pool.size() - 1);

    for (int i = 0; i < num_xforms; ++i) {
        Xform xf;
        xf.name = "Xform " + std::to_string(i + 1);
        xf.weight = 0.5 + 1.5 * dist01(m_rng);
        xf.color_index = static_cast<double>(i) / std::max(1, num_xforms - 1);
        xf.color_speed = 0.3 + 0.4 * dist01(m_rng);

        // Pre-Affine: Rotation * Scale + Translation
        double rot = angle_dist(m_rng);
        double sx = scale_dist(m_rng);
        double sy = scale_dist(m_rng);
        if (dist01(m_rng) < 0.6) {
            sy = sx; // Keep uniform scale 60% of the time
        }
        double tx = trans_dist(m_rng);
        double ty = trans_dist(m_rng);

        xf.affine = Affine2D::translation(tx, ty) * Affine2D::rotation(rot) * Affine2D::scaling(sx, sy);

        // Variations
        xf.variation_weights.clear();
        std::uniform_int_distribution<int> var_count_dist(1, 3);
        int var_count = var_count_dist(m_rng);

        // Ensure at least one xform has non-linear variation
        std::vector<std::string> chosen_vars;
        for (int v = 0; v < var_count; ++v) {
            std::string var_name = variation_pool[var_idx_dist(m_rng)];
            if (std::find(chosen_vars.begin(), chosen_vars.end(), var_name) == chosen_vars.end()) {
                chosen_vars.push_back(var_name);
            }
        }
        if (i == 0 && chosen_vars.size() == 1 && chosen_vars[0] == "linear") {
            chosen_vars.push_back(variation_pool[1 + (var_idx_dist(m_rng) % (variation_pool.size() - 1))]);
        }

        double total_w = 0.0;
        std::vector<double> raw_weights;
        for (size_t v = 0; v < chosen_vars.size(); ++v) {
            double w = 0.2 + 0.8 * dist01(m_rng);
            raw_weights.push_back(w);
            total_w += w;
        }

        for (size_t v = 0; v < chosen_vars.size(); ++v) {
            std::string var_name = chosen_vars[v];
            double norm_w = (total_w > 1e-6) ? (raw_weights[v] / total_w) : 1.0;
            xf.set_variation(var_name, norm_w);

            // Set parameters if needed
            if (var_name == "julian") {
                std::uniform_int_distribution<int> power_dist(2, 5);
                xf.set_param("julian_power", static_cast<double>(power_dist(m_rng)));
                xf.set_param("julian_dist", 0.5 + dist01(m_rng));
            } else if (var_name == "waves") {
                xf.set_param("waves_b", -0.5 + dist01(m_rng));
                xf.set_param("waves_c", -0.5 + dist01(m_rng));
                xf.set_param("waves_e", -0.5 + dist01(m_rng));
                xf.set_param("waves_f", -0.5 + dist01(m_rng));
            } else if (var_name == "pdj") {
                xf.set_param("pdj_a", -2.0 + 4.0 * dist01(m_rng));
                xf.set_param("pdj_b", -2.0 + 4.0 * dist01(m_rng));
                xf.set_param("pdj_c", -2.0 + 4.0 * dist01(m_rng));
                xf.set_param("pdj_d", -2.0 + 4.0 * dist01(m_rng));
            } else if (var_name == "blob") {
                xf.set_param("blob_low", 0.2);
                xf.set_param("blob_high", 0.8);
                xf.set_param("blob_waves", 3.0 + static_cast<int>(dist01(m_rng) * 4));
            } else if (var_name == "curl") {
                xf.set_param("curl_c1", 0.1 + 0.3 * dist01(m_rng));
                xf.set_param("curl_c2", 0.1 + 0.3 * dist01(m_rng));
            }
        }

        // 25% chance of Post-Affine for symmetry/depth
        if (dist01(m_rng) < 0.25) {
            xf.has_post_affine = true;
            double p_rot = angle_dist(m_rng) * 0.5;
            double p_scale = 0.7 + 0.6 * dist01(m_rng);
            double p_tx = trans_dist(m_rng) * 0.5;
            double p_ty = trans_dist(m_rng) * 0.5;
            xf.post_affine = Affine2D::translation(p_tx, p_ty) * Affine2D::rotation(p_rot) * Affine2D::scaling(p_scale, p_scale);
        }

        xf.compile_variations();
        g.xforms.push_back(xf);
    }

    g.compile();
    return g;
}

std::vector<FlameGenome> RandomFlameGenerator::generate_batch(size_t count, const std::string& base_name) {
    std::vector<FlameGenome> batch;
    batch.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        std::string name = base_name + " " + std::to_string(i + 1);
        batch.push_back(generate(name));
    }
    return batch;
}

} // namespace ApoNeo::Core
