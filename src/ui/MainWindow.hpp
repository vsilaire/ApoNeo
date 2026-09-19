#pragma once

#include "AnimationWidget.hpp"
#include "FlameCanvasWidget.hpp"
#include "FlameInspectorWidget.hpp"
#include "FlameListWidget.hpp"
#include "TransformEditorWidget.hpp"
#include "VariationInspectorWidget.hpp"
#include "core/FlameGenome.hpp"
#include "core/RandomFlameGenerator.hpp"
#include "core/commands/Commands.hpp"
#include "engine/CPUSIMDEngine.hpp"
#include <QComboBox>
#include <QDockWidget>
#include <QLabel>
#include <QMainWindow>
#include <QProgressBar>
#include <QTimer>
#include <QUndoStack>
#include <memory>

namespace ApoNeo::UI {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void on_action_render();
    void on_action_pause();
    void on_action_stop();
    void on_action_open_flame();
    void on_action_save_flame();
    void on_action_export_image();
    void on_action_export_animation();
    void on_action_generate_batch();
    void on_action_reset_view();
    void on_action_about();

    void on_preset_changed(int index);
    void on_poll_timer_tick();
    void on_canvas_coordinate_hovered(double x, double y);

    // Flame Inspector slots (Animatable vs Global)
    void on_camera_modified(double scale, double cx, double cy, double rot, double zoom);
    void on_tonemap_modified(double gamma, double brightness, double vibrancy);
    void on_palette_preset_changed(int index);
    void on_xform_props_modified(size_t xform_idx, double weight, double color_idx, double color_speed, double opacity, bool visible);
    void on_global_settings_modified();

    // Flame batch list synchronization
    void on_flame_batch_selected(const Core::FlameGenome& genome);

    // Animation timeline synchronization
    void on_animation_frame_rendered(const Core::FlameGenome& genome, int frame);
    void on_animation_add_keyframe();

    // Transform editor synchronization
    void on_transform_editor_modified(size_t index, const Core::Affine2D& affine, bool finished);
    void on_post_transform_editor_modified(size_t index, const Core::Affine2D& post_affine, bool finished);
    void on_transform_editor_structure_changed();
    void on_xform_selection_changed(size_t index);

    // Variation inspector synchronization
    void on_variation_modified(size_t xform_idx, const std::string& name, double weight);
    void on_param_modified(size_t xform_idx, const std::string& name, double val);
    void on_variations_cleared(size_t xform_idx);

private:
    void setup_ui();
    void setup_menus();
    void setup_toolbar();
    void setup_dock_panel();
    void setup_flame_batch_dock();
    void setup_transform_editor_dock();
    void setup_variation_inspector_dock();
    void setup_animation_dock();
    void setup_statusbar();
    void apply_modern_theme();
    void load_genome_to_ui(const Core::FlameGenome& genome);
    void trigger_interactive_preview();
    void sync_active_keyframe_if_present();

    FlameCanvasWidget* m_canvas = nullptr;
    QDockWidget* m_dock = nullptr;
    FlameInspectorWidget* m_flame_inspector = nullptr;

    QDockWidget* m_batch_dock = nullptr;
    FlameListWidget* m_flame_list = nullptr;

    QDockWidget* m_transform_dock = nullptr;
    TransformEditorWidget* m_transform_editor = nullptr;

    QDockWidget* m_variation_dock = nullptr;
    VariationInspectorWidget* m_variation_inspector = nullptr;

    QDockWidget* m_animation_dock = nullptr;
    AnimationWidget* m_animation_widget = nullptr;

    QComboBox* m_combo_preset = nullptr;

    // Status bar
    QProgressBar* m_progress_bar = nullptr;
    QLabel* m_status_label = nullptr;
    QLabel* m_coord_label = nullptr;

    // Actions
    QAction* m_act_undo = nullptr;
    QAction* m_act_redo = nullptr;
    QAction* m_act_render = nullptr;
    QAction* m_act_pause = nullptr;
    QAction* m_act_stop = nullptr;
    QAction* m_act_generate_batch = nullptr;
    QAction* m_act_export_anim = nullptr;

    // State & Engine
    Core::FlameGenome m_current_genome;
    Core::RandomFlameGenerator m_random_generator;
    QUndoStack m_undo_stack;
    std::unique_ptr<Engine::IRenderEngine> m_engine;
    QTimer m_poll_timer;
    bool m_updating_ui = false;
};

} // namespace ApoNeo::UI
