#include "FlameListWidget.hpp"
#include "engine/CPUSIMDEngine.hpp"
#include <QIcon>
#include <QListWidgetItem>
#include <QPixmap>

namespace ApoNeo::UI {

FlameListWidget::FlameListWidget(QWidget* parent)
    : QWidget(parent) {
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(6, 6, 6, 6);
    m_layout->setSpacing(6);

    m_btn_generate = new QPushButton(tr("🎲 Generate New Batch (Ctrl+R)"), this);
    m_btn_generate->setToolTip(tr("Generate 10 new random flames using Apophysis heuristics"));
    m_btn_generate->setStyleSheet(
        "QPushButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #7928ca, stop:1 #ff0080);"
        "  color: #ffffff;"
        "  font-weight: 600;"
        "  padding: 8px 12px;"
        "  border-radius: 6px;"
        "  border: none;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #903fe0, stop:1 #ff2e9a);"
        "}"
        "QPushButton:pressed {"
        "  background: #601ca3;"
        "}"
    );
    connect(m_btn_generate, &QPushButton::clicked, this, &FlameListWidget::on_generate_batch_clicked);
    m_layout->addWidget(m_btn_generate);

    m_list_widget = new QListWidget(this);
    m_list_widget->setIconSize(QSize(120, 90));
    m_list_widget->setViewMode(QListView::ListMode);
    m_list_widget->setMovement(QListView::Static);
    m_list_widget->setSpacing(4);
    m_list_widget->setStyleSheet(
        "QListWidget {"
        "  background: #141416;"
        "  border: 1px solid #232328;"
        "  border-radius: 6px;"
        "  color: #e0e0e0;"
        "}"
        "QListWidget::item {"
        "  padding: 6px;"
        "  border-radius: 4px;"
        "  border: 1px solid transparent;"
        "}"
        "QListWidget::item:hover {"
        "  background: #1e1e24;"
        "  border: 1px solid #3d3d4a;"
        "}"
        "QListWidget::item:selected {"
        "  background: #2a2238;"
        "  border: 1px solid #7928ca;"
        "  color: #ffffff;"
        "}"
    );
    connect(m_list_widget, &QListWidget::itemClicked, this, &FlameListWidget::on_item_clicked);
    m_layout->addWidget(m_list_widget);
}

QImage FlameListWidget::render_thumbnail(const Core::FlameGenome& genome, int w, int h) {
    Core::FlameGenome thumb_genome = genome;
    thumb_genome.width = w;
    thumb_genome.height = h;
    thumb_genome.supersample = 1;
    thumb_genome.quality = 20.0;
    thumb_genome.fuse_iterations = 15;
    thumb_genome.compile();

    Engine::CPUSIMDEngine engine;
    engine.set_genome(thumb_genome);
    engine.start();
    engine.wait_until_done();

    std::vector<uint8_t> rgba;
    int rw = 0, rh = 0;
    engine.get_image_rgba8888(rgba, rw, rh);
    if (rgba.empty() || rw != w || rh != h) {
        QImage empty_img(w, h, QImage::Format_RGBA8888);
        empty_img.fill(Qt::black);
        return empty_img;
    }

    QImage img(rgba.data(), w, h, w * 4, QImage::Format_RGBA8888);
    return img.copy();
}

void FlameListWidget::set_flames(const std::vector<Core::FlameGenome>& flames) {
    m_flames = flames;
    m_list_widget->clear();

    for (size_t i = 0; i < m_flames.size(); ++i) {
        const auto& flame = m_flames[i];
        QImage thumb = render_thumbnail(flame);
        QPixmap pix = QPixmap::fromImage(thumb);

        auto* item = new QListWidgetItem(QIcon(pix), QString::fromStdString(flame.name), m_list_widget);
        item->setData(Qt::UserRole, static_cast<int>(i));
        item->setSizeHint(QSize(160, 100));
        m_list_widget->addItem(item);
    }

    if (!m_flames.empty()) {
        m_list_widget->setCurrentRow(0);
    }
}

void FlameListWidget::add_flame(const Core::FlameGenome& flame) {
    m_flames.push_back(flame);
    QImage thumb = render_thumbnail(flame);
    QPixmap pix = QPixmap::fromImage(thumb);

    auto* item = new QListWidgetItem(QIcon(pix), QString::fromStdString(flame.name), m_list_widget);
    item->setData(Qt::UserRole, static_cast<int>(m_flames.size() - 1));
    item->setSizeHint(QSize(160, 100));
    m_list_widget->addItem(item);
}

void FlameListWidget::select_flame(int index) {
    if (index >= 0 && index < m_list_widget->count()) {
        m_list_widget->setCurrentRow(index);
        emit flame_selected(m_flames[index]);
    }
}

void FlameListWidget::on_item_clicked(QListWidgetItem* item) {
    if (!item) return;
    int idx = item->data(Qt::UserRole).toInt();
    if (idx >= 0 && static_cast<size_t>(idx) < m_flames.size()) {
        emit flame_selected(m_flames[idx]);
    }
}

void FlameListWidget::on_generate_batch_clicked() {
    emit request_new_batch();
}

} // namespace ApoNeo::UI
