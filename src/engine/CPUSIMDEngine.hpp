#pragma once

#include "IRenderEngine.hpp"
#include "core/FlameGenome.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <stop_token>
#include <thread>
#include <vector>

namespace ApoNeo::Engine {

struct AccumCell {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float density = 0.0f;

    void add(float ar, float ag, float ab, float ad = 1.0f) noexcept {
        r += ar;
        g += ag;
        b += ab;
        density += ad;
    }
};

class CPUSIMDEngine : public IRenderEngine {
public:
    explicit CPUSIMDEngine(size_t thread_count = 0);
    ~CPUSIMDEngine() override;

    void set_genome(const Core::FlameGenome& genome) override;
    void start(RenderCallback progress_cb = nullptr) override;
    void pause() override;
    void resume() override;
    void stop() override;
    void wait_until_done() override;
    bool is_rendering() const override;
    RenderProgress get_progress() const override;

    void get_image_rgba_float(std::vector<float>& out_buffer, int& out_w, int& out_h) override;
    void get_image_rgba8888(std::vector<uint8_t>& out_buffer, int& out_w, int& out_h) override;

private:
    void render_worker(std::stop_token stop_tok, size_t thread_idx, uint64_t samples_to_render);
    void merge_accumulators();
    void tone_map_master_buffer();

    size_t m_thread_count;
    Core::FlameGenome m_genome;
    mutable std::mutex m_state_mutex;

    std::vector<std::jthread> m_workers;
    std::atomic<bool> m_is_rendering{false};
    std::atomic<bool> m_is_paused{false};
    std::atomic<uint64_t> m_completed_samples{0};
    uint64_t m_total_samples{0};
    std::chrono::steady_clock::time_point m_start_time;

    // Per-thread accumulator buffers: prevents false sharing & lock contention
    std::vector<std::vector<AccumCell>> m_thread_accums;
    // Master merged accumulator buffer
    std::vector<AccumCell> m_master_accum;
    int m_super_w = 0;
    int m_super_h = 0;

    // Processed tone-mapped output buffers
    std::vector<float> m_output_rgba_float;
    std::vector<uint8_t> m_output_rgba8888;
    int m_out_w = 0;
    int m_out_h = 0;

    RenderCallback m_progress_callback;
};

} // namespace ApoNeo::Engine
