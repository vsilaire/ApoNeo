#include "Palette.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace ApoNeo::Core {

Palette::Palette() {
    for (size_t i = 0; i < PALETTE_SIZE; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(PALETTE_SIZE - 1);
        m_colors[i] = ColorRGBA(t, t, t, 1.0f);
    }
}

ColorRGBA Palette::sample(double index) const noexcept {
    // Wrap index into [0.0, 1.0)
    index = index - std::floor(index);
    if (index < 0.0) index += 1.0;

    const double pos = index * static_cast<double>(PALETTE_SIZE);
    const size_t i0 = static_cast<size_t>(pos) % PALETTE_SIZE;
    const size_t i1 = (i0 + 1) % PALETTE_SIZE;
    const float frac = static_cast<float>(pos - static_cast<double>(i0));

    const ColorRGBA& c0 = m_colors[i0];
    const ColorRGBA& c1 = m_colors[i1];

    return ColorRGBA(
        c0.r + (c1.r - c0.r) * frac,
        c0.g + (c1.g - c0.g) * frac,
        c0.b + (c1.b - c0.b) * frac,
        c0.a + (c1.a - c0.a) * frac
    );
}

bool Palette::from_hex_string(const std::string& hex_str) {
    // Filter out whitespace, newlines, etc.
    std::string clean;
    clean.reserve(hex_str.size());
    for (char ch : hex_str) {
        if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F')) {
            clean.push_back(ch);
        }
    }

    if (clean.size() < PALETTE_SIZE * 6) {
        return false;
    }

    auto hex_to_byte = [](char high, char low) -> uint8_t {
        auto val = [](char c) -> uint8_t {
            if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
            if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(c - 'a' + 10);
            if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(c - 'A' + 10);
            return 0;
        };
        return static_cast<uint8_t>((val(high) << 4) | val(low));
    };

    for (size_t i = 0; i < PALETTE_SIZE; ++i) {
        size_t idx = i * 6;
        uint8_t r = hex_to_byte(clean[idx + 0], clean[idx + 1]);
        uint8_t g = hex_to_byte(clean[idx + 2], clean[idx + 3]);
        uint8_t b = hex_to_byte(clean[idx + 4], clean[idx + 5]);

        m_colors[i] = ColorRGBA(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
    }

    return true;
}

std::string Palette::to_hex_string() const {
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < PALETTE_SIZE; ++i) {
        int r = std::clamp(static_cast<int>(std::round(m_colors[i].r * 255.0f)), 0, 255);
        int g = std::clamp(static_cast<int>(std::round(m_colors[i].g * 255.0f)), 0, 255);
        int b = std::clamp(static_cast<int>(std::round(m_colors[i].b * 255.0f)), 0, 255);

        ss << std::setw(2) << r << std::setw(2) << g << std::setw(2) << b;
        if ((i + 1) % 16 == 0 && i + 1 < PALETTE_SIZE) {
            ss << "\n";
        }
    }
    return ss.str();
}

static Palette create_interpolated(const std::vector<std::pair<float, ColorRGBA>>& knots, const std::string& name) {
    Palette p;
    p.set_name(name);
    for (size_t i = 0; i < Palette::PALETTE_SIZE; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(Palette::PALETTE_SIZE - 1);
        if (t <= knots.front().first) {
            p.set_color(i, knots.front().second);
            continue;
        }
        if (t >= knots.back().first) {
            p.set_color(i, knots.back().second);
            continue;
        }
        for (size_t k = 0; k + 1 < knots.size(); ++k) {
            if (t >= knots[k].first && t <= knots[k + 1].first) {
                float seg = (t - knots[k].first) / (knots[k + 1].first - knots[k].first);
                ColorRGBA c0 = knots[k].second;
                ColorRGBA c1 = knots[k + 1].second;
                p.set_color(i, ColorRGBA(
                    c0.r + (c1.r - c0.r) * seg,
                    c0.g + (c1.g - c0.g) * seg,
                    c0.b + (c1.b - c0.b) * seg,
                    1.0f
                ));
                break;
            }
        }
    }
    return p;
}

Palette Palette::preset_fire() {
    return create_interpolated({
        {0.00f, ColorRGBA(0.00f, 0.00f, 0.00f)},
        {0.25f, ColorRGBA(0.50f, 0.00f, 0.00f)},
        {0.50f, ColorRGBA(0.95f, 0.40f, 0.00f)},
        {0.75f, ColorRGBA(1.00f, 0.85f, 0.10f)},
        {1.00f, ColorRGBA(1.00f, 1.00f, 0.90f)}
    }, "Fire");
}

Palette Palette::preset_electric_blue() {
    return create_interpolated({
        {0.00f, ColorRGBA(0.01f, 0.02f, 0.08f)},
        {0.30f, ColorRGBA(0.05f, 0.20f, 0.60f)},
        {0.65f, ColorRGBA(0.10f, 0.70f, 0.95f)},
        {0.85f, ColorRGBA(0.60f, 0.90f, 1.00f)},
        {1.00f, ColorRGBA(1.00f, 1.00f, 1.00f)}
    }, "Electric Blue");
}

Palette Palette::preset_rainbow() {
    return create_interpolated({
        {0.00f, ColorRGBA(1.0f, 0.0f, 0.0f)},
        {0.17f, ColorRGBA(1.0f, 0.6f, 0.0f)},
        {0.33f, ColorRGBA(1.0f, 1.0f, 0.0f)},
        {0.50f, ColorRGBA(0.0f, 0.9f, 0.2f)},
        {0.67f, ColorRGBA(0.0f, 0.7f, 1.0f)},
        {0.83f, ColorRGBA(0.6f, 0.1f, 0.9f)},
        {1.00f, ColorRGBA(1.0f, 0.0f, 0.0f)}
    }, "Rainbow");
}

Palette Palette::preset_aurora() {
    return create_interpolated({
        {0.00f, ColorRGBA(0.02f, 0.05f, 0.10f)},
        {0.30f, ColorRGBA(0.05f, 0.65f, 0.45f)},
        {0.60f, ColorRGBA(0.20f, 0.85f, 0.70f)},
        {0.80f, ColorRGBA(0.70f, 0.20f, 0.80f)},
        {1.00f, ColorRGBA(0.95f, 0.60f, 0.90f)}
    }, "Aurora");
}

Palette Palette::preset_sunset() {
    return create_interpolated({
        {0.00f, ColorRGBA(0.10f, 0.05f, 0.25f)},
        {0.30f, ColorRGBA(0.60f, 0.10f, 0.40f)},
        {0.60f, ColorRGBA(0.95f, 0.35f, 0.20f)},
        {0.85f, ColorRGBA(1.00f, 0.75f, 0.30f)},
        {1.00f, ColorRGBA(1.00f, 0.95f, 0.80f)}
    }, "Sunset");
}

Palette Palette::preset_monochrome() {
    return create_interpolated({
        {0.00f, ColorRGBA(0.0f, 0.0f, 0.0f)},
        {0.50f, ColorRGBA(0.5f, 0.5f, 0.5f)},
        {1.00f, ColorRGBA(1.0f, 1.0f, 1.0f)}
    }, "Monochrome");
}

} // namespace ApoNeo::Core
