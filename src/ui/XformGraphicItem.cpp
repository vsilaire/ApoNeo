#include "XformGraphicItem.hpp"
#include <QCursor>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <algorithm>
#include <cmath>

namespace ApoNeo::UI {

XformGraphicItem::XformGraphicItem(size_t xform_index, QGraphicsItem* parent)
    : QGraphicsItem(parent), m_index(xform_index) {
    setAcceptHoverEvents(true);
    setFlags(ItemIsSelectable | ItemSendsGeometryChanges);
    setZValue(10.0 + static_cast<double>(xform_index));
}

void XformGraphicItem::set_affine(const Core::Affine2D& affine) {
    prepareGeometryChange();
    m_affine = affine;
    update();
}

Core::Affine2D XformGraphicItem::get_affine() const noexcept {
    return m_affine;
}

QPointF XformGraphicItem::math_to_scene(double mx, double my) const noexcept {
    return QPointF(mx * SCENE_SCALE, -my * SCENE_SCALE);
}

void XformGraphicItem::scene_to_math(const QPointF& sp, double& mx, double& my) const noexcept {
    mx = sp.x() / SCENE_SCALE;
    my = -sp.y() / SCENE_SCALE;
}

double XformGraphicItem::snap_value(double val) const noexcept {
    if (!m_snap_enabled || m_snap_step <= 1e-6) return val;
    return std::round(val / m_snap_step) * m_snap_step;
}

QRectF XformGraphicItem::boundingRect() const {
    QPointF pO = math_to_scene(m_affine.c, m_affine.f);
    QPointF pX = math_to_scene(m_affine.a + m_affine.c, m_affine.d + m_affine.f);
    QPointF pY = math_to_scene(m_affine.b + m_affine.c, m_affine.e + m_affine.f);

    qreal min_x = std::min({pO.x(), pX.x(), pY.x()}) - 24.0;
    qreal max_x = std::max({pO.x(), pX.x(), pY.x()}) + 24.0;
    qreal min_y = std::min({pO.y(), pY.y(), pX.y()}) - 24.0;
    qreal max_y = std::max({pO.y(), pY.y(), pX.y()}) + 24.0;

    return QRectF(min_x, min_y, max_x - min_x, max_y - min_y);
}

QPainterPath XformGraphicItem::shape() const {
    QPointF pO = math_to_scene(m_affine.c, m_affine.f);
    QPointF pX = math_to_scene(m_affine.a + m_affine.c, m_affine.d + m_affine.f);
    QPointF pY = math_to_scene(m_affine.b + m_affine.c, m_affine.e + m_affine.f);

    QPainterPath path;
    path.moveTo(pO);
    path.lineTo(pX);
    path.lineTo(pY);
    path.closeSubpath();

    path.addEllipse(pO, 12.0, 12.0);
    path.addEllipse(pX, 10.0, 10.0);
    path.addEllipse(pY, 10.0, 10.0);

    return path;
}

XformGraphicItem::HandleType XformGraphicItem::hit_test_handle(const QPointF& pos) const {
    QPointF pO = math_to_scene(m_affine.c, m_affine.f);
    QPointF pX = math_to_scene(m_affine.a + m_affine.c, m_affine.d + m_affine.f);
    QPointF pY = math_to_scene(m_affine.b + m_affine.c, m_affine.e + m_affine.f);

    constexpr qreal HANDLE_RADIUS_SQ = 144.0; // 12px radius

    auto dist_sq = [](const QPointF& a, const QPointF& b) {
        qreal dx = a.x() - b.x();
        qreal dy = a.y() - b.y();
        return dx * dx + dy * dy;
    };

    if (dist_sq(pos, pO) <= HANDLE_RADIUS_SQ) return Handle_Origin;
    if (dist_sq(pos, pX) <= HANDLE_RADIUS_SQ) return Handle_X;
    if (dist_sq(pos, pY) <= HANDLE_RADIUS_SQ) return Handle_Y;

    // Check triangle body
    QPolygonF poly;
    poly << pO << pX << pY;
    if (poly.containsPoint(pos, Qt::OddEvenFill)) {
        return Handle_Body;
    }

    return Handle_None;
}

void XformGraphicItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /*widget*/) {
    painter->setRenderHint(QPainter::Antialiasing, true);

    QPointF pO = math_to_scene(m_affine.c, m_affine.f);
    QPointF pX = math_to_scene(m_affine.a + m_affine.c, m_affine.d + m_affine.f);
    QPointF pY = math_to_scene(m_affine.b + m_affine.c, m_affine.e + m_affine.f);

    // 1. Draw semi-transparent triangle body
    QPolygonF poly;
    poly << pO << pX << pY;

    QColor fill_col = (m_mode == Mode_PostAffine) ? QColor(220, 80, 240) : m_color;
    fill_col.setAlpha(m_is_selected ? 80 : 45);
    painter->setBrush(fill_col);

    QColor border_col = m_is_selected ? QColor(255, 255, 255) : fill_col;
    QPen border_pen(border_col, m_is_selected ? 2.2 : 1.4);
    if (m_mode == Mode_PostAffine) {
        border_pen.setStyle(Qt::DashLine);
    }
    painter->setPen(border_pen);
    painter->drawPolygon(poly);

    // 2. Right-angle visual indicator at Origin O if orthogonal
    double lx = std::hypot(m_affine.a, m_affine.d);
    double ly = std::hypot(m_affine.b, m_affine.e);
    if (lx > 1e-6 && ly > 1e-6) {
        double dot = (m_affine.a * m_affine.b + m_affine.d * m_affine.e) / (lx * ly);
        if (std::abs(dot) < 0.08) { // approximately 90 degrees
            QPointF dir_ox = (pX - pO) / std::hypot(pX.x() - pO.x(), pX.y() - pO.y());
            QPointF dir_oy = (pY - pO) / std::hypot(pY.x() - pO.x(), pY.y() - pO.y());
            constexpr qreal BOX_SIZE = 9.0;
            QPointF corner1 = pO + dir_ox * BOX_SIZE;
            QPointF corner2 = corner1 + dir_oy * BOX_SIZE;
            QPointF corner3 = pO + dir_oy * BOX_SIZE;

            QPainterPath square_path;
            square_path.moveTo(corner1);
            square_path.lineTo(corner2);
            square_path.lineTo(corner3);

            painter->setPen(QPen(QColor(220, 230, 255, 180), 1.2));
            painter->drawPath(square_path);
        }
    }

    // 3. Draw X-axis leg (O -> X)
    QPen x_pen(QColor(240, 80, 80), m_is_selected ? 2.5 : 1.8);
    painter->setPen(x_pen);
    painter->drawLine(pO, pX);

    // 4. Draw Y-axis leg (O -> Y)
    QPen y_pen(QColor(80, 220, 100), m_is_selected ? 2.5 : 1.8);
    painter->setPen(y_pen);
    painter->drawLine(pO, pY);

    // 5. Draw hypotenuse (X -> Y) dashed
    QPen hyp_pen(border_col.darker(120), 1.0, Qt::DashLine);
    painter->setPen(hyp_pen);
    painter->drawLine(pX, pY);

    // 6. Draw Handles
    // Origin Handle (O')
    painter->setPen(QPen(Qt::white, 1.5));
    painter->setBrush(m_is_selected ? QColor(255, 215, 0) : QColor(220, 220, 230));
    painter->drawEllipse(pO, 6.0, 6.0);

    // X-Tip Handle (X')
    painter->setPen(QPen(Qt::white, 1.2));
    painter->setBrush(QColor(240, 80, 80));
    painter->drawEllipse(pX, 5.0, 5.0);

    // Y-Tip Handle (Y')
    painter->setPen(QPen(Qt::white, 1.2));
    painter->setBrush(QColor(80, 220, 100));
    painter->drawEllipse(pY, 5.0, 5.0);

    // 7. Centroid Index Badge
    QPointF centroid = (pO + pX + pY) / 3.0;
    QRectF badge_rect(centroid.x() - 13.0, centroid.y() - 9.0, 26.0, 18.0);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(20, 24, 32, 210));
    painter->drawRoundedRect(badge_rect, 4.0, 4.0);

    painter->setPen(m_is_selected ? Qt::yellow : (m_mode == Mode_PostAffine ? QColor(255, 140, 255) : Qt::white));
    QFont font = painter->font();
    font.setPointSize(8);
    font.setBold(true);
    painter->setFont(font);

    QString badge_text = (m_mode == Mode_PostAffine ? QStringLiteral("P%1") : QStringLiteral("%1")).arg(m_index + 1);
    painter->drawText(badge_rect, Qt::AlignCenter, badge_text);
}

void XformGraphicItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_active_handle = hit_test_handle(event->pos());
        if (m_active_handle != Handle_None) {
            m_drag_start_pos = event->pos();
            m_drag_start_affine = m_affine;

            double lx = std::hypot(m_affine.a, m_affine.d);
            double ly = std::hypot(m_affine.b, m_affine.e);
            m_drag_start_ratio = (lx > 1e-9) ? (ly / lx) : 1.0;

            double det = m_affine.a * m_affine.e - m_affine.b * m_affine.d;
            m_drag_start_det_sign = (det >= 0.0) ? 1.0 : -1.0;

            event->accept();
            return;
        }
    }
    QGraphicsItem::mousePressEvent(event);
}

void XformGraphicItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    if (m_active_handle == Handle_None) {
        QGraphicsItem::mouseMoveEvent(event);
        return;
    }

    QPointF current_pos = event->pos();
    double cur_mx = 0.0, cur_my = 0.0;
    scene_to_math(current_pos, cur_mx, cur_my);

    double start_mx = 0.0, start_my = 0.0;
    scene_to_math(m_drag_start_pos, start_mx, start_my);

    bool allow_skew = m_skew_mode_enabled ||
                      (event->modifiers() & (Qt::ShiftModifier | Qt::AltModifier | Qt::ControlModifier));

    Core::Affine2D new_aff = m_drag_start_affine;

    switch (m_active_handle) {
        case Handle_Origin: {
            // Translate origin: c, f change, a, b, d, e stay fixed
            new_aff.c = snap_value(cur_mx);
            new_aff.f = snap_value(cur_my);
            break;
        }
        case Handle_X: {
            // Move X tip: X' = (a + c, d + f) -> a = X'_x - c, d = X'_y - f
            double sx = snap_value(cur_mx);
            double sy = snap_value(cur_my);
            new_aff.a = sx - new_aff.c;
            new_aff.d = sy - new_aff.f;

            if (!allow_skew) {
                // Orthogonal constraint: OY must stay perpendicular to OX
                // OY = sign * ratio * (-d, a)
                double r = m_drag_start_ratio;
                double s = m_drag_start_det_sign;
                new_aff.b = -s * r * new_aff.d;
                new_aff.e =  s * r * new_aff.a;
            }
            break;
        }
        case Handle_Y: {
            if (!allow_skew) {
                // Orthogonal constraint: adjust Y scale along the perpendicular to OX without skewing
                double lx = std::hypot(new_aff.a, new_aff.d);
                if (lx > 1e-6) {
                    double s = m_drag_start_det_sign;
                    // Unit perpendicular to OX
                    double nx = -s * new_aff.d / lx;
                    double ny =  s * new_aff.a / lx;

                    // Displacement vector from O to mouse point
                    double vx = cur_mx - new_aff.c;
                    double vy = cur_my - new_aff.f;

                    // Projected length along perpendicular
                    double ly_new = snap_value(vx * nx + vy * ny);
                    if (std::abs(ly_new) < 1e-4) {
                        ly_new = (ly_new >= 0.0 ? 1e-4 : -1e-4);
                    }

                    new_aff.b = -s * (ly_new / lx) * new_aff.d;
                    new_aff.e =  s * (ly_new / lx) * new_aff.a;
                }
            } else {
                // Free shear / skew mode
                double sx = snap_value(cur_mx);
                double sy = snap_value(cur_my);
                new_aff.b = sx - new_aff.c;
                new_aff.e = sy - new_aff.f;
            }
            break;
        }
        case Handle_Body: {
            // Translate entire triangle
            double delta_x = snap_value(cur_mx - start_mx);
            double delta_y = snap_value(cur_my - start_my);
            new_aff.c = m_drag_start_affine.c + delta_x;
            new_aff.f = m_drag_start_affine.f + delta_y;
            break;
        }
        default:
            break;
    }

    set_affine(new_aff);

    if (m_change_callback) {
        m_change_callback(m_index, m_affine, false);
    }

    event->accept();
}

void XformGraphicItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    if (m_active_handle != Handle_None) {
        m_active_handle = Handle_None;
        if (m_change_callback) {
            m_change_callback(m_index, m_affine, true);
        }
        event->accept();
        return;
    }
    QGraphicsItem::mouseReleaseEvent(event);
}

void XformGraphicItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event) {
    HandleType h = hit_test_handle(event->pos());
    switch (h) {
        case Handle_Origin:
        case Handle_X:
        case Handle_Y:
            setCursor(Qt::CrossCursor);
            break;
        case Handle_Body:
            setCursor(Qt::SizeAllCursor);
            break;
        default:
            setCursor(Qt::ArrowCursor);
            break;
    }
    QGraphicsItem::hoverMoveEvent(event);
}

} // namespace ApoNeo::UI
