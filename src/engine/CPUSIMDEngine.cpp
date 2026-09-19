#include "CPUSIMDEngine.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>

namespace ApoNeo::Engine {

namespace {

// Fast Xoshiro256+ PRNG for thread workers
struct Xoshiro256Plus {
    uint64_t s[4];

    static inline uint64_t rotl(const uint64_t x, int k) {
        return (x << k) | (x >> (64 - k));
    }

    uint64_t next() {
        const uint64_t result = s[0] + s[3];
        const uint64_t t = s[1] << 17;
        s[2] ^= s[0];
        s[3] ^= s[1];
        s[1] ^= s[2];
        s[0] ^= s[3];
        s[2] ^= t;
        s[3] = rotl(s[3], 45);
        return result;
    }

    // Double in [0, 1)
    double next_double() {
        return (next() >> 11) * 0x1.0p-53;
    }

    void seed(uint64_t val) {
        // SplitMix64 to initialize state
        uint64_t z = val;
        for (int i = 0; i < 4; ++i) {
            z += 0x9e3779b97f4a7c15ULL;
            z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
            z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
            s[i] = z ^ (z >> 31);
        }
    }
};

} // namespace

CPUSIMDEngine::CPUSIMDEngine(size_t thread_count) {
    if (thread_count == 0) {
        m_thread_count = std::max(1u, std::thread::hardware_concurrency());
    } else {
        m_thread_count = thread_count;
    }
    m_genome = Core::FlameGenome::preset_swirl_flame();
}

CPUSIMDEngine::~CPUSIMDEngine() {
    stop();
}

void CPUSIMDEngine::set_genome(const Core::FlameGenome& genome) {
    std::lock_guard lock(m_state_mutex);
    m_genome = genome;
    m_genome.compile();
}

void CPUSIMDEngine::stop() {
    for (auto& w : m_workers) {
        w.request_stop();
    }
    m_workers.clear();
    m_is_rendering = false;
}

void CPUSIMDEngine::pause() {
    m_is_paused = true;
}

void CPUSIMDEngine::resume() {
    m_is_paused = false;
}

void CPUSIMDEngine::wait_until_done() {
    for (auto& w : m_workers) {
        if (w.joinable()) {
            w.join();
        }
    }
    m_workers.clear();
    m_is_rendering = false;
}

bool CPUSIMDEngine::is_rendering() const {
    return m_is_rendering.load();
}

RenderProgress CPUSIMDEngine::get_progress() const {
    RenderProgress p;
    p.total_samples = m_total_samples;
    p.completed_samples = m_completed_samples.load();
    if (m_is_rendering) {
        auto now = std::chrono::steady_clock::now();
        p.elapsed_seconds = std::chrono::duration<double>(now - m_start_time).count();
        if (p.elapsed_seconds > 0.0) {
            p.iterations_per_second = static_cast<double>(p.completed_samples) / p.elapsed_seconds;
        }
    }
    p.is_finished = !m_is_rendering.load() && (p.completed_samples >= p.total_samples);
    return p;
}

void CPUSIMDEngine::start(RenderCallback progress_cb) {
    stop();

    std::lock_guard lock(m_state_mutex);
    m_progress_callback = std::move(progress_cb);
    m_genome.compile();

    if (m_genome.xforms.empty()) {
        return;
    }

    const int ss = std::clamp(m_genome.supersample, 1, 4);
    m_out_w = std::max(64, m_genome.width);
    m_out_h = std::max(64, m_genome.height);
    m_super_w = m_out_w * ss;
    m_super_h = m_out_h * ss;

    const size_t total_super_pixels = static_cast<size_t>(m_super_w) * m_super_h;
    m_thread_accums.resize(m_thread_count);
    for (size_t t = 0; t < m_thread_count; ++t) {
        m_thread_accums[t].assign(total_super_pixels, AccumCell{});
    }
    m_master_accum.assign(total_super_pixels, AccumCell{});

    // Output buffers
    m_output_rgba_float.assign(static_cast<size_t>(m_out_w) * m_out_h * 4, 0.0f);
    m_output_rgba8888.assign(static_cast<size_t>(m_out_w) * m_out_h * 4, 0);

    // Total iterations
    m_total_samples = static_cast<uint64_t>(m_out_w * m_out_h * m_genome.quality);
    m_completed_samples = 0;
    m_is_rendering = true;
    m_is_paused = false;
    m_start_time = std::chrono::steady_clock::now();

    const uint64_t samples_per_thread = m_total_samples / m_thread_count;
    m_workers.clear();
    m_workers.reserve(m_thread_count);

    for (size_t t = 0; t < m_thread_count; ++t) {
        uint64_t thread_samples = (t == m_thread_count - 1)
            ? (m_total_samples - samples_per_thread * (m_thread_count - 1))
            : samples_per_thread;

        m_workers.emplace_back([this, t, thread_samples](std::stop_token st) {
            render_worker(st, t, thread_samples);
        });
    }
}

void CPUSIMDEngine::render_worker(std::stop_token stop_tok, size_t thread_idx, uint64_t samples_to_render) {
    Xoshiro256Plus rng;
    rng.seed(0x123456789ABCDEF0ULL + thread_idx * 0x9E3779B97F4A7C15ULL);

    auto& accum = m_thread_accums[thread_idx];
    const auto& xforms = m_genome.xforms;
    const size_t num_xforms = xforms.size();
    const auto& cum_weights = m_genome.cumulative_weights();
    const double total_w = m_genome.total_weight();
    const auto& palette = m_genome.palette;

    const int ss = std::clamp(m_genome.supersample, 1, 4);
    const int sw = m_super_w;
    const int sh = m_super_h;

    // Viewport transform constants
    const double cos_rot = std::cos(m_genome.rotate);
    const double sin_rot = std::sin(m_genome.rotate);
    const double zoom_scale = m_genome.scale * std::exp(m_genome.zoom) * ss;
    const double center_x = m_genome.center_x;
    const double center_y = m_genome.center_y;
    const double half_sw = sw * 0.5;
    const double half_sh = sh * 0.5;

    // Initial state
    double px = rng.next_double() * 2.0 - 1.0;
    double py = rng.next_double() * 2.0 - 1.0;
    double pz = 0.0;
    double color = rng.next_double();

    Core::VariationContext ctx;
    Core::VariationContext final_ctx;

    // 1. Fuse / Warmup iterations to settle on the attractor
    for (int f = 0; f < m_genome.fuse_iterations; ++f) {
        double r_pick = rng.next_double() * total_w;
        auto it = std::upper_bound(cum_weights.begin(), cum_weights.end(), r_pick);
        size_t xf_idx = std::clamp<size_t>(std::distance(cum_weights.begin(), it), 0, num_xforms - 1);

        const auto& xf = xforms[xf_idx];
        xf.apply(ctx, px, py, pz, rng.next_double(), rng.next_double());
        color = (color + xf.color_index) * 0.5;
    }

    // 2. Iteration batch loop
    constexpr uint64_t BATCH_SIZE = 4096;
    uint64_t completed = 0;

    while (completed < samples_to_render && !stop_tok.stop_requested()) {
        if (m_is_paused.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        uint64_t batch = std::min(BATCH_SIZE, samples_to_render - completed);
        for (uint64_t i = 0; i < batch; ++i) {
            // Select transform
            double r_pick = rng.next_double() * total_w;
            auto it = std::upper_bound(cum_weights.begin(), cum_weights.end(), r_pick);
            size_t xf_idx = std::clamp<size_t>(std::distance(cum_weights.begin(), it), 0, num_xforms - 1);

            const auto& xf = xforms[xf_idx];
            xf.apply(ctx, px, py, pz, rng.next_double(), rng.next_double());

            // Blend color
            color = (1.0 - xf.color_speed) * color + xf.color_speed * xf.color_index;

            // Optional final transform
            double plot_x = px;
            double plot_y = py;
            double plot_z = pz;
            if (m_genome.has_final_xform) {
                m_genome.final_xform.apply(final_ctx, plot_x, plot_y, plot_z, rng.next_double(), rng.next_double());
            }

            // Map flame world coordinates to raster canvas
            double dx = plot_x - center_x;
            double dy = plot_y - center_y;
            double rx = dx * cos_rot - dy * sin_rot;
            double ry = dx * sin_rot + dy * cos_rot;

            int sx = static_cast<int>(rx * zoom_scale + half_sw);
            // Invert Y so positive Y is upwards
            int sy = static_cast<int>(-ry * zoom_scale + half_sh);

            if (sx >= 0 && sx < sw && sy >= 0 && sy < sh) {
                Core::ColorRGBA col = palette.sample(color);
                size_t cell_idx = static_cast<size_t>(sy) * sw + sx;
                accum[cell_idx].add(col.r, col.g, col.b, 1.0f);
            }
        }

        completed += batch;
        m_completed_samples.fetch_add(batch, std::memory_order_relaxed);
    }

    // If last thread finished, merge and tonemap
    if (m_completed_samples.load() >= m_total_samples) {
        m_is_rendering = false;
        merge_accumulators();
        tone_map_master_buffer();
    }
}

void CPUSIMDEngine::merge_accumulators() {
    std::lock_guard lock(m_state_mutex);
    const size_t total_cells = m_master_accum.size();
    std::fill(m_master_accum.begin(), m_master_accum.end(), AccumCell{});

    for (size_t t = 0; t < m_thread_count; ++t) {
        const auto& t_accum = m_thread_accums[t];
        for (size_t i = 0; i < total_cells; ++i) {
            m_master_accum[i].r += t_accum[i].r;
            m_master_accum[i].g += t_accum[i].g;
            m_master_accum[i].b += t_accum[i].b;
            m_master_accum[i].density += t_accum[i].density;
        }
    }
}

void CPUSIMDEngine::tone_map_master_buffer() {
    std::lock_guard lock(m_state_mutex);
    if (m_out_w <= 0 || m_out_h <= 0) return;

    const int ss = std::clamp(m_genome.supersample, 1, 4);
    const double ss_sq = ss * ss;
    const double gamma = std::max(0.1, m_genome.tone_map.gamma);
    const double inv_gamma = 1.0 / gamma;
    const double brightness = std::max(0.01, m_genome.tone_map.brightness);
    const double vibrancy = std::clamp(m_genome.tone_map.vibrancy, 0.0, 1.0);
    const auto& bg = m_genome.background;

    // Find maximum density for scaling
    float max_density = 0.0f;
    for (const auto& c : m_master_accum) {
        if (c.density > max_density) {
            max_density = c.density;
        }
    }

    if (max_density < 1.0f) max_density = 1.0f;

    for (int y = 0; y < m_out_h; ++y) {
        for (int x = 0; x < m_out_w; ++x) {
            float sum_r = 0.0f;
            float sum_g = 0.0f;
            float sum_b = 0.0f;
            float sum_dens = 0.0f;

            // Box / spatial downsampling across supersampling grid
            for (int sy = 0; sy < ss; ++sy) {
                int src_y = y * ss + sy;
                for (int sx = 0; sx < ss; ++sx) {
                    int src_x = x * ss + sx;
                    size_t idx = static_cast<size_t>(src_y) * m_super_w + src_x;
                    const auto& cell = m_master_accum[idx];
                    sum_r += cell.r;
                    sum_g += cell.g;
                    sum_b += cell.b;
                    sum_dens += cell.density;
                }
            }

            float out_r = bg.r;
            float out_g = bg.g;
            float out_b = bg.b;
            float out_a = 1.0f;

            if (sum_dens > 0.0f) {
                // Average color
                float avg_r = sum_r / sum_dens;
                float avg_g = sum_g / sum_dens;
                float avg_b = sum_b / sum_dens;

                // Log-density tone mapping
                double norm_dens = static_cast<double>(sum_dens) / ss_sq;
                double log_factor = std::log10(1.0 + norm_dens * brightness * 100.0 / max_density);
                double alpha = std::pow(log_factor, inv_gamma);

                // Apply vibrancy
                float lum = 0.299f * avg_r + 0.587f * avg_g + 0.114f * avg_b;
                float vr = static_cast<float>(vibrancy * avg_r + (1.0 - vibrancy) * lum);
                float vg = static_cast<float>(vibrancy * avg_g + (1.0 - vibrancy) * lum);
                float vb = static_cast<float>(vibrancy * avg_b + (1.0 - vibrancy) * lum);

                // Blend with background
                float blend = std::clamp(static_cast<float>(alpha), 0.0f, 1.0f);
                out_r = vr * blend + bg.r * (1.0f - blend);
                out_g = vg * blend + bg.g * (1.0f - blend);
                out_b = vb * blend + bg.b * (1.0f - blend);
                out_a = blend;
            }

            size_t out_idx = (static_cast<size_t>(y) * m_out_w + x) * 4;
            m_output_rgba_float[out_idx + 0] = out_r;
            m_output_rgba_float[out_idx + 1] = out_g;
            m_output_rgba_float[out_idx + 2] = out_b;
            m_output_rgba_float[out_idx + 3] = out_a;

            m_output_rgba8888[out_idx + 0] = static_cast<uint8_t>(std::clamp(out_r * 255.0f, 0.0f, 255.0f));
            m_output_rgba8888[out_idx + 1] = static_cast<uint8_t>(std::clamp(out_g * 255.0f, 0.0f, 255.0f));
            m_output_rgba8888[out_idx + 2] = static_cast<uint8_t>(std::clamp(out_b * 255.0f, 0.0f, 255.0f));
            m_output_rgba8888[out_idx + 3] = static_cast<uint8_t>(std::clamp(out_a * 255.0f, 0.0f, 255.0f));
        }
    }
}

void CPUSIMDEngine::get_image_rgba_float(std::vector<float>& out_buffer, int& out_w, int& out_h) {
    if (m_is_rendering) {
        merge_accumulators();
        tone_map_master_buffer();
    }
    std::lock_guard lock(m_state_mutex);
    out_w = m_out_w;
    out_h = m_out_h;
    out_buffer = m_output_rgba_float;
}

void CPUSIMDEngine::get_image_rgba8888(std::vector<uint8_t>& out_buffer, int& out_w, int& out_h) {
    if (m_is_rendering) {
        merge_accumulators();
        tone_map_master_buffer();
    }
    std::lock_guard lock(m_state_mutex);
    out_w = m_out_w;
    out_h = m_out_h;
    out_buffer = m_output_rgba8888;
}

} // namespace ApoNeo::Engine
