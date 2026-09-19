#include "VariationInspectorWidget.hpp"
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace ApoNeo::UI {

VariationInspectorWidget::VariationInspectorWidget(QWidget* parent)
    : QWidget(parent) {
    setup_ui();
}

void VariationInspectorWidget::setup_ui() {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(6, 6, 6, 6);
    main_layout->setSpacing(8);

    // 1. Search & Filter Bar
    auto* filter_layout = new QHBoxLayout();
    filter_layout->setSpacing(6);

    m_search_edit = new QLineEdit(this);
    m_search_edit->setPlaceholderText(tr("Search variations... (e.g. julian, waves)"));
    m_search_edit->setClearButtonEnabled(true);
    connect(m_search_edit, &QLineEdit::textChanged, this, &VariationInspectorWidget::on_filter_text_changed);
    filter_layout->addWidget(m_search_edit, 2);

    m_category_combo = new QComboBox(this);
    m_category_combo->addItem(tr("All Categories"));
    m_category_combo->addItem(tr("Classic"));
    m_category_combo->addItem(tr("Complex"));
    m_category_combo->addItem(tr("Periodic"));
    m_category_combo->addItem(tr("Radial"));
    m_category_combo->addItem(tr("3D"));
    m_category_combo->addItem(tr("Optical"));
    m_category_combo->addItem(tr("Fluid"));
    m_category_combo->addItem(tr("Blur/Noise"));
    connect(m_category_combo, &QComboBox::currentIndexChanged, this, &VariationInspectorWidget::on_category_changed);
    filter_layout->addWidget(m_category_combo, 1);

    main_layout->addLayout(filter_layout);

    // Quick action buttons
    auto* btn_layout = new QHBoxLayout();
    btn_layout->setSpacing(6);

    auto* btn_linear = new QPushButton(tr("Set Linear (1.0)"), this);
    btn_linear->setStyleSheet(QStringLiteral("background-color: #2e3440; color: #88c0d0; padding: 4px; font-size: 11px;"));
    connect(btn_linear, &QPushButton::clicked, this, &VariationInspectorWidget::on_reset_linear_clicked);
    btn_layout->addWidget(btn_linear);

    auto* btn_zero = new QPushButton(tr("Zero All"), this);
    btn_zero->setStyleSheet(QStringLiteral("background-color: #2e3440; color: #d08770; padding: 4px; font-size: 11px;"));
    connect(btn_zero, &QPushButton::clicked, this, &VariationInspectorWidget::on_zero_all_clicked);
    btn_layout->addWidget(btn_zero);

    main_layout->addLayout(btn_layout);

    // 2. Variations Table
    m_table = new QTableWidget(this);
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels({tr("Variation"), tr("Weight"), tr("Category")});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    m_table->setStyleSheet(QStringLiteral(R"(
        QTableWidget {
            background-color: #1a1d24;
            alternate-background-color: #20242d;
            gridline-color: #2e3440;
            color: #eceff4;
            border: 1px solid #2e3440;
            border-radius: 4px;
        }
        QHeaderView::section {
            background-color: #242933;
            color: #88c0d0;
            padding: 4px;
            font-weight: bold;
            border: 1px solid #2e3440;
        }
    )"));

    connect(m_table, &QTableWidget::itemSelectionChanged, this, &VariationInspectorWidget::on_table_selection_changed);
    main_layout->addWidget(m_table, 3);

    // 3. Dynamic Parameter Box (Variables)
    m_param_group = new QGroupBox(tr("Variation Variables & Parameters"), this);
    m_param_layout = new QFormLayout(m_param_group);
    m_param_layout->setSpacing(6);
    m_param_layout->setContentsMargins(8, 12, 8, 8);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setWidget(m_param_group);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(QStringLiteral("background-color: transparent;"));
    scroll->setMaximumHeight(160);

    main_layout->addWidget(scroll, 1);

    // Populate initial table
    populate_table();
}

void VariationInspectorWidget::populate_table() {
    m_updating_ui = true;
    m_table->setRowCount(0);
    m_row_data.clear();

    const auto& vars = Core::VariationRegistry::instance().all_variations();
    m_table->setRowCount(static_cast<int>(vars.size()));

    for (int r = 0; r < static_cast<int>(vars.size()); ++r) {
        const auto& v = vars[r];
        m_row_data.push_back({v.name, v.category});

        // 1. Name Item
        auto* name_item = new QTableWidgetItem(QString::fromStdString(v.name));
        name_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        if (v.has_params()) {
            name_item->setText(QString::fromStdString(v.name) + QStringLiteral(" *"));
            name_item->setToolTip(tr("Parametric variation with custom variables"));
        }
        m_table->setItem(r, 0, name_item);

        // 2. Weight SpinBox
        auto* spin = new QDoubleSpinBox(m_table);
        spin->setRange(-100.0, 100.0);
        spin->setSingleStep(0.1);
        spin->setDecimals(4);
        spin->setValue(0.0);
        spin->setStyleSheet(QStringLiteral("background-color: #242933; color: #eceff4; border: 1px solid #3b4252; padding: 2px;"));

        connect(spin, &QDoubleSpinBox::valueChanged, this, [this, r, v_name = v.name](double val) {
            if (m_updating_ui) return;
            if (m_active_xform_idx < m_genome.xforms.size()) {
                m_genome.xforms[m_active_xform_idx].set_variation(v_name, val);
                update_row_for_variation(r, v_name);
                emit variation_modified(m_active_xform_idx, v_name, val);
            }
        });

        m_table->setCellWidget(r, 1, spin);

        // 3. Category Item
        auto* cat_item = new QTableWidgetItem(QString::fromStdString(v.category));
        cat_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        cat_item->setForeground(QColor(136, 192, 208));
        m_table->setItem(r, 2, cat_item);
    }

    m_updating_ui = false;
    filter_table();
}

void VariationInspectorWidget::update_row_for_variation(int row, const std::string& var_name) {
    if (m_active_xform_idx >= m_genome.xforms.size()) return;
    const auto& xf = m_genome.xforms[m_active_xform_idx];
    double w = xf.get_variation(var_name);

    auto* name_item = m_table->item(row, 0);
    if (name_item) {
        if (std::abs(w) > 1e-6) {
            name_item->setForeground(QColor(235, 203, 139)); // Gold for active
            QFont f = name_item->font();
            f.setBold(true);
            name_item->setFont(f);
        } else {
            name_item->setForeground(QColor(236, 239, 244));
            QFont f = name_item->font();
            f.setBold(false);
            name_item->setFont(f);
        }
    }
}

void VariationInspectorWidget::set_genome(const Core::FlameGenome& genome) {
    m_genome = genome;
    if (m_active_xform_idx >= m_genome.xforms.size() && !m_genome.xforms.empty()) {
        m_active_xform_idx = 0;
    }
    set_active_xform_index(m_active_xform_idx);
}

void VariationInspectorWidget::set_active_xform_index(size_t index) {
    m_active_xform_idx = index;
    if (m_active_xform_idx >= m_genome.xforms.size()) return;

    m_updating_ui = true;
    const auto& xf = m_genome.xforms[m_active_xform_idx];

    for (int r = 0; r < static_cast<int>(m_row_data.size()); ++r) {
        const auto& v_name = m_row_data[r].var_name;
        double w = xf.get_variation(v_name);

        auto* spin = qobject_cast<QDoubleSpinBox*>(m_table->cellWidget(r, 1));
        if (spin) {
            spin->setValue(w);
        }
        update_row_for_variation(r, v_name);
    }

    m_updating_ui = false;

    // Refresh parameter panel for active selected variation
    if (!m_selected_variation_name.empty()) {
        rebuild_parameter_panel(m_selected_variation_name);
    }
}

void VariationInspectorWidget::on_filter_text_changed(const QString& /*text*/) {
    filter_table();
}

void VariationInspectorWidget::on_category_changed(int /*index*/) {
    filter_table();
}

void VariationInspectorWidget::filter_table() {
    QString filter = m_search_edit->text().trimmed().toLower();
    QString cat = m_category_combo->currentText();

    for (int r = 0; r < static_cast<int>(m_row_data.size()); ++r) {
        const auto& row = m_row_data[r];
        bool matches_text = filter.isEmpty() ||
                            QString::fromStdString(row.var_name).toLower().contains(filter);
        bool matches_cat = (cat == tr("All Categories")) ||
                           (QString::fromStdString(row.category) == cat);

        m_table->setRowHidden(r, !(matches_text && matches_cat));
    }
}

void VariationInspectorWidget::on_table_selection_changed() {
    int row = m_table->currentRow();
    if (row < 0 || row >= static_cast<int>(m_row_data.size())) return;

    m_selected_variation_name = m_row_data[row].var_name;
    rebuild_parameter_panel(m_selected_variation_name);
}

void VariationInspectorWidget::rebuild_parameter_panel(const std::string& var_name) {
    // Clear existing parameter inputs
    while (m_param_layout->count() > 0) {
        auto* item = m_param_layout->takeAt(0);
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }

    const auto* info = Core::VariationRegistry::instance().find_by_name(var_name);
    if (!info || info->params.empty()) {
        m_param_group->setTitle(tr("Variation Variables (None for '%1')").arg(QString::fromStdString(var_name)));
        auto* lbl = new QLabel(tr("No custom variables for this variation."), m_param_group);
        lbl->setStyleSheet(QStringLiteral("color: #707888; font-style: italic;"));
        m_param_layout->addRow(lbl);
        return;
    }

    m_param_group->setTitle(tr("Variables for '%1'").arg(QString::fromStdString(var_name)));

    if (m_active_xform_idx >= m_genome.xforms.size()) return;
    const auto& xf = m_genome.xforms[m_active_xform_idx];

    for (const auto& p_def : info->params) {
        double cur_val = xf.get_param(p_def.name, p_def.default_val);

        auto* spin = new QDoubleSpinBox(m_param_group);
        spin->setRange(p_def.min_val, p_def.max_val);
        spin->setSingleStep(0.1);
        spin->setDecimals(4);
        spin->setValue(cur_val);
        spin->setToolTip(QString::fromStdString(p_def.description));
        spin->setStyleSheet(QStringLiteral("background-color: #242933; color: #eceff4; border: 1px solid #3b4252; padding: 2px;"));

        connect(spin, &QDoubleSpinBox::valueChanged, this, [this, p_name = p_def.name](double val) {
            if (m_updating_ui) return;
            if (m_active_xform_idx < m_genome.xforms.size()) {
                m_genome.xforms[m_active_xform_idx].set_param(p_name, val);
                emit param_modified(m_active_xform_idx, p_name, val);
            }
        });

        m_param_layout->addRow(QString::fromStdString(p_def.name) + ":", spin);
    }
}

void VariationInspectorWidget::on_reset_linear_clicked() {
    if (m_active_xform_idx >= m_genome.xforms.size()) return;
    auto& xf = m_genome.xforms[m_active_xform_idx];
    xf.variation_weights.clear();
    xf.set_variation("linear", 1.0);
    set_active_xform_index(m_active_xform_idx);
    emit variations_cleared(m_active_xform_idx);
}

void VariationInspectorWidget::on_zero_all_clicked() {
    if (m_active_xform_idx >= m_genome.xforms.size()) return;
    auto& xf = m_genome.xforms[m_active_xform_idx];
    xf.variation_weights.clear();
    xf.compile_variations();
    set_active_xform_index(m_active_xform_idx);
    emit variations_cleared(m_active_xform_idx);
}

} // namespace ApoNeo::UI
