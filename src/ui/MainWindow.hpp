#pragma once

#include "FlameCanvasWidget.hpp"
#include "core/FlameGenome.hpp"
#include "engine/CPUSIMDEngine.hpp"
#include <QComboBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QMainWindow>
#include <QProgressBar>
#include <QSpinBox>
#include <QTimer>
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
    void on_action_reset_view();
    void on_action_about();

    void on_preset_changed(int index);
    void on_palette_changed(int index);
    void on_parameters_modified();
    void on_poll_timer_tick();
    void on_canvas_coordinate_hovered(double x, double y);

private:
    void setup_ui();
    void setup_menus();
    void setup_toolbar();
    void setup_dock_panel();
    void setup_statusbar();
    void apply_modern_theme();
    void load_genome_to_ui(const Core::FlameGenome& genome);
    void read_genome_from_ui(Core::FlameGenome& genome);

    FlameCanvasWidget* m_canvas = nullptr;
    QDockWidget* m_dock = nullptr;

    // Controls
    QSpinBox* m_spin_width = nullptr;
    QSpinBox* m_spin_height = nullptr;
    QSpinBox* m_spin_supersample = nullptr;
    QDoubleSpinBox* m_spin_quality = nullptr;
    QDoubleSpinBox* m_spin_scale = nullptr;
    QDoubleSpinBox* m_spin_center_x = nullptr;
    QDoubleSpinBox* m_spin_center_y = nullptr;
    QDoubleSpinBox* m_spin_rotate = nullptr;
    QDoubleSpinBox* m_spin_zoom = nullptr;

    QDoubleSpinBox* m_spin_gamma = nullptr;
    QDoubleSpinBox* m_spin_brightness = nullptr;
    QDoubleSpinBox* m_spin_vibrancy = nullptr;

    QComboBox* m_combo_preset = nullptr;
    QComboBox* m_combo_palette = nullptr;

    // Status bar
    QProgressBar* m_progress_bar = nullptr;
    QLabel* m_status_label = nullptr;
    QLabel* m_coord_label = nullptr;

    // Actions
    QAction* m_act_render = nullptr;
    QAction* m_act_pause = nullptr;
    QAction* m_act_stop = nullptr;

    // State & Engine
    Core::FlameGenome m_current_genome;
    std::unique_ptr<Engine::IRenderEngine> m_engine;
    QTimer m_poll_timer;
    bool m_updating_ui = false;
};

} // namespace ApoNeo::UI
