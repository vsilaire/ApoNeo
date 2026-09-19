#pragma once

#include "XformGraphicItem.hpp"
#include "core/FlameGenome.hpp"
#include <QActionGroup>
#include <QCheckBox>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QToolBar>
#include <QWidget>
#include <vector>

namespace ApoNeo::UI {

class TransformGraphicsView : public QGraphicsView {
    Q_OBJECT

public:
    explicit TransformGraphicsView(QGraphicsScene* scene, QWidget* parent = nullptr);

    void reset_view();

protected:
    void drawBackground(QPainter* painter, const QRectF& rect) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    bool m_panning = false;
    QPoint m_last_mouse_pos;
};

class TransformEditorWidget : public QWidget {
    Q_OBJECT

public:
    explicit TransformEditorWidget(QWidget* parent = nullptr);
    ~TransformEditorWidget() override = default;

    void set_genome(const Core::FlameGenome& genome);
    void select_xform(size_t index);
    size_t selected_xform_index() const noexcept { return m_selected_index; }
    bool is_post_transform_mode() const noexcept { return m_is_post_mode; }

signals:
    void xform_modified(size_t index, const Core::Affine2D& affine, bool finished);
    void post_xform_modified(size_t index, const Core::Affine2D& post_affine, bool finished);
    void xform_selected(size_t index);
    void xform_added();
    void xform_deleted(size_t index);

private slots:
    void on_action_add_xform();
    void on_action_delete_xform();
    void on_action_reset_identity();
    void on_action_flip_h();
    void on_action_flip_v();
    void on_action_rotate_90();
    void on_action_snap_toggled(bool checked);
    void on_action_skew_toggled(bool checked);
    void on_action_mode_pre();
    void on_action_mode_post();
    void on_action_enable_post_toggled(bool checked);
    void on_action_reset_view();

private:
    void setup_ui();
    void rebuild_items();

    Core::FlameGenome m_genome;
    size_t m_selected_index = 0;
    bool m_snap_enabled = false;
    bool m_skew_enabled = false;
    bool m_is_post_mode = false;

    QGraphicsScene* m_scene = nullptr;
    TransformGraphicsView* m_view = nullptr;
    QToolBar* m_toolbar = nullptr;

    QAction* m_act_pre = nullptr;
    QAction* m_act_post = nullptr;
    QCheckBox* m_chk_enable_post = nullptr;

    std::vector<XformGraphicItem*> m_items;
};

} // namespace ApoNeo::UI
