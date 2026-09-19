#include "MainWindow.hpp"
#include "core/FlameXml.hpp"
#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <iostream>

namespace ApoNeo::UI {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("ApoNeo - Modern Fractal Flame Studio"));
    resize(1200, 800);

    m_engine = std::make_unique<Engine::CPUSIMDEngine>();
    m_current_genome = Core::FlameGenome::preset_swirl_flame();

    setup_ui();
    setup_menus();
    setup_toolbar();
    setup_dock_panel();
    setup_statusbar();
    apply_modern_theme();

    load_genome_to_ui(m_current_genome);

    connect(&m_poll_timer, &QTimer::timeout, this, &MainWindow::on_poll_timer_tick);
    m_poll_timer.start(80); // 80ms poll rate for 12.5 FPS progressive updates

    // Automatically kick off initial render
    on_action_render();
}

MainWindow::~MainWindow() {
    if (m_engine) {
        m_engine->stop();
    }
}

void MainWindow::setup_ui() {
    m_canvas = new FlameCanvasWidget(this);
    setCentralWidget(m_canvas);

    connect(m_canvas, &FlameCanvasWidget::coordinate_hovered, this, &MainWindow::on_canvas_coordinate_hovered);
}

void MainWindow::setup_menus() {
    auto* menu_file = menuBar()->addMenu(tr("&File"));
    menu_file->addAction(tr("&Open Flame..."), QKeySequence::Open, this, &MainWindow::on_action_open_flame);
    menu_file->addAction(tr("&Save Flame..."), QKeySequence::Save, this, &MainWindow::on_action_save_flame);
    menu_file->addSeparator();
    menu_file->addAction(tr("&Export Image..."), QKeySequence(Qt::CTRL | Qt::Key_E), this, &MainWindow::on_action_export_image);
    menu_file->addSeparator();
    menu_file->addAction(tr("E&xit"), QKeySequence::Quit, qApp, &QApplication::quit);

    auto* menu_render = menuBar()->addMenu(tr("&Render"));
    m_act_render = menu_render->addAction(tr("&Start Render"), QKeySequence(Qt::Key_F5), this, &MainWindow::on_action_render);
    m_act_pause = menu_render->addAction(tr("&Pause / Resume"), QKeySequence(Qt::Key_F6), this, &MainWindow::on_action_pause);
    m_act_stop = menu_render->addAction(tr("S&top Render"), QKeySequence(Qt::Key_F7), this, &MainWindow::on_action_stop);
    menu_render->addSeparator();
    menu_render->addAction(tr("Reset &View"), QKeySequence(Qt::Key_Home), this, &MainWindow::on_action_reset_view);

    auto* menu_help = menuBar()->addMenu(tr("&Help"));
    menu_help->addAction(tr("&About ApoNeo..."), this, &MainWindow::on_action_about);
}

void MainWindow::setup_toolbar() {
    auto* toolbar = addToolBar(tr("Main Toolbar"));
    toolbar->setMovable(false);

    toolbar->addAction(m_act_render);
    toolbar->addAction(m_act_pause);
    toolbar->addAction(m_act_stop);
    toolbar->addSeparator();

    toolbar->addAction(tr("Reset View"), this, &MainWindow::on_action_reset_view);
    toolbar->addSeparator();

    toolbar->addWidget(new QLabel(tr(" Preset: "), this));
    m_combo_preset = new QComboBox(this);
    m_combo_preset->addItem(tr("Swirl Vortex"));
    m_combo_preset->addItem(tr("Sierpinski Gasket"));
    m_combo_preset->addItem(tr("Barnsley Fern"));
    m_combo_preset->addItem(tr("Julia Vortex"));
    connect(m_combo_preset, &QComboBox::currentIndexChanged, this, &MainWindow::on_preset_changed);
    toolbar->addWidget(m_combo_preset);

    toolbar->addSeparator();
    toolbar->addWidget(new QLabel(tr(" Palette: "), this));
    m_combo_palette = new QComboBox(this);
    m_combo_palette->addItem(tr("Fire"));
    m_combo_palette->addItem(tr("Electric Blue"));
    m_combo_palette->addItem(tr("Rainbow"));
    m_combo_palette->addItem(tr("Aurora"));
    m_combo_palette->addItem(tr("Sunset"));
    m_combo_palette->addItem(tr("Monochrome"));
    connect(m_combo_palette, &QComboBox::currentIndexChanged, this, &MainWindow::on_palette_changed);
    toolbar->addWidget(m_combo_palette);
}

void MainWindow::setup_dock_panel() {
    m_dock = new QDockWidget(tr("Flame Inspector"), this);
    m_dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    auto* container = new QWidget(m_dock);
    auto* layout = new QVBoxLayout(container);
    layout->setSpacing(12);

    // Resolution & Quality Group
    auto* grp_render = new QGroupBox(tr("Render Settings"), container);
    auto* fl_render = new QFormLayout(grp_render);

    m_spin_width = new QSpinBox(grp_render);
    m_spin_width->setRange(128, 7680);
    m_spin_width->setSingleStep(64);
    fl_render->addRow(tr("Width:"), m_spin_width);

    m_spin_height = new QSpinBox(grp_render);
    m_spin_height->setRange(128, 4320);
    m_spin_height->setSingleStep(64);
    fl_render->addRow(tr("Height:"), m_spin_height);

    m_spin_supersample = new QSpinBox(grp_render);
    m_spin_supersample->setRange(1, 4);
    fl_render->addRow(tr("Supersample:"), m_spin_supersample);

    m_spin_quality = new QDoubleSpinBox(grp_render);
    m_spin_quality->setRange(1.0, 50000.0);
    m_spin_quality->setSingleStep(50.0);
    fl_render->addRow(tr("Quality:"), m_spin_quality);

    layout->addWidget(grp_render);

    // Geometry & Camera Group
    auto* grp_geom = new QGroupBox(tr("Geometry & Camera"), container);
    auto* fl_geom = new QFormLayout(grp_geom);

    m_spin_scale = new QDoubleSpinBox(grp_geom);
    m_spin_scale->setRange(1.0, 100000.0);
    m_spin_scale->setSingleStep(10.0);
    fl_geom->addRow(tr("Scale:"), m_spin_scale);

    m_spin_center_x = new QDoubleSpinBox(grp_geom);
    m_spin_center_x->setRange(-1000.0, 1000.0);
    m_spin_center_x->setSingleStep(0.1);
    fl_geom->addRow(tr("Center X:"), m_spin_center_x);

    m_spin_center_y = new QDoubleSpinBox(grp_geom);
    m_spin_center_y->setRange(-1000.0, 1000.0);
    m_spin_center_y->setSingleStep(0.1);
    fl_geom->addRow(tr("Center Y:"), m_spin_center_y);

    m_spin_rotate = new QDoubleSpinBox(grp_geom);
    m_spin_rotate->setRange(-360.0, 360.0);
    m_spin_rotate->setSingleStep(5.0);
    fl_geom->addRow(tr("Rotate (deg):"), m_spin_rotate);

    m_spin_zoom = new QDoubleSpinBox(grp_geom);
    m_spin_zoom->setRange(-10.0, 10.0);
    m_spin_zoom->setSingleStep(0.1);
    fl_geom->addRow(tr("Zoom:"), m_spin_zoom);

    layout->addWidget(grp_geom);

    // Tone Mapping Group
    auto* grp_tone = new QGroupBox(tr("Tone Mapping"), container);
    auto* fl_tone = new QFormLayout(grp_tone);

    m_spin_gamma = new QDoubleSpinBox(grp_tone);
    m_spin_gamma->setRange(0.1, 20.0);
    m_spin_gamma->setSingleStep(0.2);
    fl_tone->addRow(tr("Gamma:"), m_spin_gamma);

    m_spin_brightness = new QDoubleSpinBox(grp_tone);
    m_spin_brightness->setRange(0.01, 50.0);
    m_spin_brightness->setSingleStep(0.1);
    fl_tone->addRow(tr("Brightness:"), m_spin_brightness);

    m_spin_vibrancy = new QDoubleSpinBox(grp_tone);
    m_spin_vibrancy->setRange(0.0, 1.0);
    m_spin_vibrancy->setSingleStep(0.05);
    fl_tone->addRow(tr("Vibrancy:"), m_spin_vibrancy);

    layout->addWidget(grp_tone);
    layout->addStretch();

    // Connect spinbox modification
    auto connect_spin = [this](auto* spin) {
        connect(spin, &QDoubleSpinBox::valueChanged, this, &MainWindow::on_parameters_modified);
    };
    auto connect_int_spin = [this](auto* spin) {
        connect(spin, &QSpinBox::valueChanged, this, &MainWindow::on_parameters_modified);
    };

    connect_int_spin(m_spin_width);
    connect_int_spin(m_spin_height);
    connect_int_spin(m_spin_supersample);
    connect_spin(m_spin_quality);
    connect_spin(m_spin_scale);
    connect_spin(m_spin_center_x);
    connect_spin(m_spin_center_y);
    connect_spin(m_spin_rotate);
    connect_spin(m_spin_zoom);
    connect_spin(m_spin_gamma);
    connect_spin(m_spin_brightness);
    connect_spin(m_spin_vibrancy);

    m_dock->setWidget(container);
    addDockWidget(Qt::RightDockWidgetArea, m_dock);
}

void MainWindow::setup_statusbar() {
    m_status_label = new QLabel(tr("Ready"), this);
    m_progress_bar = new QProgressBar(this);
    m_progress_bar->setRange(0, 100);
    m_progress_bar->setValue(0);
    m_progress_bar->setTextVisible(true);
    m_progress_bar->setFixedWidth(160);

    m_coord_label = new QLabel(tr("X: 0.00, Y: 0.00"), this);
    m_coord_label->setMinimumWidth(140);

    statusBar()->addWidget(m_status_label, 1);
    statusBar()->addPermanentWidget(m_coord_label);
    statusBar()->addPermanentWidget(m_progress_bar);
}

void MainWindow::apply_modern_theme() {
    // Elegant Dark Theme
    setStyleSheet(QStringLiteral(R"(
        QMainWindow, QDockWidget {
            background-color: #14171c;
            color: #d8dee9;
        }
        QMenuBar {
            background-color: #1b1e24;
            color: #e5e9f0;
            border-bottom: 1px solid #2e3440;
        }
        QMenuBar::item:selected {
            background-color: #3b4252;
            border-radius: 4px;
        }
        QMenu {
            background-color: #242933;
            color: #eceff4;
            border: 1px solid #3b4252;
        }
        QMenu::item:selected {
            background-color: #434c5e;
        }
        QToolBar {
            background-color: #1a1d24;
            border-bottom: 1px solid #2e3440;
            padding: 4px;
            spacing: 6px;
        }
        QGroupBox {
            font-weight: bold;
            border: 1px solid #2e3440;
            border-radius: 6px;
            margin-top: 10px;
            padding-top: 12px;
            color: #88c0d0;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 0 5px;
        }
        QSpinBox, QDoubleSpinBox, QComboBox {
            background-color: #242933;
            color: #eceff4;
            border: 1px solid #3b4252;
            border-radius: 4px;
            padding: 4px;
        }
        QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus {
            border: 1px solid #88c0d0;
        }
        QProgressBar {
            background-color: #242933;
            border: 1px solid #3b4252;
            border-radius: 4px;
            text-align: center;
            color: #eceff4;
        }
        QProgressBar::chunk {
            background-color: #5e81ac;
            border-radius: 3px;
        }
        QStatusBar {
            background-color: #1b1e24;
            color: #d8dee9;
            border-top: 1px solid #2e3440;
        }
    )"));
}

void MainWindow::load_genome_to_ui(const Core::FlameGenome& g) {
    m_updating_ui = true;
    m_spin_width->setValue(g.width);
    m_spin_height->setValue(g.height);
    m_spin_supersample->setValue(g.supersample);
    m_spin_quality->setValue(g.quality);

    m_spin_scale->setValue(g.scale);
    m_spin_center_x->setValue(g.center_x);
    m_spin_center_y->setValue(g.center_y);
    m_spin_rotate->setValue(g.rotate * 180.0 / 3.141592653589793);
    m_spin_zoom->setValue(g.zoom);

    m_spin_gamma->setValue(g.tone_map.gamma);
    m_spin_brightness->setValue(g.tone_map.brightness);
    m_spin_vibrancy->setValue(g.tone_map.vibrancy);
    m_updating_ui = false;
}

void MainWindow::read_genome_from_ui(Core::FlameGenome& g) {
    g.width = m_spin_width->value();
    g.height = m_spin_height->value();
    g.supersample = m_spin_supersample->value();
    g.quality = m_spin_quality->value();

    g.scale = m_spin_scale->value();
    g.center_x = m_spin_center_x->value();
    g.center_y = m_spin_center_y->value();
    g.rotate = m_spin_rotate->value() * 3.141592653589793 / 180.0;
    g.zoom = m_spin_zoom->value();

    g.tone_map.gamma = m_spin_gamma->value();
    g.tone_map.brightness = m_spin_brightness->value();
    g.tone_map.vibrancy = m_spin_vibrancy->value();
}

void MainWindow::on_action_render() {
    read_genome_from_ui(m_current_genome);
    m_engine->set_genome(m_current_genome);
    m_engine->start();
    m_status_label->setText(tr("Rendering started..."));
}

void MainWindow::on_action_pause() {
    if (m_engine->is_rendering()) {
        m_engine->pause();
        m_status_label->setText(tr("Rendering paused"));
    } else {
        m_engine->resume();
        m_status_label->setText(tr("Rendering resumed"));
    }
}

void MainWindow::on_action_stop() {
    m_engine->stop();
    m_status_label->setText(tr("Rendering stopped"));
}

void MainWindow::on_action_reset_view() {
    m_canvas->reset_view();
}

void MainWindow::on_action_open_flame() {
    QString path = QFileDialog::getOpenFileName(this, tr("Open Fractal Flame"), QString(), tr("Flame Files (*.flame *.xml);;All Files (*)"));
    if (path.isEmpty()) return;

    std::string err;
    auto flames = Core::FlameXml::load_from_file(path.toStdString(), &err);
    if (flames.empty()) {
        QMessageBox::warning(this, tr("Error Opening Flame"), QString::fromStdString(err.empty() ? "No flames found in file." : err));
        return;
    }

    m_current_genome = flames.front();
    load_genome_to_ui(m_current_genome);
    on_action_render();
}

void MainWindow::on_action_save_flame() {
    QString path = QFileDialog::getSaveFileName(this, tr("Save Fractal Flame"), QStringLiteral("fractal.flame"), tr("Flame Files (*.flame *.xml)"));
    if (path.isEmpty()) return;

    read_genome_from_ui(m_current_genome);
    std::string err;
    if (!Core::FlameXml::save_to_file(path.toStdString(), m_current_genome, &err)) {
        QMessageBox::critical(this, tr("Error Saving Flame"), QString::fromStdString(err));
    } else {
        statusBar()->showMessage(tr("Saved %1 successfully").arg(path), 3000);
    }
}

void MainWindow::on_action_export_image() {
    QString path = QFileDialog::getSaveFileName(this, tr("Export Rendered Image"), QStringLiteral("render.png"), tr("PNG Images (*.png);;JPEG Images (*.jpg *.jpeg)"));
    if (path.isEmpty()) return;

    const QImage& img = m_canvas->rendered_image();
    if (img.isNull()) {
        QMessageBox::warning(this, tr("Export Image"), tr("No rendered image available to export."));
        return;
    }

    if (img.save(path)) {
        statusBar()->showMessage(tr("Exported %1").arg(path), 3000);
    } else {
        QMessageBox::critical(this, tr("Export Image"), tr("Failed to save image to %1").arg(path));
    }
}

void MainWindow::on_action_about() {
    QMessageBox::about(this, tr("About ApoNeo"),
        tr("<h3>ApoNeo Studio 1.0</h3>"
           "<p>A modern, cross-platform Fractal Flame editor and SIMD renderer in C++20 and Qt 6.</p>"
           "<p>Featuring Apple Silicon NEON acceleration, multithreaded lockless chaos game iteration, and flam3 compatibility.</p>"));
}

void MainWindow::on_preset_changed(int index) {
    switch (index) {
        case 0: m_current_genome = Core::FlameGenome::preset_swirl_flame(); break;
        case 1: m_current_genome = Core::FlameGenome::preset_sierpinski(); break;
        case 2: m_current_genome = Core::FlameGenome::preset_barnsley_fern(); break;
        case 3: m_current_genome = Core::FlameGenome::preset_julia_vortex(); break;
        default: break;
    }
    load_genome_to_ui(m_current_genome);
    on_action_render();
}

void MainWindow::on_palette_changed(int index) {
    switch (index) {
        case 0: m_current_genome.palette = Core::Palette::preset_fire(); break;
        case 1: m_current_genome.palette = Core::Palette::preset_electric_blue(); break;
        case 2: m_current_genome.palette = Core::Palette::preset_rainbow(); break;
        case 3: m_current_genome.palette = Core::Palette::preset_aurora(); break;
        case 4: m_current_genome.palette = Core::Palette::preset_sunset(); break;
        case 5: m_current_genome.palette = Core::Palette::preset_monochrome(); break;
        default: break;
    }
    on_action_render();
}

void MainWindow::on_parameters_modified() {
    if (m_updating_ui) return;
    read_genome_from_ui(m_current_genome);
    on_action_render();
}

void MainWindow::on_poll_timer_tick() {
    auto progress = m_engine->get_progress();

    // Update status & progress bar
    int pct = static_cast<int>(std::round(progress.fraction() * 100.0));
    m_progress_bar->setValue(pct);

    double m_iters = progress.iterations_per_second / 1e6;
    if (m_engine->is_rendering()) {
        m_status_label->setText(tr("Rendering: %1% | Speed: %2 Miter/s | Time: %3s")
            .arg(pct)
            .arg(m_iters, 0, 'f', 2)
            .arg(progress.elapsed_seconds, 0, 'f', 1));
    } else if (progress.is_finished) {
        m_status_label->setText(tr("Render complete in %1s (%2 Miter/s)")
            .arg(progress.elapsed_seconds, 0, 'f', 2)
            .arg(m_iters, 0, 'f', 2));
    }

    // Retrieve latest frame buffer and display on canvas
    std::vector<uint8_t> buffer;
    int w = 0, h = 0;
    m_engine->get_image_rgba8888(buffer, w, h);

    if (w > 0 && h > 0 && buffer.size() >= static_cast<size_t>(w * h * 4)) {
        // Create QImage directly referencing buffer data (and detach to own memory)
        QImage frame(buffer.data(), w, h, w * 4, QImage::Format_RGBA8888);
        m_canvas->set_rendered_image(frame.copy());
    }
}

void MainWindow::on_canvas_coordinate_hovered(double x, double y) {
    m_coord_label->setText(tr("X: %1, Y: %2").arg(x, 0, 'f', 2).arg(y, 0, 'f', 2));
}

} // namespace ApoNeo::UI
