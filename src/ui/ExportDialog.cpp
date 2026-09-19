#include "ExportDialog.hpp"
#include "engine/CPUSIMDEngine.hpp"
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QImage>
#include <QMessageBox>
#include <QProcess>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <chrono>
#include <cmath>

namespace ApoNeo::UI {

ExportDialog::ExportDialog(const AnimationWidget* anim_widget, QWidget* parent)
    : QDialog(parent), m_anim_widget(anim_widget) {
    setWindowTitle(tr("Export Animation (flam3-animate)"));
    setMinimumWidth(520);
    setup_ui();

    connect(this, &ExportDialog::render_progress_signal, this, &ExportDialog::on_render_progress, Qt::QueuedConnection);
    connect(this, &ExportDialog::render_finished_signal, this, &ExportDialog::on_render_finished, Qt::QueuedConnection);
}

ExportDialog::~ExportDialog() {
    m_cancel_requested = true;
    if (m_worker_thread && m_worker_thread->joinable()) {
        m_worker_thread->join();
    }
}

void ExportDialog::setup_ui() {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setSpacing(12);

    auto* form_layout = new QFormLayout();
    form_layout->setLabelAlignment(Qt::AlignRight);
    form_layout->setSpacing(8);

    // Preset
    m_combo_preset = new QComboBox(this);
    m_combo_preset->addItems({tr("720p HD (1280x720)"), tr("1080p Full HD (1920x1080)"), tr("4K UHD (3840x2160)"), tr("Custom")});
    m_combo_preset->setCurrentIndex(0);
    connect(m_combo_preset, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ExportDialog::on_preset_changed);
    form_layout->addRow(tr("Resolution Preset:"), m_combo_preset);

    // Dimensions
    auto* dim_layout = new QHBoxLayout();
    m_spin_width = new QSpinBox(this);
    m_spin_width->setRange(160, 7680);
    m_spin_width->setValue(1280);
    dim_layout->addWidget(m_spin_width);
    dim_layout->addWidget(new QLabel("x", this));
    m_spin_height = new QSpinBox(this);
    m_spin_height->setRange(120, 4320);
    m_spin_height->setValue(720);
    dim_layout->addWidget(m_spin_height);
    form_layout->addRow(tr("Dimensions:"), dim_layout);

    // Quality & Supersample
    m_spin_supersample = new QSpinBox(this);
    m_spin_supersample->setRange(1, 3);
    m_spin_supersample->setValue(1);
    form_layout->addRow(tr("Spatial Supersample:"), m_spin_supersample);

    m_spin_quality = new QSpinBox(this);
    m_spin_quality->setRange(10, 2000);
    m_spin_quality->setValue(50);
    form_layout->addRow(tr("Sample Density (Quality):"), m_spin_quality);

    // Format
    m_combo_format = new QComboBox(this);
    m_combo_format->addItems({tr("MP4 Video (H.264 / FFmpeg)"), tr("PNG Frame Sequence")});
    form_layout->addRow(tr("Output Format:"), m_combo_format);

    // Output Directory
    auto* dir_layout = new QHBoxLayout();
    m_edit_out_dir = new QLineEdit(this);
    QString default_dir = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    if (default_dir.isEmpty()) {
        default_dir = QDir::homePath();
    }
    m_edit_out_dir->setText(default_dir);
    dir_layout->addWidget(m_edit_out_dir, 1);
    m_btn_browse = new QPushButton(tr("Browse..."), this);
    connect(m_btn_browse, &QPushButton::clicked, this, &ExportDialog::on_browse_output_dir);
    dir_layout->addWidget(m_btn_browse);
    form_layout->addRow(tr("Output Directory:"), dir_layout);

    // Base Name
    m_edit_base_name = new QLineEdit(this);
    m_edit_base_name->setText("aponeo_animation");
    form_layout->addRow(tr("Filename Prefix:"), m_edit_base_name);

    main_layout->addLayout(form_layout);

    // Progress Section
    m_progress_bar = new QProgressBar(this);
    m_progress_bar->setRange(0, 100);
    m_progress_bar->setValue(0);
    m_progress_bar->setStyleSheet(
        "QProgressBar {"
        "  background: #141416;"
        "  border: 1px solid #232328;"
        "  border-radius: 4px;"
        "  text-align: center;"
        "  color: #ffffff;"
        "}"
        "QProgressBar::chunk {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #0070f3, stop:1 #00dfd8);"
        "  border-radius: 3px;"
        "}"
    );
    main_layout->addWidget(m_progress_bar);

    auto* status_layout = new QHBoxLayout();
    m_lbl_status = new QLabel(tr("Ready to export."), this);
    m_lbl_status->setStyleSheet("color: #a0a0b0;");
    status_layout->addWidget(m_lbl_status);

    m_lbl_eta = new QLabel(this);
    m_lbl_eta->setStyleSheet("color: #a0a0b0; font-family: monospace;");
    status_layout->addWidget(m_lbl_eta, 0, Qt::AlignRight);
    main_layout->addLayout(status_layout);

    // Action Buttons
    auto* btn_layout = new QHBoxLayout();
    btn_layout->addStretch(1);

    m_btn_start = new QPushButton(tr("🚀 Start Export"), this);
    m_btn_start->setStyleSheet(
        "QPushButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #7928ca, stop:1 #ff0080);"
        "  color: #ffffff;"
        "  font-weight: 600;"
        "  padding: 8px 18px;"
        "  border-radius: 6px;"
        "  border: none;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #903fe0, stop:1 #ff2e9a);"
        "}"
    );
    connect(m_btn_start, &QPushButton::clicked, this, &ExportDialog::on_start_export);
    btn_layout->addWidget(m_btn_start);

    m_btn_cancel = new QPushButton(tr("Cancel"), this);
    connect(m_btn_cancel, &QPushButton::clicked, this, &ExportDialog::on_cancel_export);
    btn_layout->addWidget(m_btn_cancel);

    main_layout->addLayout(btn_layout);
}

void ExportDialog::on_preset_changed(int index) {
    if (index == 0) {
        m_spin_width->setValue(1280);
        m_spin_height->setValue(720);
    } else if (index == 1) {
        m_spin_width->setValue(1920);
        m_spin_height->setValue(1080);
    } else if (index == 2) {
        m_spin_width->setValue(3840);
        m_spin_height->setValue(2160);
    }
}

void ExportDialog::on_browse_output_dir() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Output Directory"), m_edit_out_dir->text());
    if (!dir.isEmpty()) {
        m_edit_out_dir->setText(dir);
    }
}

void ExportDialog::on_start_export() {
    if (!m_anim_widget || m_anim_widget->keyframes().empty()) {
        QMessageBox::warning(this, tr("No Keyframes"), tr("Please add at least one keyframe to the timeline before exporting."));
        return;
    }

    m_cancel_requested = false;
    m_btn_start->setEnabled(false);
    m_btn_browse->setEnabled(false);
    m_combo_preset->setEnabled(false);
    m_spin_width->setEnabled(false);
    m_spin_height->setEnabled(false);
    m_spin_supersample->setEnabled(false);
    m_spin_quality->setEnabled(false);
    m_combo_format->setEnabled(false);

    int w = m_spin_width->value();
    int h = m_spin_height->value();
    int ss = m_spin_supersample->value();
    double qual = static_cast<double>(m_spin_quality->value());
    int total_frames = m_anim_widget->total_frames();
    int fps = m_anim_widget->fps();
    bool is_video = (m_combo_format->currentIndex() == 0);
    QString out_dir = m_edit_out_dir->text();
    QString base_name = m_edit_base_name->text();

    m_worker_thread = std::make_unique<std::jthread>([this, w, h, ss, qual, total_frames, fps, is_video, out_dir, base_name]() {
        render_worker_thread(w, h, ss, qual, total_frames, fps, is_video, out_dir, base_name);
    });
}

void ExportDialog::on_cancel_export() {
    if (m_worker_thread && m_worker_thread->joinable()) {
        m_cancel_requested = true;
        m_lbl_status->setText(tr("Cancelling export..."));
    } else {
        reject();
    }
}

void ExportDialog::render_worker_thread(int width, int height, int supersample, double quality,
                                      int total_frames, int fps, bool is_video,
                                      const QString& out_dir, const QString& base_name) {
    QDir dir(out_dir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString render_dir = out_dir;
    if (is_video) {
        render_dir = out_dir + "/.aponeo_tmp_frames";
        QDir(render_dir).mkpath(".");
    }

    auto start_time = std::chrono::steady_clock::now();

    Engine::CPUSIMDEngine engine;

    for (int frame = 0; frame < total_frames; ++frame) {
        if (m_cancel_requested) {
            emit render_finished_signal(false, tr("Export cancelled by user."));
            return;
        }

        Core::FlameGenome genome = m_anim_widget->genome_at_frame(frame);
        genome.width = width;
        genome.height = height;
        genome.supersample = supersample;
        genome.quality = quality;
        genome.compile();

        engine.set_genome(genome);
        engine.start();
        engine.wait_until_done();

        std::vector<uint8_t> rgba;
        int rw = 0, rh = 0;
        engine.get_image_rgba8888(rgba, rw, rh);

        if (rw == width && rh == height && rgba.size() >= static_cast<size_t>(rw * rh * 4)) {
            QImage img(rgba.data(), width, height, width * 4, QImage::Format_RGBA8888);
            char filename_buf[128];
            std::snprintf(filename_buf, sizeof(filename_buf), "frame_%04d.png", frame);
            QString frame_path = render_dir + "/" + QString::fromUtf8(filename_buf);
            img.save(frame_path, "PNG");
        }

        auto now = std::chrono::steady_clock::now();
        double elapsed_sec = std::chrono::duration<double>(now - start_time).count();
        emit render_progress_signal(frame + 1, total_frames, elapsed_sec);
    }

    // Video encoding with FFmpeg if selected
    if (is_video) {
        QString ffmpeg_bin = QStandardPaths::findExecutable("ffmpeg");
        if (ffmpeg_bin.isEmpty()) {
            // Check common macOS locations
            if (QFile::exists("/opt/homebrew/bin/ffmpeg")) ffmpeg_bin = "/opt/homebrew/bin/ffmpeg";
            else if (QFile::exists("/usr/local/bin/ffmpeg")) ffmpeg_bin = "/usr/local/bin/ffmpeg";
        }

        if (ffmpeg_bin.isEmpty()) {
            emit render_finished_signal(true, tr("Frames rendered, but ffmpeg was not found on PATH. PNG frames preserved at %1").arg(render_dir));
            return;
        }

        QString video_out = out_dir + "/" + base_name + ".mp4";
        QStringList args = {
            "-y",
            "-framerate", QString::number(fps),
            "-i", render_dir + "/frame_%04d.png",
            "-c:v", "libx264",
            "-pix_fmt", "yuv420p",
            video_out
        };

        QProcess proc;
        proc.start(ffmpeg_bin, args);
        proc.waitForFinished(-1);

        if (proc.exitStatus() == QProcess::NormalExit && proc.exitCode() == 0) {
            // Clean temporary frame directory
            QDir(render_dir).removeRecursively();
            emit render_finished_signal(true, tr("Animation exported successfully to:\n%1").arg(video_out));
        } else {
            QString err = QString::fromUtf8(proc.readAllStandardError());
            emit render_finished_signal(false, tr("FFmpeg encoding error:\n%1").arg(err));
        }
    } else {
        emit render_finished_signal(true, tr("Rendered %1 frames successfully to:\n%2").arg(total_frames).arg(render_dir));
    }
}

void ExportDialog::on_render_progress(int current_frame, int total_frames, double elapsed_sec) {
    int pct = static_cast<int>(std::round((static_cast<double>(current_frame) / total_frames) * 100.0));
    m_progress_bar->setValue(pct);

    double sec_per_frame = elapsed_sec / std::max(1, current_frame);
    double remaining_sec = sec_per_frame * (total_frames - current_frame);

    int eta_m = static_cast<int>(remaining_sec) / 60;
    int eta_s = static_cast<int>(remaining_sec) % 60;

    m_lbl_status->setText(tr("Rendering frame %1 / %2 (%3%)...").arg(current_frame).arg(total_frames).arg(pct));
    m_lbl_eta->setText(QString("ETA: %1m %2s").arg(eta_m).arg(eta_s, 2, 10, QChar('0')));
}

void ExportDialog::on_render_finished(bool success, const QString& message) {
    m_btn_start->setEnabled(true);
    m_btn_browse->setEnabled(true);
    m_combo_preset->setEnabled(true);
    m_spin_width->setEnabled(true);
    m_spin_height->setEnabled(true);
    m_spin_supersample->setEnabled(true);
    m_spin_quality->setEnabled(true);
    m_combo_format->setEnabled(true);

    if (success) {
        m_lbl_status->setText(tr("Export completed!"));
        QMessageBox::information(this, tr("Export Succeeded"), message);
        accept();
    } else {
        m_lbl_status->setText(tr("Export halted."));
        QMessageBox::warning(this, tr("Export Result"), message);
    }
}

} // namespace ApoNeo::UI
