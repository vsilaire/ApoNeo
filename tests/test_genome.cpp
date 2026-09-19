#include "core/Affine2D.hpp"
#include "core/FlameGenome.hpp"
#include "core/FlameXml.hpp"
#include "core/Palette.hpp"
#include "core/Variation.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

void test_affine2d() {
    using namespace ApoNeo::Core;
    std::cout << "[Test] Affine2D..." << std::endl;

    Affine2D id = Affine2D::identity();
    double x = 3.0, y = 4.0;
    id.transform(x, y);
    assert(std::abs(x - 3.0) < 1e-9 && std::abs(y - 4.0) < 1e-9);

    Affine2D trans = Affine2D::translation(2.0, -1.0);
    x = 1.0; y = 1.0;
    trans.transform(x, y);
    assert(std::abs(x - 3.0) < 1e-9 && std::abs(y - 0.0) < 1e-9);

    Affine2D inv = trans.inverse();
    inv.transform(x, y);
    assert(std::abs(x - 1.0) < 1e-9 && std::abs(y - 1.0) < 1e-9);

    std::cout << "  -> Affine2D passed." << std::endl;
}

void test_palette() {
    using namespace ApoNeo::Core;
    std::cout << "[Test] Palette..." << std::endl;

    Palette fire = Palette::preset_fire();
    ColorRGBA c0 = fire.sample(0.0);
    assert(c0.r >= 0.0f && c0.a == 1.0f);

    ColorRGBA c1 = fire.sample(1.0);
    assert(c1.r > 0.8f && c1.g > 0.8f);

    std::string hex = fire.to_hex_string();
    assert(!hex.empty());

    Palette restored;
    assert(restored.from_hex_string(hex));

    std::cout << "  -> Palette passed." << std::endl;
}

void test_variations() {
    using namespace ApoNeo::Core;
    std::cout << "[Test] Variations..." << std::endl;

    auto& reg = VariationRegistry::instance();
    const auto* lin = reg.find_by_name("linear");
    assert(lin != nullptr && lin->fn != nullptr);

    VariationContext ctx;
    ctx.update(2.0, 3.0);
    double ox = 0.0, oy = 0.0, oz = 0.0;
    lin->fn(ctx, 1.0, nullptr, ox, oy, oz);
    assert(std::abs(ox - 2.0) < 1e-9 && std::abs(oy - 3.0) < 1e-9);

    std::cout << "  -> Variations passed." << std::endl;
}

void test_flame_xml_roundtrip() {
    using namespace ApoNeo::Core;
    std::cout << "[Test] FlameXml Round-trip..." << std::endl;

    FlameGenome orig = FlameGenome::preset_swirl_flame();
    std::string xml = FlameXml::serialize_flame(orig);
    assert(!xml.empty());

    FlameGenome parsed;
    std::string err;
    assert(FlameXml::parse_single_flame(xml, parsed, &err));
    assert(parsed.xforms.size() == orig.xforms.size());
    assert(std::abs(parsed.scale - orig.scale) < 1e-5);

    std::cout << "  -> FlameXml Round-trip passed." << std::endl;
}

int main() {
    std::cout << "=== Running Core Tests ===" << std::endl;
    test_affine2d();
    test_palette();
    test_variations();
    test_flame_xml_roundtrip();
    std::cout << "=== All Core Tests Passed! ===" << std::endl;
    return 0;
}
