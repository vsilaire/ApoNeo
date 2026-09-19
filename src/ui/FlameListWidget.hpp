#pragma once

#include "core/FlameGenome.hpp"
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <vector>

namespace ApoNeo::UI {

class FlameListWidget : public QWidget {
    Q_OBJECT

public:
    explicit FlameListWidget(QWidget* parent = nullptr);
    ~FlameListWidget() override = default;

    /// @brief Set the batch of flames and render their thumbnail icons
    void set_flames(const std::vector<Core::FlameGenome>& flames);

    /// @brief Add a single flame to the list
    void add_flame(const Core::FlameGenome& flame);

    /// @brief Select a flame by index
    void select_flame(int index);

    /// @brief Get current list of genomes
    const std::vector<Core::FlameGenome>& flames() const { return m_flames; }

signals:
    void flame_selected(const Core::FlameGenome& genome);
    void request_new_batch();

private slots:
    void on_item_clicked(QListWidgetItem* item);
    void on_generate_batch_clicked();

private:
    QImage render_thumbnail(const Core::FlameGenome& genome, int w = 120, int h = 90);

    QVBoxLayout* m_layout = nullptr;
    QPushButton* m_btn_generate = nullptr;
    QListWidget* m_list_widget = nullptr;
    std::vector<Core::FlameGenome> m_flames;
};

} // namespace ApoNeo::UI
