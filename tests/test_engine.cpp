#include "core/FlameGenome.hpp"
#include "engine/CPUSIMDEngine.hpp"
#include <cstdlib>
#include <iostream>
#include <numeric>

#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed: " #cond " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::abort(); \
        } \
    } while (0)

void test_engine_render() {
    using namespace ApoNeo;
    std::cout << "[Test] CPUSIMDEngine render execution..." << std::endl;

    Engine::CPUSIMDEngine engine(4); // 4 threads

    Core::FlameGenome g = Core::FlameGenome::preset_sierpinski();
    g.width = 128;
    g.height = 128;
    g.quality = 20.0;
    g.supersample = 2;

    engine.set_genome(g);
    engine.start();
    engine.wait_until_done();

    TEST_ASSERT(!engine.is_rendering());
    auto progress = engine.get_progress();
    TEST_ASSERT(progress.is_finished);
    TEST_ASSERT(progress.completed_samples > 0);

    std::vector<uint8_t> rgba_buf;
    int w = 0, h = 0;
    engine.get_image_rgba8888(rgba_buf, w, h);

    TEST_ASSERT(w == 128 && h == 128);
    TEST_ASSERT(rgba_buf.size() == 128 * 128 * 4);

    // Verify some non-zero pixels exist (fractal rendered)
    uint64_t sum_colors = 0;
    for (uint8_t byte : rgba_buf) {
        sum_colors += byte;
    }
    TEST_ASSERT(sum_colors > 0);

    std::cout << "  -> Render completed successfully! Total color sum: " << sum_colors
              << " (" << progress.iterations_per_second / 1e6 << " Miter/s)" << std::endl;
}

int main() {
    std::cout << "=== Running Engine Tests ===" << std::endl;
    test_engine_render();
    std::cout << "=== All Engine Tests Passed! ===" << std::endl;
    return 0;
}
