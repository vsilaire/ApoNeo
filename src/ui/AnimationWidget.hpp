#pragma once

#include "core/FlameGenome.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <vector>

namespace ApoNeo::UI {

struct AnimationKeyframe {
    int frame = 0;
    Core::FlameGenome genome;
};

class AnimationWidget : public QWidget {
    Q_OBJECT

public:
    explicit AnimationWidget(QWidget* parent = nullptr);
    ~AnimationWidget() override = default;

    /// @brief Add a keyframe at the current playhead frame using the given genome
    void add_keyframe(int frame, const Core::FlameGenome& genome);

    /// @brief Get all keyframes
    const std::vector<AnimationKeyframe>& keyframes() const { return m_keyframes; }

    /// @brief Compute the interpolated genome for any given frame
    Core::FlameGenome genome_at_frame(int frame) const;

    int total_frames() const;
    int fps() const;
    int current_frame() const;

public slots:
    void set_current_frame(int frame);
    void play();
    void pause();
    void stop();
    void toggle_play();

signals:
    void frame_rendered(const Core::FlameGenome& genome, int frame);
    void request_add_current_keyframe();
    void request_export();

private slots:
    void on_timer_tick();
    void on_slider_moved(int frame);
    void on_spin_frame_changed(int frame);
    void on_total_frames_changed(int total);
    void on_fps_changed(int index);
    void on_btn_add_keyframe();
    void on_btn_remove_keyframe();
    void on_btn_clear_keyframes();
    void on_keyframe_list_selected(int row);

private:
    void update_time_label();
    void refresh_keyframe_list();

    std::vector<AnimationKeyframe> m_keyframes;
    QTimer m_play_timer;
    int m_current_frame = 0;
    int m_total_frames = 120;
    int m_fps = 30;
    bool m_is_playing = false;

    // Controls
    QPushButton* m_btn_play = nullptr;
    QPushButton* m_btn_stop = nullptr;
    QCheckBox* m_chk_loop = nullptr;
    QComboBox* m_combo_fps = nullptr;
    QSpinBox* m_spin_total_frames = nullptr;
    QSpinBox* m_spin_current_frame = nullptr;
    QSlider* m_slider_frame = nullptr;
    QLabel* m_lbl_time = nullptr;

    QPushButton* m_btn_add_kf = nullptr;
    QPushButton* m_btn_rem_kf = nullptr;
    QPushButton* m_btn_clear_kf = nullptr;
    QPushButton* m_btn_export = nullptr;
    QListWidget* m_list_keyframes = nullptr;
};

} // namespace ApoNeo::UI
