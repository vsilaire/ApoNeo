#include "MainWindow.hpp"
#include "ExportDialog.hpp"
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
    resize(1500, 950);

    m_engine = std::make_unique<Engine::CPUSIMDEngine>();

    setup_ui();
    setup_menus();
    setup_toolbar();
    setup_flame_batch_dock();
    setup_dock_panel();
    setup_transform_editor_dock();
    setup_variation_inspector_dock();
    setup_animation_dock();
    setup_statusbar();
    apply_modern_theme();

    // Startup with 10 random flames generated via Apophysis heuristics
    auto initial_batch = m_random_generator.generate_batch(10, "Random Flame");
    if (!initial_batch.empty()) {
        m_current_genome = initial_batch.front();
        if (m_flame_list) {
            m_flame_list->set_flames(initial_batch);
        }
    } else {
        m_current_genome = Core::FlameGenome::preset_swirl_flame();
    }

    load_genome_to_ui(m_current_genome);

    connect(&m_poll_timer, &QTimer::timeout, this, &MainWindow::on_poll_timer_tick);
    m_poll_timer.start(80); // 80ms poll rate for smooth progressive updates

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
    m_act_export_anim = menu_file->addAction(tr("Export &Animation..."), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E), this, &MainWindow::on_action_export_animation);
    menu_file->addSeparator();
    menu_file->addAction(tr("E&xit"), QKeySequence::Quit, qApp, &QApplication::quit);

    auto* menu_edit = menuBar()->addMenu(tr("&Edit"));
    m_act_undo = m_undo_stack.createUndoAction(this, tr("&Undo"));
    m_act_undo->setShortcut(QKeySequence::Undo);
    menu_edit->addAction(m_act_undo);

    m_act_redo = m_undo_stack.createRedoAction(this, tr("&Redo"));
    m_act_redo->setShortcut(QKeySequence::Redo);
    menu_edit->addAction(m_act_redo);

    auto* menu_flame = menuBar()->addMenu(tr("&Flame"));
    m_act_generate_batch = menu_flame->addAction(tr("🎲 &Generate New Batch"), QKeySequence(Qt::CTRL | Qt::Key_R), this, &MainWindow::on_action_generate_batch);

    auto* menu_render = menuBar()->addMenu(tr("&Render"));
    m_act_render = menu_render->addAction(tr("&Start Render"), QKeySequence(Qt::Key_F5), this, &MainWindow::on_action_render);
    m_act_pause = menu_render->addAction(tr("&Pause / Resume"), QKeySequence(Qt::Key_F6), this, &MainWindow::on_action_pause);
    m_act_stop = menu_render->addAction(tr("S&top Render"), QKeySequence(Qt::Key_F7), this, &MainWindow::on_action_stop);
    menu_render->addSeparator();
    menu_render->addAction(tr("Reset &View"), QKeySequence(Qt::Key_Home), this, &MainWindow::on_action_reset_view);

    auto* menu_view = menuBar()->addMenu(tr("&View"));
    menu_view->addAction(tr("Reset Canvas View"), this, &MainWindow::on_action_reset_view);

    auto* menu_help = menuBar()->addMenu(tr("&Help"));
    menu_help->addAction(tr("&About ApoNeo..."), this, &MainWindow::on_action_about);
}

void MainWindow::setup_toolbar() {
    auto* toolbar = addToolBar(tr("Main Toolbar"));
    toolbar->setMovable(false);

    toolbar->addAction(m_act_undo);
    toolbar->addAction(m_act_redo);
    toolbar->addSeparator();

    toolbar->addAction(m_act_render);
    toolbar->addAction(m_act_pause);
    toolbar->addAction(m_act_stop);
    toolbar->addSeparator();

    toolbar->addAction(m_act_generate_batch);
    toolbar->addAction(m_act_export_anim);
    toolbar->addSeparator();

    toolbar->addAction(tr("Reset View"), this, &MainWindow::on_action_reset_view);
    toolbar->addSeparator();

    toolbar->addWidget(new QLabel(tr(" Preset: "), this));
    m_combo_preset = new QComboBox(this);
    m_combo_preset->addItem(tr("Custom / Random"));
    m_combo_preset->addItem(tr("Swirl Vortex"));
    m_combo_preset->addItem(tr("Sierpinski Gasket"));
    m_combo_preset->addItem(tr("Barnsley Fern"));
    m_combo_preset->addItem(tr("Julia Vortex"));
    connect(m_combo_preset, &QComboBox::currentIndexChanged, this, &MainWindow::on_preset_changed);
    toolbar->addWidget(m_combo_preset);
}

void MainWindow::setup_dock_panel() {
    m_dock = new QDockWidget(tr("Flame Inspector"), this);
    m_dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    m_flame_inspector = new FlameInspectorWidget(m_dock);
    m_dock->setWidget(m_flame_inspector);
    addDockWidget(Qt::RightDockWidgetArea, m_dock);

    connect(m_flame_inspector, &FlameInspectorWidget::camera_modified, this, &MainWindow::on_camera_modified);
    connect(m_flame_inspector, &FlameInspectorWidget::tonemap_modified, this, &MainWindow::on_tonemap_modified);
    connect(m_flame_inspector, &FlameInspectorWidget::palette_preset_changed, this, &MainWindow::on_palette_preset_changed);
    connect(m_flame_inspector, &FlameInspectorWidget::xform_props_modified, this, &MainWindow::on_xform_props_modified);
    connect(m_flame_inspector, &FlameInspectorWidget::global_settings_modified, this, &MainWindow::on_global_settings_modified);
}

void MainWindow::setup_flame_batch_dock() {
    m_batch_dock = new QDockWidget(tr("Random Flame Batch (Startup)"), this);
    m_batch_dock->setAllowedAreas(Qt::AllDockWidgetAreas);

    m_flame_list = new FlameListWidget(m_batch_dock);
    m_batch_dock->setWidget(m_flame_list);
    addDockWidget(Qt::LeftDockWidgetArea, m_batch_dock);

    connect(m_flame_list, &FlameListWidget::flame_selected, this, &MainWindow::on_flame_batch_selected);
    connect(m_flame_list, &FlameListWidget::request_new_batch, this, &MainWindow::on_action_generate_batch);
}

void MainWindow::setup_transform_editor_dock() {
    m_transform_dock = new QDockWidget(tr("Transform Editor (Triangles)"), this);
    m_transform_dock->setAllowedAreas(Qt::AllDockWidgetAreas);

    m_transform_editor = new TransformEditorWidget(m_transform_dock);
    m_transform_dock->setWidget(m_transform_editor);
    addDockWidget(Qt::LeftDockWidgetArea, m_transform_dock);
    tabifyDockWidget(m_batch_dock, m_transform_dock);

    connect(m_transform_editor, &TransformEditorWidget::xform_modified, this, &MainWindow::on_transform_editor_modified);
    connect(m_transform_editor, &TransformEditorWidget::post_xform_modified, this, &MainWindow::on_post_transform_editor_modified);
    connect(m_transform_editor, &TransformEditorWidget::xform_added, this, &MainWindow::on_transform_editor_structure_changed);
    connect(m_transform_editor, &TransformEditorWidget::xform_deleted, this, &MainWindow::on_transform_editor_structure_changed);
    connect(m_transform_editor, &TransformEditorWidget::xform_selected, this, &MainWindow::on_xform_selection_changed);
}

void MainWindow::setup_variation_inspector_dock() {
    m_variation_dock = new QDockWidget(tr("Variation & Variable Inspector"), this);
    m_variation_dock->setAllowedAreas(Qt::AllDockWidgetAreas);

    m_variation_inspector = new VariationInspectorWidget(m_variation_dock);
    m_variation_dock->setWidget(m_variation_inspector);
    addDockWidget(Qt::RightDockWidgetArea, m_variation_dock);
    tabifyDockWidget(m_dock, m_variation_dock);

    connect(m_variation_inspector, &VariationInspectorWidget::variation_modified, this, &MainWindow::on_variation_modified);
    connect(m_variation_inspector, &VariationInspectorWidget::param_modified, this, &MainWindow::on_param_modified);
    connect(m_variation_inspector, &VariationInspectorWidget::variations_cleared, this, &MainWindow::on_variations_cleared);
}

void MainWindow::setup_animation_dock() {
    m_animation_dock = new QDockWidget(tr("Animation Timeline & flam3-animate"), this);
    m_animation_dock->setAllowedAreas(Qt::AllDockWidgetAreas);

    m_animation_widget = new AnimationWidget(m_animation_dock);
    m_animation_dock->setWidget(m_animation_widget);
    addDockWidget(Qt::BottomDockWidgetArea, m_animation_dock);

    connect(m_animation_widget, &AnimationWidget::frame_rendered, this, &MainWindow::on_animation_frame_rendered);
    connect(m_animation_widget, &AnimationWidget::request_add_current_keyframe, this, &MainWindow::on_animation_add_keyframe);
    connect(m_animation_widget, &AnimationWidget::request_export, this, &MainWindow::on_action_export_animation);
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
        QSpinBox, QDoubleSpinBox, QComboBox, QLineEdit {
            background-color: #242933;
            color: #eceff4;
            border: 1px solid #3b4252;
            border-radius: 4px;
            padding: 4px;
        }
        QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus, QLineEdit:focus {
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

    if (m_flame_inspector) {
        m_flame_inspector->set_genome(g);
    }
    if (m_transform_editor) {
        m_transform_editor->set_genome(g);
    }
    if (m_variation_inspector) {
        m_variation_inspector->set_genome(g);
    }

    m_updating_ui = false;
}

void MainWindow::sync_active_keyframe_if_present() {
    if (!m_animation_widget || m_animation_widget->keyframes().empty()) return;
    int cur_frame = m_animation_widget->current_frame();
    for (const auto& kf : m_animation_widget->keyframes()) {
        if (kf.frame == cur_frame) {
            m_animation_widget->add_keyframe(cur_frame, m_current_genome);
            break;
        }
    }
}

void MainWindow::trigger_interactive_preview() {
    sync_active_keyframe_if_present();
    Core::FlameGenome preview_genome = m_current_genome;
    preview_genome.quality = std::min(15.0, m_current_genome.quality);
    preview_genome.supersample = 1;
    m_engine->set_genome(preview_genome);
    m_engine->start();
}

void MainWindow::on_action_render() {
    if (m_flame_inspector) {
        m_flame_inspector->read_genome(m_current_genome);
    }
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
    m_undo_stack.clear();
    on_action_render();
}

void MainWindow::on_action_save_flame() {
    QString path = QFileDialog::getSaveFileName(this, tr("Save Fractal Flame"), QStringLiteral("fractal.flame"), tr("Flame Files (*.flame *.xml)"));
    if (path.isEmpty()) return;

    if (m_flame_inspector) {
        m_flame_inspector->read_genome(m_current_genome);
    }
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

void MainWindow::on_action_export_animation() {
    if (!m_animation_widget || m_animation_widget->keyframes().empty()) {
        auto res = QMessageBox::question(
            this, tr("No Keyframes in Timeline"),
            tr("No keyframes exist on the timeline.\n\nWould you like to automatically create an interpolation morph between the current flame and another random flame?"),
            QMessageBox::Yes | QMessageBox::No
        );
        if (res == QMessageBox::Yes) {
            if (m_flame_inspector) {
                m_flame_inspector->read_genome(m_current_genome);
            }
            m_animation_widget->add_keyframe(0, m_current_genome);
            auto next_genome = m_random_generator.generate("Target Flame");
            m_animation_widget->add_keyframe(m_animation_widget->total_frames() - 1, next_genome);
        } else {
            return;
        }
    }

    ExportDialog dlg(m_animation_widget, this);
    dlg.exec();
}

void MainWindow::on_action_generate_batch() {
    auto batch = m_random_generator.generate_batch(10, "Random Flame");
    if (!batch.empty()) {
        if (m_flame_list) {
            m_flame_list->set_flames(batch);
        }
        m_current_genome = batch.front();
        load_genome_to_ui(m_current_genome);
        m_undo_stack.clear();
        on_action_render();
    }
}

void MainWindow::on_flame_batch_selected(const Core::FlameGenome& genome) {
    m_current_genome = genome;
    load_genome_to_ui(m_current_genome);
    m_undo_stack.clear();
    on_action_render();
}

void MainWindow::on_animation_frame_rendered(const Core::FlameGenome& genome, int frame) {
    (void)frame;
    m_current_genome = genome;
    load_genome_to_ui(m_current_genome);
    trigger_interactive_preview();
}

void MainWindow::on_animation_add_keyframe() {
    if (m_flame_inspector) {
        m_flame_inspector->read_genome(m_current_genome);
    }
    if (m_animation_widget) {
        m_animation_widget->add_keyframe(m_animation_widget->current_frame(), m_current_genome);
        statusBar()->showMessage(tr("Added keyframe at frame %1 (%2)").arg(m_animation_widget->current_frame()).arg(QString::fromStdString(m_current_genome.name)), 3000);
    }
}

void MainWindow::on_action_about() {
    QMessageBox::about(this, tr("About ApoNeo"),
        tr("<h3>ApoNeo Studio 1.0</h3>"
           "<p>A modern, cross-platform Fractal Flame editor and SIMD renderer in C++20 and Qt 6.</p>"
           "<p>Featuring complete Apophysis 7x variations and parameters, 2D Affine & Post-Transform Triangles, Apple Silicon NEON acceleration, QUndoStack undo/redo, and flam3-animate keyframe interpolation.</p>"));
}

void MainWindow::on_preset_changed(int index) {
    switch (index) {
        case 0: return; // Custom / Random (no action)
        case 1: m_current_genome = Core::FlameGenome::preset_swirl_flame(); break;
        case 2: m_current_genome = Core::FlameGenome::preset_sierpinski(); break;
        case 3: m_current_genome = Core::FlameGenome::preset_barnsley_fern(); break;
        case 4: m_current_genome = Core::FlameGenome::preset_julia_vortex(); break;
        default: break;
    }
    load_genome_to_ui(m_current_genome);
    m_undo_stack.clear();
    on_action_render();
}

void MainWindow::on_camera_modified(double scale, double cx, double cy, double rot, double zoom) {
    if (m_updating_ui) return;

    Core::Commands::ChangeCameraCommand::CameraState old_state{
        m_current_genome.scale, m_current_genome.center_x, m_current_genome.center_y,
        m_current_genome.rotate, m_current_genome.zoom
    };
    Core::Commands::ChangeCameraCommand::CameraState new_state{
        scale, cx, cy, rot, zoom
    };

    auto* cmd = new Core::Commands::ChangeCameraCommand(&m_current_genome, old_state, new_state, [this]() {
        load_genome_to_ui(m_current_genome);
        trigger_interactive_preview();
    });
    m_undo_stack.push(cmd);
}

void MainWindow::on_tonemap_modified(double gamma, double brightness, double vibrancy) {
    if (m_updating_ui) return;

    Core::ToneMapParams old_p = m_current_genome.tone_map;
    Core::ToneMapParams new_p = old_p;
    new_p.gamma = gamma;
    new_p.brightness = brightness;
    new_p.vibrancy = vibrancy;

    auto* cmd = new Core::Commands::ChangeToneMapCommand(&m_current_genome, old_p, new_p, [this]() {
        load_genome_to_ui(m_current_genome);
        trigger_interactive_preview();
    });
    m_undo_stack.push(cmd);
}

void MainWindow::on_palette_preset_changed(int index) {
    if (m_updating_ui) return;

    Core::Palette old_pal = m_current_genome.palette;
    Core::Palette new_pal;
    switch (index) {
        case 0: new_pal = Core::Palette::preset_fire(); break;
        case 1: new_pal = Core::Palette::preset_electric_blue(); break;
        case 2: new_pal = Core::Palette::preset_rainbow(); break;
        case 3: new_pal = Core::Palette::preset_aurora(); break;
        case 4: new_pal = Core::Palette::preset_sunset(); break;
        case 5: new_pal = Core::Palette::preset_monochrome(); break;
        default: break;
    }

    auto* cmd = new Core::Commands::ChangePaletteCommand(&m_current_genome, old_pal, new_pal, [this]() {
        load_genome_to_ui(m_current_genome);
        trigger_interactive_preview();
    });
    m_undo_stack.push(cmd);
}

void MainWindow::on_xform_props_modified(size_t xform_idx, double weight, double color_idx, double color_speed, double opacity, bool visible) {
    if (m_updating_ui || xform_idx >= m_current_genome.xforms.size()) return;

    m_current_genome.xforms[xform_idx].weight = weight;
    m_current_genome.xforms[xform_idx].color_index = color_idx;
    m_current_genome.xforms[xform_idx].color_speed = color_speed;
    m_current_genome.xforms[xform_idx].opacity = opacity;
    m_current_genome.xforms[xform_idx].visible = visible;
    m_current_genome.compile();

    trigger_interactive_preview();
}

void MainWindow::on_global_settings_modified() {
    if (m_updating_ui) return;
    if (m_flame_inspector) {
        m_flame_inspector->read_genome(m_current_genome);
    }
    on_action_render();
}

void MainWindow::on_transform_editor_modified(size_t index, const Core::Affine2D& affine, bool finished) {
    if (index >= m_current_genome.xforms.size()) return;

    if (!finished) {
        m_current_genome.xforms[index].affine = affine;
        m_current_genome.compile();
        trigger_interactive_preview();
    } else {
        Core::Affine2D old_affine = m_current_genome.xforms[index].affine;
        auto* cmd = new Core::Commands::ChangeXformMatrixCommand(&m_current_genome, index, false, old_affine, affine, [this]() {
            load_genome_to_ui(m_current_genome);
            trigger_interactive_preview();
        });
        m_undo_stack.push(cmd);
    }
}

void MainWindow::on_post_transform_editor_modified(size_t index, const Core::Affine2D& post_affine, bool finished) {
    if (index >= m_current_genome.xforms.size()) return;

    if (!finished) {
        m_current_genome.xforms[index].post_affine = post_affine;
        m_current_genome.xforms[index].has_post_affine = true;
        m_current_genome.compile();
        trigger_interactive_preview();
    } else {
        Core::Affine2D old_post = m_current_genome.xforms[index].post_affine;
        auto* cmd = new Core::Commands::ChangeXformMatrixCommand(&m_current_genome, index, true, old_post, post_affine, [this]() {
            load_genome_to_ui(m_current_genome);
            trigger_interactive_preview();
        });
        m_undo_stack.push(cmd);
    }
}

void MainWindow::on_transform_editor_structure_changed() {
    if (m_flame_inspector) {
        m_flame_inspector->set_genome(m_current_genome);
    }
    if (m_variation_inspector) {
        m_variation_inspector->set_genome(m_current_genome);
    }
    on_action_render();
}

void MainWindow::on_xform_selection_changed(size_t index) {
    if (m_flame_inspector) {
        m_flame_inspector->set_active_xform_index(index);
    }
    if (m_variation_inspector) {
        m_variation_inspector->set_active_xform_index(index);
    }
}

void MainWindow::on_variation_modified(size_t xform_idx, const std::string& name, double weight) {
    if (xform_idx >= m_current_genome.xforms.size()) return;

    double old_w = m_current_genome.xforms[xform_idx].get_variation(name);
    auto* cmd = new Core::Commands::ChangeVariationWeightCommand(&m_current_genome, xform_idx, name, old_w, weight, [this]() {
        load_genome_to_ui(m_current_genome);
        trigger_interactive_preview();
    });
    m_undo_stack.push(cmd);
}

void MainWindow::on_param_modified(size_t xform_idx, const std::string& name, double val) {
    if (xform_idx >= m_current_genome.xforms.size()) return;

    double old_v = m_current_genome.xforms[xform_idx].get_param(name, 0.0);
    auto* cmd = new Core::Commands::ChangeParamValueCommand(&m_current_genome, xform_idx, name, old_v, val, [this]() {
        load_genome_to_ui(m_current_genome);
        trigger_interactive_preview();
    });
    m_undo_stack.push(cmd);
}

void MainWindow::on_variations_cleared(size_t xform_idx) {
    if (xform_idx < m_current_genome.xforms.size()) {
        trigger_interactive_preview();
    }
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
        QImage frame(buffer.data(), w, h, w * 4, QImage::Format_RGBA8888);
        m_canvas->set_rendered_image(frame.copy());
    }
}

void MainWindow::on_canvas_coordinate_hovered(double x, double y) {
    m_coord_label->setText(tr("X: %1, Y: %2").arg(x, 0, 'f', 2).arg(y, 0, 'f', 2));
}

} // namespace ApoNeo::UI
