#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace ApoNeo::Core {

struct ColorRGBA {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;

    constexpr ColorRGBA() noexcept = default;
    constexpr ColorRGBA(float r_, float g_, float b_, float a_ = 1.0f) noexcept
        : r(r_), g(g_), b(b_), a(a_) {}

    constexpr ColorRGBA operator*(float s) const noexcept {
        return ColorRGBA(r * s, g * s, b * s, a * s);
    }
    constexpr ColorRGBA operator+(const ColorRGBA& o) const noexcept {
        return ColorRGBA(r + o.r, g + o.g, b + o.b, a + o.a);
    }
};

class Palette {
public:
    static constexpr size_t PALETTE_SIZE = 256;

    Palette();

    /// @brief Sample palette color at normalized index [0.0, 1.0] with linear interpolation
    ColorRGBA sample(double index) const noexcept;

    /// @brief Direct access to palette entries (0 to 255)
    const ColorRGBA& get_color(size_t index) const noexcept {
        return m_colors[index % PALETTE_SIZE];
    }

    void set_color(size_t index, const ColorRGBA& color) noexcept {
        m_colors[index % PALETTE_SIZE] = color;
    }

    /// @brief Load palette from standard flam3 512-character hex string (256 RGB pairs)
    bool from_hex_string(const std::string& hex_str);

    /// @brief Export palette to standard flam3 512-character hex string
    std::string to_hex_string() const;

    /// @brief Built-in presets
    static Palette preset_fire();
    static Palette preset_electric_blue();
    static Palette preset_rainbow();
    static Palette preset_aurora();
    static Palette preset_sunset();
    static Palette preset_monochrome();

    const std::string& name() const noexcept { return m_name; }
    void set_name(std::string name) { m_name = std::move(name); }

private:
    std::string m_name = "Default";
    std::array<ColorRGBA, PALETTE_SIZE> m_colors;
};

} // namespace ApoNeo::Core
