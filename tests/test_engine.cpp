#include "core/FlameGenome.hpp"
#include "engine/CPUSIMDEngine.hpp"
#include <cassert>
#include <iostream>
#include <numeric>

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

    assert(!engine.is_rendering());
    auto progress = engine.get_progress();
    assert(progress.is_finished);
    assert(progress.completed_samples > 0);

    std::vector<uint8_t> rgba_buf;
    int w = 0, h = 0;
    engine.get_image_rgba8888(rgba_buf, w, h);

    assert(w == 128 && h == 128);
    assert(rgba_buf.size() == 128 * 128 * 4);

    // Verify some non-zero pixels exist (fractal rendered)
    uint64_t sum_colors = 0;
    for (uint8_t byte : rgba_buf) {
        sum_colors += byte;
    }
    assert(sum_colors > 0);

    std::cout << "  -> Render completed successfully! Total color sum: " << sum_colors
              << " (" << progress.iterations_per_second / 1e6 << " Miter/s)" << std::endl;
}

int main() {
    std::cout << "=== Running Engine Tests ===" << std::endl;
    test_engine_render();
    std::cout << "=== All Engine Tests Passed! ===" << std::endl;
    return 0;
}
