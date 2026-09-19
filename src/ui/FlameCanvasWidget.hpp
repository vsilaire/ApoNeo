#pragma once

#include <QImage>
#include <QPointF>
#include <QWidget>

namespace ApoNeo::UI {

class FlameCanvasWidget : public QWidget {
    Q_OBJECT

public:
    explicit FlameCanvasWidget(QWidget* parent = nullptr);

    void set_rendered_image(const QImage& img);
    const QImage& rendered_image() const noexcept { return m_image; }

    void reset_view();
    double view_zoom() const noexcept { return m_zoom; }
    QPointF view_pan() const noexcept { return m_pan; }

signals:
    void view_changed(double zoom, QPointF pan);
    void coordinate_hovered(double world_x, double world_y);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    QImage m_image;
    double m_zoom = 1.0;
    QPointF m_pan = {0.0, 0.0};

    bool m_dragging = false;
    QPoint m_last_mouse_pos;
};

} // namespace ApoNeo::UI
