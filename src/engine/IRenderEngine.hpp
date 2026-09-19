#pragma once

#include "core/FlameGenome.hpp"
#include <cstdint>
#include <functional>
#include <vector>

namespace ApoNeo::Engine {

struct RenderProgress {
    uint64_t total_samples = 0;
    uint64_t completed_samples = 0;
    double elapsed_seconds = 0.0;
    double iterations_per_second = 0.0;
    bool is_finished = false;

    double fraction() const noexcept {
        return total_samples > 0 ? static_cast<double>(completed_samples) / static_cast<double>(total_samples) : 0.0;
    }
};

using RenderCallback = std::function<void(const RenderProgress&)>;

class IRenderEngine {
public:
    virtual ~IRenderEngine() = default;

    /// @brief Update genome parameters to render
    virtual void set_genome(const Core::FlameGenome& genome) = 0;

    /// @brief Start asynchronous rendering
    virtual void start(RenderCallback progress_cb = nullptr) = 0;

    /// @brief Pause rendering
    virtual void pause() = 0;

    /// @brief Resume paused rendering
    virtual void resume() = 0;

    /// @brief Request immediate stop / abort of rendering
    virtual void stop() = 0;

    /// @brief Synchronously wait for rendering to finish
    virtual void wait_until_done() = 0;

    /// @brief Check if rendering worker threads are active
    virtual bool is_rendering() const = 0;

    /// @brief Query latest render statistics
    virtual RenderProgress get_progress() const = 0;

    /// @brief Retrieve tone-mapped HDR float RGBA image (w * h * 4 floats)
    virtual void get_image_rgba_float(std::vector<float>& out_buffer, int& out_w, int& out_h) = 0;

    /// @brief Retrieve tone-mapped 32-bit RGBA8888 image (w * h * 4 bytes) suitable for QImage
    virtual void get_image_rgba8888(std::vector<uint8_t>& out_buffer, int& out_w, int& out_h) = 0;
};

} // namespace ApoNeo::Engine
