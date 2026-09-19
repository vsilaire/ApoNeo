#pragma once

#include "core/Affine2D.hpp"
#include <QColor>
#include <QGraphicsItem>
#include <QObject>
#include <functional>

namespace ApoNeo::UI {

class XformGraphicItem : public QGraphicsItem {
public:
    enum HandleType {
        Handle_None,
        Handle_Origin, // O' (c, f)
        Handle_X,      // X' (a+c, d+f) - Primary axis (controls scale & rotation, keeps OY perpendicular)
        Handle_Y,      // Y' (b+c, e+f) - Orthogonal axis (controls Y scale; shears when Shift/Alt or skew mode enabled)
        Handle_Body    // Triangle body (Translation)
    };

    enum TransformMode {
        Mode_PreAffine,  // Standard Pre-Transform A
        Mode_PostAffine  // Post-Transform P
    };

    static constexpr double SCENE_SCALE = 100.0; // 100 pixels per math unit

    explicit XformGraphicItem(size_t xform_index, QGraphicsItem* parent = nullptr);

    size_t xform_index() const noexcept { return m_index; }
    void set_xform_index(size_t idx) { m_index = idx; update(); }

    void set_transform_mode(TransformMode mode) { m_mode = mode; update(); }
    TransformMode transform_mode() const noexcept { return m_mode; }

    void set_color(const QColor& color) { m_color = color; update(); }
    const QColor& color() const noexcept { return m_color; }

    void set_selected_visual(bool selected) { m_is_selected = selected; update(); }
    bool is_selected_visual() const noexcept { return m_is_selected; }

    void set_affine(const Core::Affine2D& affine);
    Core::Affine2D get_affine() const noexcept;

    void set_snap_grid(bool enabled, double step = 0.1) {
        m_snap_enabled = enabled;
        m_snap_step = step;
    }

    void set_skew_mode(bool enabled) {
        m_skew_mode_enabled = enabled;
    }
    bool is_skew_mode() const noexcept { return m_skew_mode_enabled; }

    // Callbacks for live interaction
    using ChangeCallback = std::function<void(size_t index, const Core::Affine2D& affine, bool finished)>;
    void set_change_callback(ChangeCallback cb) { m_change_callback = std::move(cb); }

    // QGraphicsItem interface
    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;

private:
    QPointF math_to_scene(double mx, double my) const noexcept;
    void scene_to_math(const QPointF& sp, double& mx, double& my) const noexcept;
    double snap_value(double val) const noexcept;
    HandleType hit_test_handle(const QPointF& pos) const;

    size_t m_index = 0;
    TransformMode m_mode = Mode_PreAffine;
    Core::Affine2D m_affine;
    QColor m_color = QColor(100, 180, 255);
    bool m_is_selected = false;

    bool m_snap_enabled = false;
    double m_snap_step = 0.1;
    bool m_skew_mode_enabled = false;

    HandleType m_active_handle = Handle_None;
    QPointF m_drag_start_pos;
    Core::Affine2D m_drag_start_affine;
    double m_drag_start_ratio = 1.0;
    double m_drag_start_det_sign = 1.0;

    ChangeCallback m_change_callback;
};

} // namespace ApoNeo::UI
