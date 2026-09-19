#include "FlameCanvasWidget.hpp"
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>
#include <algorithm>

namespace ApoNeo::UI {

FlameCanvasWidget::FlameCanvasWidget(QWidget* parent)
    : QWidget(parent) {
    setAttribute(Qt::WA_OpaquePaintEvent);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(400, 300);
}

void FlameCanvasWidget::set_rendered_image(const QImage& img) {
    m_image = img;
    update();
}

void FlameCanvasWidget::reset_view() {
    m_zoom = 1.0;
    m_pan = QPointF(0.0, 0.0);
    emit view_changed(m_zoom, m_pan);
    update();
}

void FlameCanvasWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // Dark background with subtle grid
    painter.fillRect(rect(), QColor(18, 20, 24));

    if (m_image.isNull()) {
        painter.setPen(QColor(120, 130, 150));
        painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("No image rendered. Click 'Render' to start."));
        return;
    }

    // Centered image drawing with pan & zoom
    const double target_w = m_image.width() * m_zoom;
    const double target_h = m_image.height() * m_zoom;
    const double origin_x = (width() - target_w) * 0.5 + m_pan.x();
    const double origin_y = (height() - target_h) * 0.5 + m_pan.y();

    QRectF target_rect(origin_x, origin_y, target_w, target_h);
    painter.drawImage(target_rect, m_image);

    // Draw thin canvas border
    painter.setPen(QPen(QColor(60, 70, 85), 1.0));
    painter.drawRect(target_rect);
}

void FlameCanvasWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton) {
        m_dragging = true;
        m_last_mouse_pos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void FlameCanvasWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_dragging) {
        QPoint delta = event->pos() - m_last_mouse_pos;
        m_last_mouse_pos = event->pos();
        m_pan += QPointF(delta.x(), delta.y());
        emit view_changed(m_zoom, m_pan);
        update();
    }

    // World coordinate estimation
    if (!m_image.isNull()) {
        const double target_w = m_image.width() * m_zoom;
        const double target_h = m_image.height() * m_zoom;
        const double origin_x = (width() - target_w) * 0.5 + m_pan.x();
        const double origin_y = (height() - target_h) * 0.5 + m_pan.y();

        double img_x = (event->pos().x() - origin_x) / m_zoom;
        double img_y = (event->pos().y() - origin_y) / m_zoom;
        emit coordinate_hovered(img_x, img_y);
    }
}

void FlameCanvasWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton) {
        m_dragging = false;
        setCursor(Qt::ArrowCursor);
    }
}

void FlameCanvasWidget::wheelEvent(QWheelEvent* event) {
    const double num_degrees = event->angleDelta().y() / 8.0;
    const double num_steps = num_degrees / 15.0;
    const double factor = std::pow(1.15, num_steps);

    // Zoom centered around mouse cursor
    QPointF mouse_pos = event->position();
    QPointF center_before = (mouse_pos - m_pan - QPointF(width() * 0.5, height() * 0.5)) / m_zoom;

    m_zoom = std::clamp(m_zoom * factor, 0.05, 50.0);

    QPointF center_after = center_before * m_zoom;
    m_pan = mouse_pos - center_after - QPointF(width() * 0.5, height() * 0.5);

    emit view_changed(m_zoom, m_pan);
    update();
    event->accept();
}

void FlameCanvasWidget::resizeEvent(QResizeEvent* /*event*/) {
    update();
}

} // namespace ApoNeo::UI
