#pragma once

#include "core/FlameGenome.hpp"
#include "core/Variation.hpp"
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QSlider>
#include <QTableWidget>
#include <QWidget>
#include <map>
#include <string>
#include <vector>

namespace ApoNeo::UI {

class VariationInspectorWidget : public QWidget {
    Q_OBJECT

public:
    explicit VariationInspectorWidget(QWidget* parent = nullptr);
    ~VariationInspectorWidget() override = default;

    void set_genome(const Core::FlameGenome& genome);
    void set_active_xform_index(size_t index);
    size_t active_xform_index() const noexcept { return m_active_xform_idx; }

signals:
    void variation_modified(size_t xform_idx, const std::string& name, double weight);
    void param_modified(size_t xform_idx, const std::string& name, double val);
    void variations_cleared(size_t xform_idx);

private slots:
    void on_filter_text_changed(const QString& text);
    void on_category_changed(int index);
    void on_table_selection_changed();
    void on_reset_linear_clicked();
    void on_zero_all_clicked();

private:
    void setup_ui();
    void populate_table();
    void filter_table();
    void rebuild_parameter_panel(const std::string& var_name);
    void update_row_for_variation(int row, const std::string& var_name);

    Core::FlameGenome m_genome;
    size_t m_active_xform_idx = 0;
    bool m_updating_ui = false;

    QLineEdit* m_search_edit = nullptr;
    QComboBox* m_category_combo = nullptr;
    QTableWidget* m_table = nullptr;

    QGroupBox* m_param_group = nullptr;
    QFormLayout* m_param_layout = nullptr;
    std::string m_selected_variation_name;

    struct RowMap {
        std::string var_name;
        std::string category;
    };
    std::vector<RowMap> m_row_data;
};

} // namespace ApoNeo::UI
