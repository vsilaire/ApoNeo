#include "TransformEditorWidget.hpp"
#include <QAction>
#include <QActionGroup>
#include <QGraphicsSceneMouseEvent>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <cmath>

namespace ApoNeo::UI {

// ============================================================================
// TransformGraphicsView Implementation
// ============================================================================

TransformGraphicsView::TransformGraphicsView(QGraphicsScene* scene, QWidget* parent)
    : QGraphicsView(scene, parent) {
    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::SmoothPixmapTransform, true);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setMouseTracking(true);
    setStyleSheet(QStringLiteral("background-color: #161920; border: none;"));

    // Center view at origin
    setSceneRect(-5000, -5000, 10000, 10000);
    centerOn(0, 0);
}

void TransformGraphicsView::reset_view() {
    resetTransform();
    centerOn(0, 0);
}

void TransformGraphicsView::drawBackground(QPainter* painter, const QRectF& rect) {
    painter->fillRect(rect, QColor(20, 23, 29));

    constexpr double UNIT_PX = XformGraphicItem::SCENE_SCALE; // 100 px = 1 unit
    constexpr double MINOR_PX = UNIT_PX / 10.0;               // 10 px = 0.1 unit

    qreal left = std::floor(rect.left() / MINOR_PX) * MINOR_PX;
    qreal top = std::floor(rect.top() / MINOR_PX) * MINOR_PX;

    // 1. Minor grid lines (0.1 units)
    QPen minor_pen(QColor(30, 35, 45), 0.6);
    painter->setPen(minor_pen);
    for (qreal x = left; x <= rect.right(); x += MINOR_PX) {
        painter->drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
    }
    for (qreal y = top; y <= rect.bottom(); y += MINOR_PX) {
        painter->drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
    }

    // 2. Major grid lines (1.0 units)
    qreal major_left = std::floor(rect.left() / UNIT_PX) * UNIT_PX;
    qreal major_top = std::floor(rect.top() / UNIT_PX) * UNIT_PX;

    QPen major_pen(QColor(48, 56, 72), 1.0);
    painter->setPen(major_pen);
    for (qreal x = major_left; x <= rect.right(); x += UNIT_PX) {
        painter->drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
    }
    for (qreal y = major_top; y <= rect.bottom(); y += UNIT_PX) {
        painter->drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
    }

    // 3. Cartesian Axes: X-axis (Red) and Y-axis (Green)
    QPen x_axis_pen(QColor(200, 70, 70), 1.6);
    painter->setPen(x_axis_pen);
    painter->drawLine(QPointF(rect.left(), 0), QPointF(rect.right(), 0));

    QPen y_axis_pen(QColor(70, 180, 85), 1.6);
    painter->setPen(y_axis_pen);
    painter->drawLine(QPointF(0, rect.top()), QPointF(0, rect.bottom()));

    // 4. Axis Labels
    painter->setPen(QColor(110, 125, 150));
    QFont font = painter->font();
    font.setPointSize(8);
    painter->setFont(font);

    for (qreal x = major_left; x <= rect.right(); x += UNIT_PX) {
        if (std::abs(x) < 1.0) continue; // skip origin
        int val = static_cast<int>(std::round(x / UNIT_PX));
        painter->drawText(QPointF(x + 3, 14), QString::number(val));
    }
    for (qreal y = major_top; y <= rect.bottom(); y += UNIT_PX) {
        if (std::abs(y) < 1.0) continue;
        int val = static_cast<int>(std::round(-y / UNIT_PX));
        painter->drawText(QPointF(4, y - 3), QString::number(val));
    }
}

void TransformGraphicsView::wheelEvent(QWheelEvent* event) {
    const double num_degrees = event->angleDelta().y() / 8.0;
    const double num_steps = num_degrees / 15.0;
    const double factor = std::pow(1.15, num_steps);
    scale(factor, factor);
    event->accept();
}

void TransformGraphicsView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton || event->button() == Qt::RightButton) {
        m_panning = true;
        m_last_mouse_pos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QGraphicsView::mousePressEvent(event);
}

void TransformGraphicsView::mouseMoveEvent(QMouseEvent* event) {
    if (m_panning) {
        QPoint delta = event->pos() - m_last_mouse_pos;
        m_last_mouse_pos = event->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        event->accept();
        return;
    }
    QGraphicsView::mouseMoveEvent(event);
}

void TransformGraphicsView::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton || event->button() == Qt::RightButton) {
        m_panning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    QGraphicsView::mouseReleaseEvent(event);
}

// ============================================================================
// TransformEditorWidget Implementation
// ============================================================================

TransformEditorWidget::TransformEditorWidget(QWidget* parent)
    : QWidget(parent) {
    setup_ui();
}

void TransformEditorWidget::setup_ui() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Toolbar
    m_toolbar = new QToolBar(tr("Transform Editor Toolbar"), this);
    m_toolbar->setIconSize(QSize(18, 18));
    m_toolbar->setStyleSheet(QStringLiteral("QToolBar { background-color: #1a1d24; border-bottom: 1px solid #2e3440; padding: 2px; }"));

    auto* act_add = m_toolbar->addAction(tr("+ Add Xform"), this, &TransformEditorWidget::on_action_add_xform);
    act_add->setToolTip(tr("Add a new Xform transform"));

    auto* act_del = m_toolbar->addAction(tr("- Delete"), this, &TransformEditorWidget::on_action_delete_xform);
    act_del->setToolTip(tr("Delete active Xform"));

    m_toolbar->addSeparator();

    // Mode Toggle: Pre-Transform vs Post-Transform
    auto* mode_group = new QActionGroup(this);
    m_act_pre = m_toolbar->addAction(tr("Pre (A)"), this, &TransformEditorWidget::on_action_mode_pre);
    m_act_pre->setCheckable(true);
    m_act_pre->setChecked(true);
    m_act_pre->setToolTip(tr("Edit Pre-Affine Transformation Matrix A"));
    mode_group->addAction(m_act_pre);

    m_act_post = m_toolbar->addAction(tr("Post (P)"), this, &TransformEditorWidget::on_action_mode_post);
    m_act_post->setCheckable(true);
    m_act_post->setToolTip(tr("Edit Post-Affine Transformation Matrix P"));
    mode_group->addAction(m_act_post);

    m_chk_enable_post = new QCheckBox(tr("Active Post"), this);
    m_chk_enable_post->setStyleSheet(QStringLiteral("color: #e040fb; padding-left: 4px; padding-right: 4px; font-weight: bold;"));
    m_chk_enable_post->setToolTip(tr("Enable/Disable Post-Affine transform for active Xform"));
    connect(m_chk_enable_post, &QCheckBox::toggled, this, &TransformEditorWidget::on_action_enable_post_toggled);
    m_toolbar->addWidget(m_chk_enable_post);

    m_toolbar->addSeparator();

    auto* act_ident = m_toolbar->addAction(tr("Reset Identity"), this, &TransformEditorWidget::on_action_reset_identity);
    act_ident->setToolTip(tr("Reset selected transform matrix to identity"));

    auto* act_fliph = m_toolbar->addAction(tr("Flip H"), this, &TransformEditorWidget::on_action_flip_h);
    act_fliph->setToolTip(tr("Flip horizontally"));

    auto* act_flipv = m_toolbar->addAction(tr("Flip V"), this, &TransformEditorWidget::on_action_flip_v);
    act_flipv->setToolTip(tr("Flip vertically"));

    auto* act_rot90 = m_toolbar->addAction(tr("Rot 90°"), this, &TransformEditorWidget::on_action_rotate_90);
    act_rot90->setToolTip(tr("Rotate 90 degrees clockwise"));

    m_toolbar->addSeparator();

    auto* chk_snap = new QCheckBox(tr("Snap (0.1)"), this);
    chk_snap->setStyleSheet(QStringLiteral("color: #d8dee9; padding-left: 4px; padding-right: 4px;"));
    connect(chk_snap, &QCheckBox::toggled, this, &TransformEditorWidget::on_action_snap_toggled);
    m_toolbar->addWidget(chk_snap);

    auto* chk_skew = new QCheckBox(tr("Free Skew (Shift)"), this);
    chk_skew->setStyleSheet(QStringLiteral("color: #d8dee9; padding-left: 4px; padding-right: 4px;"));
    connect(chk_skew, &QCheckBox::toggled, this, &TransformEditorWidget::on_action_skew_toggled);
    m_toolbar->addWidget(chk_skew);

    m_toolbar->addSeparator();
    m_toolbar->addAction(tr("Center View"), this, &TransformEditorWidget::on_action_reset_view);

    layout->addWidget(m_toolbar);

    // Scene and View
    m_scene = new QGraphicsScene(this);
    m_view = new TransformGraphicsView(m_scene, this);
    layout->addWidget(m_view);

    // Initial default flame
    set_genome(Core::FlameGenome::preset_swirl_flame());
}

void TransformEditorWidget::set_genome(const Core::FlameGenome& genome) {
    m_genome = genome;
    if (m_selected_index >= m_genome.xforms.size() && !m_genome.xforms.empty()) {
        m_selected_index = 0;
    }
    rebuild_items();
}

void TransformEditorWidget::select_xform(size_t index) {
    if (index >= m_items.size()) return;
    m_selected_index = index;
    for (size_t i = 0; i < m_items.size(); ++i) {
        m_items[i]->set_selected_visual(i == m_selected_index);
        m_items[i]->setZValue(i == m_selected_index ? 100.0 : 10.0 + static_cast<double>(i));
    }
    if (m_selected_index < m_genome.xforms.size() && m_chk_enable_post) {
        m_chk_enable_post->setChecked(m_genome.xforms[m_selected_index].has_post_affine);
    }
    emit xform_selected(m_selected_index);
}

void TransformEditorWidget::rebuild_items() {
    m_scene->clear();
    m_items.clear();

    const size_t num_xforms = m_genome.xforms.size();
    if (num_xforms == 0) return;

    if (m_selected_index < num_xforms && m_chk_enable_post) {
        m_chk_enable_post->setChecked(m_genome.xforms[m_selected_index].has_post_affine);
    }

    const QColor colors[] = {
        QColor(80, 160, 255),
        QColor(255, 120, 80),
        QColor(100, 230, 140),
        QColor(240, 200, 70),
        QColor(220, 100, 240),
        QColor(80, 230, 230)
    };
    constexpr size_t NUM_COLORS = sizeof(colors) / sizeof(colors[0]);

    for (size_t i = 0; i < num_xforms; ++i) {
        auto* item = new XformGraphicItem(i);
        item->set_transform_mode(m_is_post_mode ? XformGraphicItem::Mode_PostAffine : XformGraphicItem::Mode_PreAffine);
        item->set_affine(m_is_post_mode ? m_genome.xforms[i].post_affine : m_genome.xforms[i].affine);
        item->set_color(colors[i % NUM_COLORS]);
        item->set_snap_grid(m_snap_enabled, 0.1);
        item->set_skew_mode(m_skew_enabled);
        item->set_selected_visual(i == m_selected_index);

        item->set_change_callback([this](size_t idx, const Core::Affine2D& aff, bool finished) {
            if (idx < m_genome.xforms.size()) {
                if (m_is_post_mode) {
                    m_genome.xforms[idx].post_affine = aff;
                    m_genome.xforms[idx].has_post_affine = true;
                    if (m_chk_enable_post) m_chk_enable_post->setChecked(true);
                    emit post_xform_modified(idx, aff, finished);
                } else {
                    m_genome.xforms[idx].affine = aff;
                    emit xform_modified(idx, aff, finished);
                }
            }
        });

        m_scene->addItem(item);
        m_items.push_back(item);
    }
}

void TransformEditorWidget::on_action_mode_pre() {
    m_is_post_mode = false;
    rebuild_items();
}

void TransformEditorWidget::on_action_mode_post() {
    m_is_post_mode = true;
    rebuild_items();
}

void TransformEditorWidget::on_action_enable_post_toggled(bool checked) {
    if (m_selected_index < m_genome.xforms.size()) {
        m_genome.xforms[m_selected_index].has_post_affine = checked;
        emit post_xform_modified(m_selected_index, m_genome.xforms[m_selected_index].post_affine, true);
    }
}

void TransformEditorWidget::on_action_add_xform() {
    Core::Xform new_xf;
    new_xf.affine = Core::Affine2D::scaling(0.5, 0.5);
    new_xf.set_variation("linear", 1.0);
    new_xf.weight = 1.0;
    new_xf.color_index = static_cast<double>(m_genome.xforms.size() % 10) / 10.0;

    m_genome.xforms.push_back(std::move(new_xf));
    m_selected_index = m_genome.xforms.size() - 1;
    rebuild_items();
    emit xform_added();
}

void TransformEditorWidget::on_action_delete_xform() {
    if (m_genome.xforms.size() <= 1) {
        return;
    }
    size_t del_idx = m_selected_index;
    m_genome.xforms.erase(m_genome.xforms.begin() + del_idx);
    if (m_selected_index >= m_genome.xforms.size()) {
        m_selected_index = m_genome.xforms.size() - 1;
    }
    rebuild_items();
    emit xform_deleted(del_idx);
}

void TransformEditorWidget::on_action_reset_identity() {
    if (m_selected_index < m_genome.xforms.size()) {
        if (m_is_post_mode) {
            m_genome.xforms[m_selected_index].post_affine = Core::Affine2D::identity();
            if (m_selected_index < m_items.size()) {
                m_items[m_selected_index]->set_affine(Core::Affine2D::identity());
            }
            emit post_xform_modified(m_selected_index, Core::Affine2D::identity(), true);
        } else {
            m_genome.xforms[m_selected_index].affine = Core::Affine2D::identity();
            if (m_selected_index < m_items.size()) {
                m_items[m_selected_index]->set_affine(Core::Affine2D::identity());
            }
            emit xform_modified(m_selected_index, Core::Affine2D::identity(), true);
        }
    }
}

void TransformEditorWidget::on_action_flip_h() {
    if (m_selected_index < m_genome.xforms.size()) {
        auto& aff = m_is_post_mode ? m_genome.xforms[m_selected_index].post_affine : m_genome.xforms[m_selected_index].affine;
        aff.a = -aff.a;
        aff.b = -aff.b;
        if (m_selected_index < m_items.size()) {
            m_items[m_selected_index]->set_affine(aff);
        }
        if (m_is_post_mode) {
            emit post_xform_modified(m_selected_index, aff, true);
        } else {
            emit xform_modified(m_selected_index, aff, true);
        }
    }
}

void TransformEditorWidget::on_action_flip_v() {
    if (m_selected_index < m_genome.xforms.size()) {
        auto& aff = m_is_post_mode ? m_genome.xforms[m_selected_index].post_affine : m_genome.xforms[m_selected_index].affine;
        aff.d = -aff.d;
        aff.e = -aff.e;
        if (m_selected_index < m_items.size()) {
            m_items[m_selected_index]->set_affine(aff);
        }
        if (m_is_post_mode) {
            emit post_xform_modified(m_selected_index, aff, true);
        } else {
            emit xform_modified(m_selected_index, aff, true);
        }
    }
}

void TransformEditorWidget::on_action_rotate_90() {
    if (m_selected_index < m_genome.xforms.size()) {
        auto& aff = m_is_post_mode ? m_genome.xforms[m_selected_index].post_affine : m_genome.xforms[m_selected_index].affine;
        double na = -aff.d;
        double nb = -aff.e;
        double nd = aff.a;
        double ne = aff.b;
        aff.a = na; aff.b = nb;
        aff.d = nd; aff.e = ne;
        if (m_selected_index < m_items.size()) {
            m_items[m_selected_index]->set_affine(aff);
        }
        if (m_is_post_mode) {
            emit post_xform_modified(m_selected_index, aff, true);
        } else {
            emit xform_modified(m_selected_index, aff, true);
        }
    }
}

void TransformEditorWidget::on_action_snap_toggled(bool checked) {
    m_snap_enabled = checked;
    for (auto* item : m_items) {
        item->set_snap_grid(m_snap_enabled, 0.1);
    }
}

void TransformEditorWidget::on_action_skew_toggled(bool checked) {
    m_skew_enabled = checked;
    for (auto* item : m_items) {
        item->set_skew_mode(m_skew_enabled);
    }
}

void TransformEditorWidget::on_action_reset_view() {
    m_view->reset_view();
}

} // namespace ApoNeo::UI
