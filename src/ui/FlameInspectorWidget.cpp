#include "FlameInspectorWidget.hpp"
#include <QColorDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>
#include <cmath>

namespace ApoNeo::UI {

FlameInspectorWidget::FlameInspectorWidget(QWidget* parent)
    : QWidget(parent) {
    setup_ui();
}

void FlameInspectorWidget::setup_ui() {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(4, 4, 4, 4);
    main_layout->setSpacing(6);

    m_tabs = new QTabWidget(this);
    m_tabs->setStyleSheet(
        "QTabWidget::pane {"
        "  border: 1px solid #2e3440;"
        "  background: #14171c;"
        "  border-radius: 4px;"
        "}"
        "QTabBar::tab {"
        "  background: #1e222b;"
        "  color: #d8dee9;"
        "  padding: 8px 14px;"
        "  margin-right: 2px;"
        "  border-top-left-radius: 4px;"
        "  border-top-right-radius: 4px;"
        "  font-weight: 600;"
        "}"
        "QTabBar::tab:selected {"
        "  background: #2e3440;"
        "  color: #88c0d0;"
        "  border-bottom: 2px solid #88c0d0;"
        "}"
    );

    m_tabs->addTab(create_animatable_tab(), tr("🎬 Animatable State"));
    m_tabs->addTab(create_global_tab(), tr("⚙️ Global Settings"));

    main_layout->addWidget(m_tabs);
}

QWidget* FlameInspectorWidget::create_animatable_tab() {
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* container = new QWidget(scroll);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(10);

    // Keyframe Badge Notice
    auto* badge_frame = new QFrame(container);
    badge_frame->setStyleSheet(
        "QFrame {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #2a1b40, stop:1 #1e222b);"
        "  border: 1px solid #7928ca;"
        "  border-radius: 6px;"
        "  padding: 4px;"
        "}"
    );
    auto* badge_layout = new QHBoxLayout(badge_frame);
    badge_layout->setContentsMargins(8, 4, 8, 4);
    auto* badge_icon = new QLabel(tr("✨ <b>Keyframe-Ready</b>"), badge_frame);
    badge_icon->setStyleSheet("color: #ff0080; font-size: 11px;");
    badge_layout->addWidget(badge_icon);
    auto* badge_desc = new QLabel(tr("Interpolates smoothly in animation timeline"), badge_frame);
    badge_desc->setStyleSheet("color: #b0b0c0; font-size: 11px;");
    badge_layout->addWidget(badge_desc, 1);
    layout->addWidget(badge_frame);

    // Camera & Framing Group
    auto* grp_camera = new QGroupBox(tr("Camera & Viewport"), container);
    auto* fl_camera = new QFormLayout(grp_camera);
    fl_camera->setLabelAlignment(Qt::AlignRight);

    m_spin_scale = new QDoubleSpinBox(grp_camera);
    m_spin_scale->setRange(0.1, 1000000.0);
    m_spin_scale->setSingleStep(10.0);
    m_spin_scale->setValue(150.0);
    fl_camera->addRow(tr("Scale (Zoom):"), m_spin_scale);

    m_spin_center_x = new QDoubleSpinBox(grp_camera);
    m_spin_center_x->setRange(-10000.0, 10000.0);
    m_spin_center_x->setSingleStep(0.1);
    m_spin_center_x->setDecimals(4);
    fl_camera->addRow(tr("Center X (Pan):"), m_spin_center_x);

    m_spin_center_y = new QDoubleSpinBox(grp_camera);
    m_spin_center_y->setRange(-10000.0, 10000.0);
    m_spin_center_y->setSingleStep(0.1);
    m_spin_center_y->setDecimals(4);
    fl_camera->addRow(tr("Center Y (Pan):"), m_spin_center_y);

    m_spin_rotate = new QDoubleSpinBox(grp_camera);
    m_spin_rotate->setRange(-360.0, 360.0);
    m_spin_rotate->setSingleStep(5.0);
    m_spin_rotate->setSuffix(tr("°"));
    fl_camera->addRow(tr("Rotation:"), m_spin_rotate);

    m_spin_zoom = new QDoubleSpinBox(grp_camera);
    m_spin_zoom->setRange(-20.0, 20.0);
    m_spin_zoom->setSingleStep(0.1);
    fl_camera->addRow(tr("Perspective Zoom:"), m_spin_zoom);

    layout->addWidget(grp_camera);

    // Tone Mapping & Color Group
    auto* grp_tone = new QGroupBox(tr("Tone Mapping & Palette"), container);
    auto* fl_tone = new QFormLayout(grp_tone);
    fl_tone->setLabelAlignment(Qt::AlignRight);

    m_combo_palette = new QComboBox(grp_tone);
    m_combo_palette->addItem(tr("Fire (Standard)"));
    m_combo_palette->addItem(tr("Electric Blue"));
    m_combo_palette->addItem(tr("Rainbow Neon"));
    m_combo_palette->addItem(tr("Aurora Borealis"));
    m_combo_palette->addItem(tr("Sunset Glow"));
    m_combo_palette->addItem(tr("Monochrome"));
    fl_tone->addRow(tr("Color Palette:"), m_combo_palette);

    m_spin_gamma = new QDoubleSpinBox(grp_tone);
    m_spin_gamma->setRange(0.1, 20.0);
    m_spin_gamma->setSingleStep(0.2);
    m_spin_gamma->setValue(4.0);
    fl_tone->addRow(tr("Gamma:"), m_spin_gamma);

    m_spin_brightness = new QDoubleSpinBox(grp_tone);
    m_spin_brightness->setRange(0.01, 50.0);
    m_spin_brightness->setSingleStep(0.1);
    m_spin_brightness->setValue(1.0);
    fl_tone->addRow(tr("Brightness:"), m_spin_brightness);

    m_spin_vibrancy = new QDoubleSpinBox(grp_tone);
    m_spin_vibrancy->setRange(0.0, 1.0);
    m_spin_vibrancy->setSingleStep(0.05);
    m_spin_vibrancy->setValue(1.0);
    fl_tone->addRow(tr("Vibrancy:"), m_spin_vibrancy);

    layout->addWidget(grp_tone);

    // Active Transform Animatable Properties Group
    auto* grp_xform = new QGroupBox(tr("Active Transform Properties"), container);
    auto* fl_xform = new QFormLayout(grp_xform);
    fl_xform->setLabelAlignment(Qt::AlignRight);

    m_combo_xform_select = new QComboBox(grp_xform);
    fl_xform->addRow(tr("Target Xform:"), m_combo_xform_select);

    m_spin_xf_weight = new QDoubleSpinBox(grp_xform);
    m_spin_xf_weight->setRange(0.0, 100.0);
    m_spin_xf_weight->setSingleStep(0.1);
    m_spin_xf_weight->setValue(1.0);
    fl_xform->addRow(tr("Transform Weight:"), m_spin_xf_weight);

    m_spin_xf_color_idx = new QDoubleSpinBox(grp_xform);
    m_spin_xf_color_idx->setRange(0.0, 1.0);
    m_spin_xf_color_idx->setSingleStep(0.05);
    m_spin_xf_color_idx->setValue(0.0);
    fl_xform->addRow(tr("Palette Index:"), m_spin_xf_color_idx);

    m_spin_xf_color_speed = new QDoubleSpinBox(grp_xform);
    m_spin_xf_color_speed->setRange(0.0, 1.0);
    m_spin_xf_color_speed->setSingleStep(0.05);
    m_spin_xf_color_speed->setValue(0.5);
    fl_xform->addRow(tr("Color Speed:"), m_spin_xf_color_speed);

    m_spin_xf_opacity = new QDoubleSpinBox(grp_xform);
    m_spin_xf_opacity->setRange(0.0, 1.0);
    m_spin_xf_opacity->setSingleStep(0.05);
    m_spin_xf_opacity->setValue(1.0);
    fl_xform->addRow(tr("Opacity:"), m_spin_xf_opacity);

    m_chk_xf_visible = new QCheckBox(tr("Visible"), grp_xform);
    m_chk_xf_visible->setChecked(true);
    fl_xform->addRow(tr("Status:"), m_chk_xf_visible);

    layout->addWidget(grp_xform);
    layout->addStretch(1);

    // Connect animatable controls
    connect(m_spin_scale, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FlameInspectorWidget::on_camera_changed);
    connect(m_spin_center_x, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FlameInspectorWidget::on_camera_changed);
    connect(m_spin_center_y, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FlameInspectorWidget::on_camera_changed);
    connect(m_spin_rotate, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FlameInspectorWidget::on_camera_changed);
    connect(m_spin_zoom, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FlameInspectorWidget::on_camera_changed);

    connect(m_spin_gamma, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FlameInspectorWidget::on_tonemap_changed);
    connect(m_spin_brightness, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FlameInspectorWidget::on_tonemap_changed);
    connect(m_spin_vibrancy, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FlameInspectorWidget::on_tonemap_changed);
    connect(m_combo_palette, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FlameInspectorWidget::on_palette_changed);

    connect(m_combo_xform_select, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        if (idx >= 0) set_active_xform_index(static_cast<size_t>(idx));
    });
    connect(m_spin_xf_weight, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FlameInspectorWidget::on_xform_props_changed);
    connect(m_spin_xf_color_idx, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FlameInspectorWidget::on_xform_props_changed);
    connect(m_spin_xf_color_speed, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FlameInspectorWidget::on_xform_props_changed);
    connect(m_spin_xf_opacity, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FlameInspectorWidget::on_xform_props_changed);
    connect(m_chk_xf_visible, &QCheckBox::toggled, this, &FlameInspectorWidget::on_xform_props_changed);

    scroll->setWidget(container);
    return scroll;
}

QWidget* FlameInspectorWidget::create_global_tab() {
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* container = new QWidget(scroll);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(10);

    // Metadata Group
    auto* grp_meta = new QGroupBox(tr("Flame Metadata"), container);
    auto* fl_meta = new QFormLayout(grp_meta);
    fl_meta->setLabelAlignment(Qt::AlignRight);

    m_edit_flame_name = new QLineEdit(grp_meta);
    m_edit_flame_name->setText(tr("Untitled Flame"));
    fl_meta->addRow(tr("Name:"), m_edit_flame_name);

    layout->addWidget(grp_meta);

    // Render Canvas & Quality Group
    auto* grp_render = new QGroupBox(tr("Render Output & Quality"), container);
    auto* fl_render = new QFormLayout(grp_render);
    fl_render->setLabelAlignment(Qt::AlignRight);

    m_spin_width = new QSpinBox(grp_render);
    m_spin_width->setRange(128, 7680);
    m_spin_width->setSingleStep(64);
    m_spin_width->setValue(800);
    fl_render->addRow(tr("Width (px):"), m_spin_width);

    m_spin_height = new QSpinBox(grp_render);
    m_spin_height->setRange(128, 4320);
    m_spin_height->setSingleStep(64);
    m_spin_height->setValue(600);
    fl_render->addRow(tr("Height (px):"), m_spin_height);

    m_spin_supersample = new QSpinBox(grp_render);
    m_spin_supersample->setRange(1, 4);
    m_spin_supersample->setValue(2);
    fl_render->addRow(tr("Spatial Supersample:"), m_spin_supersample);

    m_spin_quality = new QDoubleSpinBox(grp_render);
    m_spin_quality->setRange(1.0, 50000.0);
    m_spin_quality->setSingleStep(50.0);
    m_spin_quality->setValue(100.0);
    fl_render->addRow(tr("Sample Density:"), m_spin_quality);

    m_spin_fuse = new QSpinBox(grp_render);
    m_spin_fuse->setRange(0, 500);
    m_spin_fuse->setValue(30);
    fl_render->addRow(tr("Fuse Warmup Iters:"), m_spin_fuse);

    m_btn_bg_color = new QPushButton(tr("Choose Background Color..."), grp_render);
    m_bg_color = Core::ColorRGBA(0.0f, 0.0f, 0.0f, 1.0f);
    connect(m_btn_bg_color, &QPushButton::clicked, this, &FlameInspectorWidget::on_bg_color_btn_clicked);
    fl_render->addRow(tr("Background:"), m_btn_bg_color);

    layout->addWidget(grp_render);
    layout->addStretch(1);

    // Connect global controls
    connect(m_edit_flame_name, &QLineEdit::editingFinished, this, &FlameInspectorWidget::on_global_changed);
    connect(m_spin_width, QOverload<int>::of(&QSpinBox::valueChanged), this, &FlameInspectorWidget::on_global_changed);
    connect(m_spin_height, QOverload<int>::of(&QSpinBox::valueChanged), this, &FlameInspectorWidget::on_global_changed);
    connect(m_spin_supersample, QOverload<int>::of(&QSpinBox::valueChanged), this, &FlameInspectorWidget::on_global_changed);
    connect(m_spin_quality, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FlameInspectorWidget::on_global_changed);
    connect(m_spin_fuse, QOverload<int>::of(&QSpinBox::valueChanged), this, &FlameInspectorWidget::on_global_changed);

    scroll->setWidget(container);
    return scroll;
}

void FlameInspectorWidget::set_genome(const Core::FlameGenome& genome) {
    m_is_updating = true;
    m_cached_genome = genome;

    // Block signals on all controls to prevent redundant mutation cascades
    m_spin_scale->blockSignals(true);
    m_spin_center_x->blockSignals(true);
    m_spin_center_y->blockSignals(true);
    m_spin_rotate->blockSignals(true);
    m_spin_zoom->blockSignals(true);
    m_spin_gamma->blockSignals(true);
    m_spin_brightness->blockSignals(true);
    m_spin_vibrancy->blockSignals(true);
    m_edit_flame_name->blockSignals(true);
    m_spin_width->blockSignals(true);
    m_spin_height->blockSignals(true);
    m_spin_supersample->blockSignals(true);
    m_spin_quality->blockSignals(true);
    m_spin_fuse->blockSignals(true);
    m_combo_xform_select->blockSignals(true);

    m_spin_scale->setValue(genome.scale);
    m_spin_center_x->setValue(genome.center_x);
    m_spin_center_y->setValue(genome.center_y);
    m_spin_rotate->setValue(genome.rotate * 180.0 / M_PI);
    m_spin_zoom->setValue(genome.zoom);

    m_spin_gamma->setValue(genome.tone_map.gamma);
    m_spin_brightness->setValue(genome.tone_map.brightness);
    m_spin_vibrancy->setValue(genome.tone_map.vibrancy);

    m_edit_flame_name->setText(QString::fromStdString(genome.name));
    m_spin_width->setValue(genome.width);
    m_spin_height->setValue(genome.height);
    m_spin_supersample->setValue(genome.supersample);
    m_spin_quality->setValue(genome.quality);
    m_spin_fuse->setValue(genome.fuse_iterations);
    m_bg_color = genome.background;

    // Update Xform list
    m_combo_xform_select->clear();
    for (size_t i = 0; i < genome.xforms.size(); ++i) {
        m_combo_xform_select->addItem(QString("Xform %1").arg(i + 1));
    }
    if (m_active_xform_idx >= genome.xforms.size()) {
        m_active_xform_idx = 0;
    }
    if (!genome.xforms.empty()) {
        m_combo_xform_select->setCurrentIndex(static_cast<int>(m_active_xform_idx));
        set_active_xform_index(m_active_xform_idx);
    }

    m_spin_scale->blockSignals(false);
    m_spin_center_x->blockSignals(false);
    m_spin_center_y->blockSignals(false);
    m_spin_rotate->blockSignals(false);
    m_spin_zoom->blockSignals(false);
    m_spin_gamma->blockSignals(false);
    m_spin_brightness->blockSignals(false);
    m_spin_vibrancy->blockSignals(false);
    m_edit_flame_name->blockSignals(false);
    m_spin_width->blockSignals(false);
    m_spin_height->blockSignals(false);
    m_spin_supersample->blockSignals(false);
    m_spin_quality->blockSignals(false);
    m_spin_fuse->blockSignals(false);
    m_combo_xform_select->blockSignals(false);

    m_is_updating = false;
}

void FlameInspectorWidget::set_active_xform_index(size_t index) {
    m_active_xform_idx = index;
    if (index >= m_cached_genome.xforms.size()) return;

    const auto& xf = m_cached_genome.xforms[index];
    m_spin_xf_weight->blockSignals(true);
    m_spin_xf_color_idx->blockSignals(true);
    m_spin_xf_color_speed->blockSignals(true);
    m_spin_xf_opacity->blockSignals(true);
    m_chk_xf_visible->blockSignals(true);

    m_spin_xf_weight->setValue(xf.weight);
    m_spin_xf_color_idx->setValue(xf.color_index);
    m_spin_xf_color_speed->setValue(xf.color_speed);
    m_spin_xf_opacity->setValue(xf.opacity);
    m_chk_xf_visible->setChecked(xf.visible);

    m_spin_xf_weight->blockSignals(false);
    m_spin_xf_color_idx->blockSignals(false);
    m_spin_xf_color_speed->blockSignals(false);
    m_spin_xf_opacity->blockSignals(false);
    m_chk_xf_visible->blockSignals(false);
}

void FlameInspectorWidget::read_genome(Core::FlameGenome& genome) const {
    genome.name = m_edit_flame_name->text().toStdString();
    genome.width = m_spin_width->value();
    genome.height = m_spin_height->value();
    genome.supersample = m_spin_supersample->value();
    genome.quality = m_spin_quality->value();
    genome.fuse_iterations = m_spin_fuse->value();
    genome.background = m_bg_color;

    genome.scale = m_spin_scale->value();
    genome.center_x = m_spin_center_x->value();
    genome.center_y = m_spin_center_y->value();
    genome.rotate = m_spin_rotate->value() * M_PI / 180.0;
    genome.zoom = m_spin_zoom->value();

    genome.tone_map.gamma = m_spin_gamma->value();
    genome.tone_map.brightness = m_spin_brightness->value();
    genome.tone_map.vibrancy = m_spin_vibrancy->value();
}

void FlameInspectorWidget::on_camera_changed() {
    if (m_is_updating) return;
    double scale = m_spin_scale->value();
    double cx = m_spin_center_x->value();
    double cy = m_spin_center_y->value();
    double rot = m_spin_rotate->value() * M_PI / 180.0;
    double zoom = m_spin_zoom->value();

    m_cached_genome.scale = scale;
    m_cached_genome.center_x = cx;
    m_cached_genome.center_y = cy;
    m_cached_genome.rotate = rot;
    m_cached_genome.zoom = zoom;

    emit camera_modified(scale, cx, cy, rot, zoom);
}

void FlameInspectorWidget::on_tonemap_changed() {
    if (m_is_updating) return;
    double gamma = m_spin_gamma->value();
    double brightness = m_spin_brightness->value();
    double vibrancy = m_spin_vibrancy->value();

    m_cached_genome.tone_map.gamma = gamma;
    m_cached_genome.tone_map.brightness = brightness;
    m_cached_genome.tone_map.vibrancy = vibrancy;

    emit tonemap_modified(gamma, brightness, vibrancy);
}

void FlameInspectorWidget::on_palette_changed(int index) {
    if (m_is_updating) return;
    emit palette_preset_changed(index);
}

void FlameInspectorWidget::on_xform_props_changed() {
    if (m_is_updating) return;
    double weight = m_spin_xf_weight->value();
    double color_idx = m_spin_xf_color_idx->value();
    double color_speed = m_spin_xf_color_speed->value();
    double opacity = m_spin_xf_opacity->value();
    bool visible = m_chk_xf_visible->isChecked();

    if (m_active_xform_idx < m_cached_genome.xforms.size()) {
        m_cached_genome.xforms[m_active_xform_idx].weight = weight;
        m_cached_genome.xforms[m_active_xform_idx].color_index = color_idx;
        m_cached_genome.xforms[m_active_xform_idx].color_speed = color_speed;
        m_cached_genome.xforms[m_active_xform_idx].opacity = opacity;
        m_cached_genome.xforms[m_active_xform_idx].visible = visible;
    }

    emit xform_props_modified(m_active_xform_idx, weight, color_idx, color_speed, opacity, visible);
}

void FlameInspectorWidget::on_global_changed() {
    if (m_is_updating) return;
    emit global_settings_modified();
}

void FlameInspectorWidget::on_bg_color_btn_clicked() {
    QColor cur(static_cast<int>(m_bg_color.r * 255), static_cast<int>(m_bg_color.g * 255), static_cast<int>(m_bg_color.b * 255));
    QColor c = QColorDialog::getColor(cur, this, tr("Select Background Color"));
    if (c.isValid()) {
        m_bg_color = Core::ColorRGBA(c.redF(), c.greenF(), c.blueF(), 1.0f);
        m_cached_genome.background = m_bg_color;
        on_global_changed();
    }
}

} // namespace ApoNeo::UI
