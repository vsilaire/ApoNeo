#pragma once

#include "AnimationWidget.hpp"
#include "core/FlameGenome.hpp"
#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <atomic>
#include <memory>
#include <thread>

namespace ApoNeo::UI {

class ExportDialog : public QDialog {
    Q_OBJECT

public:
    explicit ExportDialog(const AnimationWidget* anim_widget, QWidget* parent = nullptr);
    ~ExportDialog() override;

private slots:
    void on_preset_changed(int index);
    void on_browse_output_dir();
    void on_start_export();
    void on_cancel_export();
    void on_render_progress(int current_frame, int total_frames, double elapsed_sec);
    void on_render_finished(bool success, const QString& message);

signals:
    void render_progress_signal(int current_frame, int total_frames, double elapsed_sec);
    void render_finished_signal(bool success, const QString& message);

private:
    void setup_ui();
    void render_worker_thread(int width, int height, int supersample, double quality,
                             int total_frames, int fps, bool is_video,
                             const QString& out_dir, const QString& base_name);

    const AnimationWidget* m_anim_widget = nullptr;

    // UI elements
    QComboBox* m_combo_preset = nullptr;
    QSpinBox* m_spin_width = nullptr;
    QSpinBox* m_spin_height = nullptr;
    QSpinBox* m_spin_supersample = nullptr;
    QSpinBox* m_spin_quality = nullptr;
    QComboBox* m_combo_format = nullptr;
    QLineEdit* m_edit_out_dir = nullptr;
    QLineEdit* m_edit_base_name = nullptr;
    QPushButton* m_btn_browse = nullptr;

    QProgressBar* m_progress_bar = nullptr;
    QLabel* m_lbl_status = nullptr;
    QLabel* m_lbl_eta = nullptr;
    QPushButton* m_btn_start = nullptr;
    QPushButton* m_btn_cancel = nullptr;

    std::atomic<bool> m_cancel_requested{false};
    std::unique_ptr<std::jthread> m_worker_thread;
};

} // namespace ApoNeo::UI
