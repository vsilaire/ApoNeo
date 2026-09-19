#include "AnimationWidget.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace ApoNeo::UI {

AnimationWidget::AnimationWidget(QWidget* parent)
    : QWidget(parent) {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(8, 6, 8, 6);
    main_layout->setSpacing(6);

    // Top Row: Keyframe management and Export button
    auto* top_row = new QHBoxLayout();
    top_row->setSpacing(8);

    m_btn_add_kf = new QPushButton(tr("➕ Add Keyframe"), this);
    m_btn_add_kf->setToolTip(tr("Add current flame state as a keyframe at the playhead position"));
    connect(m_btn_add_kf, &QPushButton::clicked, this, &AnimationWidget::on_btn_add_keyframe);
    top_row->addWidget(m_btn_add_kf);

    m_btn_rem_kf = new QPushButton(tr("➖ Delete"), this);
    connect(m_btn_rem_kf, &QPushButton::clicked, this, &AnimationWidget::on_btn_remove_keyframe);
    top_row->addWidget(m_btn_rem_kf);

    m_btn_clear_kf = new QPushButton(tr("🗑 Clear All"), this);
    connect(m_btn_clear_kf, &QPushButton::clicked, this, &AnimationWidget::on_btn_clear_keyframes);
    top_row->addWidget(m_btn_clear_kf);

    m_list_keyframes = new QListWidget(this);
    m_list_keyframes->setFixedHeight(36);
    m_list_keyframes->setFlow(QListView::LeftToRight);
    m_list_keyframes->setWrapping(false);
    m_list_keyframes->setStyleSheet(
        "QListWidget {"
        "  background: #141416;"
        "  border: 1px solid #232328;"
        "  border-radius: 4px;"
        "  color: #e0e0e0;"
        "}"
        "QListWidget::item {"
        "  background: #1e1e24;"
        "  border-radius: 3px;"
        "  padding: 4px 8px;"
        "  margin-right: 4px;"
        "}"
        "QListWidget::item:selected {"
        "  background: #7928ca;"
        "  color: #ffffff;"
        "}"
    );
    connect(m_list_keyframes, &QListWidget::currentRowChanged, this, &AnimationWidget::on_keyframe_list_selected);
    top_row->addWidget(m_list_keyframes, 1);

    m_btn_export = new QPushButton(tr("🎬 Export Animation..."), this);
    m_btn_export->setStyleSheet(
        "QPushButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #0070f3, stop:1 #00dfd8);"
        "  color: #ffffff;"
        "  font-weight: 600;"
        "  padding: 6px 14px;"
        "  border-radius: 6px;"
        "  border: none;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #1a82ff, stop:1 #26e6e0);"
        "}"
    );
    connect(m_btn_export, &QPushButton::clicked, this, [this]() { emit request_export(); });
    top_row->addWidget(m_btn_export);

    main_layout->addLayout(top_row);

    // Middle Row: Timeline Slider
    auto* slider_row = new QHBoxLayout();
    slider_row->setSpacing(8);

    m_slider_frame = new QSlider(Qt::Horizontal, this);
    m_slider_frame->setRange(0, m_total_frames - 1);
    m_slider_frame->setValue(0);
    m_slider_frame->setStyleSheet(
        "QSlider::groove:horizontal {"
        "  height: 6px;"
        "  background: #232328;"
        "  border-radius: 3px;"
        "}"
        "QSlider::sub-page:horizontal {"
        "  background: #7928ca;"
        "  border-radius: 3px;"
        "}"
        "QSlider::handle:horizontal {"
        "  background: #ff0080;"
        "  border: 1px solid #ffffff;"
        "  width: 14px;"
        "  margin-top: -4px;"
        "  margin-bottom: -4px;"
        "  border-radius: 7px;"
        "}"
    );
    connect(m_slider_frame, &QSlider::valueChanged, this, &AnimationWidget::on_slider_moved);
    slider_row->addWidget(m_slider_frame, 1);

    main_layout->addLayout(slider_row);

    // Bottom Row: Transport controls & metrics
    auto* transport_row = new QHBoxLayout();
    transport_row->setSpacing(10);

    m_btn_play = new QPushButton(tr("▶ Play"), this);
    m_btn_play->setFixedWidth(75);
    connect(m_btn_play, &QPushButton::clicked, this, &AnimationWidget::toggle_play);
    transport_row->addWidget(m_btn_play);

    m_btn_stop = new QPushButton(tr("⏹ Stop"), this);
    m_btn_stop->setFixedWidth(75);
    connect(m_btn_stop, &QPushButton::clicked, this, &AnimationWidget::stop);
    transport_row->addWidget(m_btn_stop);

    m_chk_loop = new QCheckBox(tr("🔁 Loop"), this);
    m_chk_loop->setChecked(true);
    transport_row->addWidget(m_chk_loop);

    auto* sep1 = new QFrame(this);
    sep1->setFrameShape(QFrame::VLine);
    transport_row->addWidget(sep1);

    transport_row->addWidget(new QLabel(tr("Frame:"), this));
    m_spin_current_frame = new QSpinBox(this);
    m_spin_current_frame->setRange(0, m_total_frames - 1);
    m_spin_current_frame->setValue(0);
    connect(m_spin_current_frame, QOverload<int>::of(&QSpinBox::valueChanged), this, &AnimationWidget::on_spin_frame_changed);
    transport_row->addWidget(m_spin_current_frame);

    transport_row->addWidget(new QLabel(tr("/"), this));
    m_spin_total_frames = new QSpinBox(this);
    m_spin_total_frames->setRange(10, 3600);
    m_spin_total_frames->setValue(m_total_frames);
    connect(m_spin_total_frames, QOverload<int>::of(&QSpinBox::valueChanged), this, &AnimationWidget::on_total_frames_changed);
    transport_row->addWidget(m_spin_total_frames);

    auto* sep2 = new QFrame(this);
    sep2->setFrameShape(QFrame::VLine);
    transport_row->addWidget(sep2);

    transport_row->addWidget(new QLabel(tr("FPS:"), this));
    m_combo_fps = new QComboBox(this);
    m_combo_fps->addItems({"24", "30", "60"});
    m_combo_fps->setCurrentText("30");
    connect(m_combo_fps, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AnimationWidget::on_fps_changed);
    transport_row->addWidget(m_combo_fps);

    m_lbl_time = new QLabel(this);
    m_lbl_time->setStyleSheet("color: #888899; font-family: monospace; font-size: 11px;");
    update_time_label();
    transport_row->addWidget(m_lbl_time);

    transport_row->addStretch(1);
    main_layout->addLayout(transport_row);

    connect(&m_play_timer, &QTimer::timeout, this, &AnimationWidget::on_timer_tick);
}

void AnimationWidget::add_keyframe(int frame, const Core::FlameGenome& genome) {
    // Check if keyframe already exists at this exact frame
    auto it = std::find_if(m_keyframes.begin(), m_keyframes.end(), [frame](const AnimationKeyframe& kf) {
        return kf.frame == frame;
    });

    if (it != m_keyframes.end()) {
        it->genome = genome;
    } else {
        m_keyframes.push_back({frame, genome});
        std::sort(m_keyframes.begin(), m_keyframes.end(), [](const AnimationKeyframe& a, const AnimationKeyframe& b) {
            return a.frame < b.frame;
        });
    }

    refresh_keyframe_list();
}

void AnimationWidget::refresh_keyframe_list() {
    m_list_keyframes->clear();
    for (const auto& kf : m_keyframes) {
        QString label = QString("KF @ %1f (%2)").arg(kf.frame).arg(QString::fromStdString(kf.genome.name));
        m_list_keyframes->addItem(label);
    }
}

Core::FlameGenome AnimationWidget::genome_at_frame(int frame) const {
    if (m_keyframes.empty()) {
        return Core::FlameGenome();
    }
    if (m_keyframes.size() == 1) {
        return m_keyframes.front().genome;
    }

    // If frame is before first keyframe
    if (frame <= m_keyframes.front().frame) {
        return m_keyframes.front().genome;
    }
    // If frame is after last keyframe
    if (frame >= m_keyframes.back().frame) {
        return m_keyframes.back().genome;
    }

    // Find bounding keyframes
    for (size_t i = 0; i + 1 < m_keyframes.size(); ++i) {
        if (frame >= m_keyframes[i].frame && frame <= m_keyframes[i + 1].frame) {
            int span = m_keyframes[i + 1].frame - m_keyframes[i].frame;
            double t = (span > 0) ? static_cast<double>(frame - m_keyframes[i].frame) / span : 0.0;
            return Core::FlameGenome::interpolate(m_keyframes[i].genome, m_keyframes[i + 1].genome, t);
        }
    }

    return m_keyframes.back().genome;
}

int AnimationWidget::total_frames() const {
    return m_total_frames;
}

int AnimationWidget::fps() const {
    return m_fps;
}

int AnimationWidget::current_frame() const {
    return m_current_frame;
}

void AnimationWidget::set_current_frame(int frame) {
    if (frame < 0) frame = 0;
    if (frame >= m_total_frames) frame = m_total_frames - 1;

    m_current_frame = frame;

    m_slider_frame->blockSignals(true);
    m_slider_frame->setValue(m_current_frame);
    m_slider_frame->blockSignals(false);

    m_spin_current_frame->blockSignals(true);
    m_spin_current_frame->setValue(m_current_frame);
    m_spin_current_frame->blockSignals(false);

    update_time_label();

    if (!m_keyframes.empty()) {
        Core::FlameGenome interpolated = genome_at_frame(m_current_frame);
        emit frame_rendered(interpolated, m_current_frame);
    }
}

void AnimationWidget::play() {
    if (m_is_playing) return;
    m_is_playing = true;
    m_btn_play->setText(tr("⏸ Pause"));
    m_play_timer.start(1000 / m_fps);
}

void AnimationWidget::pause() {
    if (!m_is_playing) return;
    m_is_playing = false;
    m_btn_play->setText(tr("▶ Play"));
    m_play_timer.stop();
}

void AnimationWidget::stop() {
    pause();
    set_current_frame(0);
}

void AnimationWidget::toggle_play() {
    if (m_is_playing) {
        pause();
    } else {
        play();
    }
}

void AnimationWidget::on_timer_tick() {
    int next_frame = m_current_frame + 1;
    if (next_frame >= m_total_frames) {
        if (m_chk_loop->isChecked()) {
            next_frame = 0;
        } else {
            pause();
            return;
        }
    }
    set_current_frame(next_frame);
}

void AnimationWidget::on_slider_moved(int frame) {
    set_current_frame(frame);
}

void AnimationWidget::on_spin_frame_changed(int frame) {
    set_current_frame(frame);
}

void AnimationWidget::on_total_frames_changed(int total) {
    m_total_frames = std::max(1, total);
    m_slider_frame->setRange(0, m_total_frames - 1);
    m_spin_current_frame->setRange(0, m_total_frames - 1);
    update_time_label();
}

void AnimationWidget::on_fps_changed(int index) {
    (void)index;
    m_fps = m_combo_fps->currentText().toInt();
    if (m_fps <= 0) m_fps = 30;
    if (m_is_playing) {
        m_play_timer.setInterval(1000 / m_fps);
    }
    update_time_label();
}

void AnimationWidget::on_btn_add_keyframe() {
    emit request_add_current_keyframe();
}

void AnimationWidget::on_btn_remove_keyframe() {
    int row = m_list_keyframes->currentRow();
    if (row >= 0 && row < static_cast<int>(m_keyframes.size())) {
        m_keyframes.erase(m_keyframes.begin() + row);
        refresh_keyframe_list();
    }
}

void AnimationWidget::on_btn_clear_keyframes() {
    m_keyframes.clear();
    refresh_keyframe_list();
}

void AnimationWidget::on_keyframe_list_selected(int row) {
    if (row >= 0 && row < static_cast<int>(m_keyframes.size())) {
        set_current_frame(m_keyframes[row].frame);
    }
}

void AnimationWidget::update_time_label() {
    double cur_sec = static_cast<double>(m_current_frame) / std::max(1, m_fps);
    double tot_sec = static_cast<double>(m_total_frames) / std::max(1, m_fps);

    int cur_m = static_cast<int>(cur_sec) / 60;
    int cur_s = static_cast<int>(cur_sec) % 60;
    int cur_ms = static_cast<int>((cur_sec - std::floor(cur_sec)) * 100);

    int tot_m = static_cast<int>(tot_sec) / 60;
    int tot_s = static_cast<int>(tot_sec) % 60;
    int tot_ms = static_cast<int>((tot_sec - std::floor(tot_sec)) * 100);

    char buf[64];
    std::snprintf(buf, sizeof(buf), "%02d:%02d.%02d / %02d:%02d.%02d",
                  cur_m, cur_s, cur_ms, tot_m, tot_s, tot_ms);
    m_lbl_time->setText(QString::fromUtf8(buf));
}

} // namespace ApoNeo::UI
