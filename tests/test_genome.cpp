#include "core/Affine2D.hpp"
#include "core/FlameGenome.hpp"
#include "core/FlameXml.hpp"
#include "core/Palette.hpp"
#include "core/RandomFlameGenerator.hpp"
#include "core/Variation.hpp"
#include "core/commands/Commands.hpp"
#include <QUndoStack>
#include <cmath>
#include <cstdlib>
#include <iostream>

#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed: " #cond " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::abort(); \
        } \
    } while (0)

void test_affine2d() {
    using namespace ApoNeo::Core;
    std::cout << "[Test] Affine2D..." << std::endl;

    Affine2D id = Affine2D::identity();
    double x = 3.0, y = 4.0;
    id.transform(x, y);
    TEST_ASSERT(std::abs(x - 3.0) < 1e-9 && std::abs(y - 4.0) < 1e-9);

    Affine2D trans = Affine2D::translation(2.0, -1.0);
    x = 1.0; y = 1.0;
    trans.transform(x, y);
    TEST_ASSERT(std::abs(x - 3.0) < 1e-9 && std::abs(y - 0.0) < 1e-9);

    Affine2D inv = trans.inverse();
    inv.transform(x, y);
    TEST_ASSERT(std::abs(x - 1.0) < 1e-9 && std::abs(y - 1.0) < 1e-9);

    std::cout << "  -> Affine2D passed." << std::endl;
}

void test_palette() {
    using namespace ApoNeo::Core;
    std::cout << "[Test] Palette..." << std::endl;

    Palette fire = Palette::preset_fire();
    ColorRGBA c0 = fire.sample(0.0);
    TEST_ASSERT(c0.r >= 0.0f && c0.a == 1.0f);

    ColorRGBA c1 = fire.sample(1.0);
    TEST_ASSERT(c1.r > 0.8f && c1.g > 0.8f);

    std::string hex = fire.to_hex_string();
    TEST_ASSERT(!hex.empty());

    Palette restored;
    TEST_ASSERT(restored.from_hex_string(hex));

    std::cout << "  -> Palette passed." << std::endl;
}

void test_variations() {
    using namespace ApoNeo::Core;
    std::cout << "[Test] Variations..." << std::endl;

    auto& reg = VariationRegistry::instance();
    const auto* lin = reg.find_by_name("linear");
    TEST_ASSERT(lin != nullptr && lin->fn != nullptr);

    VariationContext ctx;
    ctx.update(2.0, 3.0);
    double ox = 0.0, oy = 0.0, oz = 0.0;
    lin->fn(ctx, 1.0, nullptr, ox, oy, oz);
    TEST_ASSERT(std::abs(ox - 2.0) < 1e-9 && std::abs(oy - 3.0) < 1e-9);

    std::cout << "  -> Variations passed." << std::endl;
}

void test_flame_xml_roundtrip() {
    using namespace ApoNeo::Core;
    std::cout << "[Test] FlameXml Round-trip..." << std::endl;

    FlameGenome orig = FlameGenome::preset_swirl_flame();
    std::string xml = FlameXml::serialize_flame(orig);
    TEST_ASSERT(!xml.empty());

    FlameGenome parsed;
    std::string err;
    TEST_ASSERT(FlameXml::parse_single_flame(xml, parsed, &err));
    TEST_ASSERT(parsed.xforms.size() == orig.xforms.size());
    TEST_ASSERT(std::abs(parsed.scale - orig.scale) < 1e-5);

    std::cout << "  -> FlameXml Round-trip passed." << std::endl;
}

void test_orthogonal_transforms() {
    using namespace ApoNeo::Core;
    std::cout << "[Test] Orthogonal Transforms..." << std::endl;

    // Rotation matrix + scaling
    Affine2D rot = Affine2D::rotation(0.78539816339); // 45 deg
    // Dot product of column 1 (a, d) and column 2 (b, e)
    double dot = rot.a * rot.b + rot.d * rot.e;
    TEST_ASSERT(std::abs(dot) < 1e-9);

    // Scaling
    Affine2D sc = rot * Affine2D::scaling(2.0, 1.5);
    double dot_sc = sc.a * sc.b + sc.d * sc.e;
    TEST_ASSERT(std::abs(dot_sc) < 1e-9);

    std::cout << "  -> Orthogonal Transforms passed." << std::endl;
}

void test_post_affine_and_pipeline() {
    using namespace ApoNeo::Core;
    std::cout << "[Test] Post-Affine & Math Pipeline..." << std::endl;

    Xform xf;
    // Pre-affine: scale by 2.0
    xf.affine = Affine2D::scaling(2.0, 2.0);
    // Variation: spherical weight = 1.0
    xf.set_variation("linear", 0.0);
    xf.set_variation("spherical", 1.0);
    // Post-affine: translate by (10.0, 5.0)
    xf.has_post_affine = true;
    xf.post_affine = Affine2D::translation(10.0, 5.0);
    xf.compile_variations();

    // Initial point (1.0, 0.0)
    // 1. Pre-affine: (1.0 * 2.0, 0.0 * 2.0) = (2.0, 0.0), r^2 = 4.0
    // 2. Spherical: (x / r^2, y / r^2) = (2.0 / 4.0, 0.0) = (0.5, 0.0)
    // 3. Post-affine: (0.5 + 10.0, 0.0 + 5.0) = (10.5, 5.0)
    double px = 1.0, py = 0.0, pz = 0.0;
    VariationContext ctx;
    xf.apply(ctx, px, py, pz, 0.5, 0.5);

    TEST_ASSERT(std::abs(px - 10.5) < 1e-6);
    TEST_ASSERT(std::abs(py - 5.0) < 1e-6);

    // Test parameter variables (julian with julian_power = 2, julian_dist = 1)
    Xform xf_julian;
    xf_julian.set_variation("linear", 0.0);
    xf_julian.set_variation("julian", 1.0);
    xf_julian.set_param("julian_power", 2.0);
    xf_julian.set_param("julian_dist", 1.0);
    xf_julian.compile_variations();
    
    px = 4.0; py = 0.0; pz = 0.0;
    xf_julian.apply(ctx, px, py, pz, 0.1, 0.2);
    // r = 4.0, power = 2, dist = 1 -> r^(1/2) = 2.0, theta = 0 -> (2.0, 0.0)
    TEST_ASSERT(std::abs(px - 2.0) < 1e-4);
    TEST_ASSERT(std::abs(py - 0.0) < 1e-4);

    std::cout << "  -> Post-Affine & Math Pipeline passed." << std::endl;
}

void test_flame_xml_variables_and_post() {
    using namespace ApoNeo::Core;
    std::cout << "[Test] FlameXml Variables & Post..." << std::endl;

    FlameGenome orig;
    orig.name = "ParametricTest";
    Xform xf;
    xf.weight = 1.0;
    xf.set_variation("linear", 0.5);
    xf.set_variation("julian", 1.0);
    xf.set_param("julian_power", 5.0);
    xf.set_param("julian_dist", -1.5);
    xf.has_post_affine = true;
    xf.post_affine = Affine2D(1.1, 0.2, 3.3, 0.4, 1.5, 6.7);
    orig.xforms.push_back(xf);

    std::string xml = FlameXml::serialize_flame(orig);
    TEST_ASSERT(xml.find("julian=\"1\"") != std::string::npos || xml.find("julian=\"1.0\"") != std::string::npos);
    TEST_ASSERT(xml.find("julian_power=\"5\"") != std::string::npos || xml.find("julian_power=\"5.0\"") != std::string::npos);
    TEST_ASSERT(xml.find("post=\"") != std::string::npos);

    FlameGenome parsed;
    std::string err;
    TEST_ASSERT(FlameXml::parse_single_flame(xml, parsed, &err));
    TEST_ASSERT(parsed.xforms.size() == 1);
    
    const auto& pxform = parsed.xforms[0];
    TEST_ASSERT(pxform.has_post_affine == true);
    TEST_ASSERT(std::abs(pxform.post_affine.a - 1.1) < 1e-5);
    TEST_ASSERT(std::abs(pxform.post_affine.c - 3.3) < 1e-5);
    TEST_ASSERT(std::abs(pxform.post_affine.f - 6.7) < 1e-5);
    TEST_ASSERT(std::abs(pxform.get_param("julian_power") - 5.0) < 1e-5);
    TEST_ASSERT(std::abs(pxform.get_param("julian_dist") - (-1.5)) < 1e-5);

    std::cout << "  -> FlameXml Variables & Post passed." << std::endl;
}

void test_random_flame_generator() {
    using namespace ApoNeo::Core;
    std::cout << "[Test] Random Flame Generator..." << std::endl;

    RandomFlameGenerator gen(42);
    auto flame = gen.generate("TestGen");
    TEST_ASSERT(!flame.name.empty());
    TEST_ASSERT(flame.xforms.size() >= 2 && flame.xforms.size() <= 4);
    TEST_ASSERT(flame.total_weight() > 0.0);
    TEST_ASSERT(!flame.cumulative_weights().empty());

    for (const auto& xf : flame.xforms) {
        TEST_ASSERT(!xf.variation_weights.empty());
        TEST_ASSERT(xf.weight > 0.0);
    }

    auto batch = gen.generate_batch(10, "Batch");
    TEST_ASSERT(batch.size() == 10);
    TEST_ASSERT(batch[0].name == "Batch 1");
    TEST_ASSERT(batch[9].name == "Batch 10");

    std::cout << "  -> Random Flame Generator passed." << std::endl;
}

void test_flame_interpolation() {
    using namespace ApoNeo::Core;
    std::cout << "[Test] FlameGenome Interpolation (flam3-animate)..." << std::endl;

    FlameGenome g1 = FlameGenome::preset_swirl_flame();
    FlameGenome g2 = FlameGenome::preset_julia_vortex();

    // t = 0.0 -> g1
    FlameGenome at_0 = FlameGenome::interpolate(g1, g2, 0.0);
    TEST_ASSERT(std::abs(at_0.scale - g1.scale) < 1e-6);
    TEST_ASSERT(std::abs(at_0.quality - g1.quality) < 1e-6);

    // t = 1.0 -> g2
    FlameGenome at_1 = FlameGenome::interpolate(g1, g2, 1.0);
    TEST_ASSERT(std::abs(at_1.scale - g2.scale) < 1e-6);
    TEST_ASSERT(std::abs(at_1.quality - g2.quality) < 1e-6);

    // t = 0.5 -> exact midpoint
    FlameGenome at_half = FlameGenome::interpolate(g1, g2, 0.5);
    double expected_scale = 0.5 * (g1.scale + g2.scale);
    TEST_ASSERT(std::abs(at_half.scale - expected_scale) < 1e-6);

    // Check color palette midpoint
    ColorRGBA c1 = g1.palette.get_color(100);
    ColorRGBA c2 = g2.palette.get_color(100);
    ColorRGBA c_half = at_half.palette.get_color(100);
    TEST_ASSERT(std::abs(c_half.r - 0.5f * (c1.r + c2.r)) < 1e-4f);
    TEST_ASSERT(std::abs(c_half.g - 0.5f * (c1.g + c2.g)) < 1e-4f);
    TEST_ASSERT(std::abs(c_half.b - 0.5f * (c1.b + c2.b)) < 1e-4f);

    std::cout << "  -> FlameGenome Interpolation passed." << std::endl;
}

void test_undo_redo_commands() {
    using namespace ApoNeo::Core;
    using namespace ApoNeo::Core::Commands;
    std::cout << "[Test] QUndoStack & Command Pattern..." << std::endl;

    FlameGenome genome = FlameGenome::preset_swirl_flame();
    QUndoStack stack;

    // Test 1: Change Xform Matrix
    Affine2D old_aff = genome.xforms[0].affine;
    Affine2D new_aff = Affine2D::scaling(3.0, 3.0);
    bool cb_called = false;

    stack.push(new ChangeXformMatrixCommand(&genome, 0, false, old_aff, new_aff, [&]() {
        cb_called = true;
    }));

    TEST_ASSERT(std::abs(genome.xforms[0].affine.a - 3.0) < 1e-6);
    TEST_ASSERT(cb_called);
    TEST_ASSERT(stack.canUndo());

    // Undo
    cb_called = false;
    stack.undo();
    TEST_ASSERT(std::abs(genome.xforms[0].affine.a - old_aff.a) < 1e-6);
    TEST_ASSERT(cb_called);

    // Redo
    stack.redo();
    TEST_ASSERT(std::abs(genome.xforms[0].affine.a - 3.0) < 1e-6);

    // Test 2: Variation Weight Command Merging
    stack.push(new ChangeVariationWeightCommand(&genome, 0, "swirl", 0.8, 1.2, [&]() {}));
    TEST_ASSERT(std::abs(genome.xforms[0].get_variation("swirl") - 1.2) < 1e-6);

    // Push another variation change on same xform and variation -> should merge
    int count_before = stack.count();
    stack.push(new ChangeVariationWeightCommand(&genome, 0, "swirl", 1.2, 1.5, [&]() {}));
    TEST_ASSERT(std::abs(genome.xforms[0].get_variation("swirl") - 1.5) < 1e-6);
    TEST_ASSERT(stack.count() == count_before); // Merged into single undo step

    stack.undo();
    TEST_ASSERT(std::abs(genome.xforms[0].get_variation("swirl") - 0.8) < 1e-6);

    // Test 3: Camera Command
    ChangeCameraCommand::CameraState old_cam{genome.scale, genome.center_x, genome.center_y, genome.rotate, genome.zoom};
    ChangeCameraCommand::CameraState new_cam{300.0, 1.5, -2.0, 0.5, 1.0};
    stack.push(new ChangeCameraCommand(&genome, old_cam, new_cam, [&]() {}));
    TEST_ASSERT(std::abs(genome.scale - 300.0) < 1e-6);
    TEST_ASSERT(std::abs(genome.center_x - 1.5) < 1e-6);

    stack.undo();
    TEST_ASSERT(std::abs(genome.scale - old_cam.scale) < 1e-6);

    std::cout << "  -> QUndoStack & Command Pattern passed." << std::endl;
}

int main() {
    std::cout << "=== Running Core Tests ===" << std::endl;
    test_affine2d();
    test_palette();
    test_variations();
    test_flame_xml_roundtrip();
    test_orthogonal_transforms();
    test_post_affine_and_pipeline();
    test_flame_xml_variables_and_post();
    test_random_flame_generator();
    test_flame_interpolation();
    test_undo_redo_commands();
    std::cout << "=== All Core Tests Passed! ===" << std::endl;
    return 0;
}
