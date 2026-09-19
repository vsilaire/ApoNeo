#pragma once

#include "core/FlameGenome.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QWidget>

namespace ApoNeo::UI {

class FlameInspectorWidget : public QWidget {
    Q_OBJECT

public:
    explicit FlameInspectorWidget(QWidget* parent = nullptr);
    ~FlameInspectorWidget() override = default;

    /// @brief Populate all inspector widgets from the genome model
    void set_genome(const Core::FlameGenome& genome);

    /// @brief Set the active Xform index for Xform-specific animatable properties
    void set_active_xform_index(size_t index);

    /// @brief Read all current values back into the genome model
    void read_genome(Core::FlameGenome& genome) const;

signals:
    // Animatable properties (keyframeable)
    void camera_modified(double scale, double cx, double cy, double rot, double zoom);
    void tonemap_modified(double gamma, double brightness, double vibrancy);
    void palette_preset_changed(int index);
    void xform_props_modified(size_t xform_idx, double weight, double color_idx, double color_speed, double opacity, bool visible);

    // Global / static properties
    void global_settings_modified();

private slots:
    void on_camera_changed();
    void on_tonemap_changed();
    void on_palette_changed(int index);
    void on_xform_props_changed();
    void on_global_changed();
    void on_bg_color_btn_clicked();

private:
    void setup_ui();
    QWidget* create_animatable_tab();
    QWidget* create_global_tab();

    QTabWidget* m_tabs = nullptr;

    // Tab 1: Animatable State Controls
    // Camera
    QDoubleSpinBox* m_spin_scale = nullptr;
    QDoubleSpinBox* m_spin_center_x = nullptr;
    QDoubleSpinBox* m_spin_center_y = nullptr;
    QDoubleSpinBox* m_spin_rotate = nullptr;
    QDoubleSpinBox* m_spin_zoom = nullptr;

    // Tone Map & Palette
    QDoubleSpinBox* m_spin_gamma = nullptr;
    QDoubleSpinBox* m_spin_brightness = nullptr;
    QDoubleSpinBox* m_spin_vibrancy = nullptr;
    QComboBox* m_combo_palette = nullptr;

    // Active Xform animatable properties
    QComboBox* m_combo_xform_select = nullptr;
    QDoubleSpinBox* m_spin_xf_weight = nullptr;
    QDoubleSpinBox* m_spin_xf_color_idx = nullptr;
    QDoubleSpinBox* m_spin_xf_color_speed = nullptr;
    QDoubleSpinBox* m_spin_xf_opacity = nullptr;
    QCheckBox* m_chk_xf_visible = nullptr;

    // Tab 2: Global & Render Settings
    QLineEdit* m_edit_flame_name = nullptr;
    QSpinBox* m_spin_width = nullptr;
    QSpinBox* m_spin_height = nullptr;
    QSpinBox* m_spin_supersample = nullptr;
    QDoubleSpinBox* m_spin_quality = nullptr;
    QSpinBox* m_spin_fuse = nullptr;
    QPushButton* m_btn_bg_color = nullptr;
    Core::ColorRGBA m_bg_color;

    Core::FlameGenome m_cached_genome;
    size_t m_active_xform_idx = 0;
    bool m_is_updating = false;
};

} // namespace ApoNeo::UI
