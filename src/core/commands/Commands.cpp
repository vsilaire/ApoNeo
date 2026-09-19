#include "Commands.hpp"

namespace ApoNeo::Core::Commands {

// ============================================================================
// ChangeXformMatrixCommand
// ============================================================================
ChangeXformMatrixCommand::ChangeXformMatrixCommand(FlameGenome* genome, size_t xform_idx, bool is_post,
                                                 const Affine2D& old_matrix, const Affine2D& new_matrix,
                                                 UpdateCallback callback, QUndoCommand* parent)
    : QUndoCommand(parent), m_genome(genome), m_xform_idx(xform_idx), m_is_post(is_post),
      m_old_matrix(old_matrix), m_new_matrix(new_matrix), m_callback(callback) {
    setText(is_post ? "Modify Post-Affine Matrix" : "Modify Transform Matrix");
}

void ChangeXformMatrixCommand::undo() {
    if (!m_genome || m_xform_idx >= m_genome->xforms.size()) return;
    if (m_is_post) {
        m_genome->xforms[m_xform_idx].post_affine = m_old_matrix;
    } else {
        m_genome->xforms[m_xform_idx].affine = m_old_matrix;
    }
    m_genome->compile();
    if (m_callback) m_callback();
}

void ChangeXformMatrixCommand::redo() {
    if (!m_genome || m_xform_idx >= m_genome->xforms.size()) return;
    if (m_is_post) {
        m_genome->xforms[m_xform_idx].post_affine = m_new_matrix;
        m_genome->xforms[m_xform_idx].has_post_affine = true;
    } else {
        m_genome->xforms[m_xform_idx].affine = m_new_matrix;
    }
    m_genome->compile();
    if (m_callback) m_callback();
}

int ChangeXformMatrixCommand::id() const {
    return 1000 + (m_is_post ? 100 : 0) + static_cast<int>(m_xform_idx);
}

bool ChangeXformMatrixCommand::mergeWith(const QUndoCommand* other) {
    if (other->id() != id()) return false;
    const auto* cmd = static_cast<const ChangeXformMatrixCommand*>(other);
    m_new_matrix = cmd->m_new_matrix;
    return true;
}

// ============================================================================
// ChangeVariationWeightCommand
// ============================================================================
ChangeVariationWeightCommand::ChangeVariationWeightCommand(FlameGenome* genome, size_t xform_idx,
                                                           const std::string& var_name, double old_weight, double new_weight,
                                                           UpdateCallback callback, QUndoCommand* parent)
    : QUndoCommand(parent), m_genome(genome), m_xform_idx(xform_idx), m_var_name(var_name),
      m_old_weight(old_weight), m_new_weight(new_weight), m_callback(callback) {
    setText(QString("Change Variation %1").arg(QString::fromStdString(var_name)));
}

void ChangeVariationWeightCommand::undo() {
    if (!m_genome || m_xform_idx >= m_genome->xforms.size()) return;
    m_genome->xforms[m_xform_idx].set_variation(m_var_name, m_old_weight);
    m_genome->compile();
    if (m_callback) m_callback();
}

void ChangeVariationWeightCommand::redo() {
    if (!m_genome || m_xform_idx >= m_genome->xforms.size()) return;
    m_genome->xforms[m_xform_idx].set_variation(m_var_name, m_new_weight);
    m_genome->compile();
    if (m_callback) m_callback();
}

int ChangeVariationWeightCommand::id() const {
    return 2000 + static_cast<int>(m_xform_idx);
}

bool ChangeVariationWeightCommand::mergeWith(const QUndoCommand* other) {
    if (other->id() != id()) return false;
    const auto* cmd = static_cast<const ChangeVariationWeightCommand*>(other);
    if (cmd->m_var_name != m_var_name) return false;
    m_new_weight = cmd->m_new_weight;
    return true;
}

// ============================================================================
// ChangeParamValueCommand
// ============================================================================
ChangeParamValueCommand::ChangeParamValueCommand(FlameGenome* genome, size_t xform_idx,
                                                 const std::string& param_name, double old_val, double new_val,
                                                 UpdateCallback callback, QUndoCommand* parent)
    : QUndoCommand(parent), m_genome(genome), m_xform_idx(xform_idx), m_param_name(param_name),
      m_old_val(old_val), m_new_val(new_val), m_callback(callback) {
    setText(QString("Change Parameter %1").arg(QString::fromStdString(param_name)));
}

void ChangeParamValueCommand::undo() {
    if (!m_genome || m_xform_idx >= m_genome->xforms.size()) return;
    m_genome->xforms[m_xform_idx].set_param(m_param_name, m_old_val);
    m_genome->compile();
    if (m_callback) m_callback();
}

void ChangeParamValueCommand::redo() {
    if (!m_genome || m_xform_idx >= m_genome->xforms.size()) return;
    m_genome->xforms[m_xform_idx].set_param(m_param_name, m_new_val);
    m_genome->compile();
    if (m_callback) m_callback();
}

int ChangeParamValueCommand::id() const {
    return 2500 + static_cast<int>(m_xform_idx);
}

bool ChangeParamValueCommand::mergeWith(const QUndoCommand* other) {
    if (other->id() != id()) return false;
    const auto* cmd = static_cast<const ChangeParamValueCommand*>(other);
    if (cmd->m_param_name != m_param_name) return false;
    m_new_val = cmd->m_new_val;
    return true;
}

// ============================================================================
// ChangeCameraCommand
// ============================================================================
ChangeCameraCommand::ChangeCameraCommand(FlameGenome* genome, const CameraState& old_state, const CameraState& new_state,
                                         UpdateCallback callback, QUndoCommand* parent)
    : QUndoCommand(parent), m_genome(genome), m_old_state(old_state), m_new_state(new_state), m_callback(callback) {
    setText("Adjust Camera");
}

void ChangeCameraCommand::undo() {
    if (!m_genome) return;
    m_genome->scale = m_old_state.scale;
    m_genome->center_x = m_old_state.center_x;
    m_genome->center_y = m_old_state.center_y;
    m_genome->rotate = m_old_state.rotate;
    m_genome->zoom = m_old_state.zoom;
    if (m_callback) m_callback();
}

void ChangeCameraCommand::redo() {
    if (!m_genome) return;
    m_genome->scale = m_new_state.scale;
    m_genome->center_x = m_new_state.center_x;
    m_genome->center_y = m_new_state.center_y;
    m_genome->rotate = m_new_state.rotate;
    m_genome->zoom = m_new_state.zoom;
    if (m_callback) m_callback();
}

int ChangeCameraCommand::id() const {
    return 3000;
}

bool ChangeCameraCommand::mergeWith(const QUndoCommand* other) {
    if (other->id() != id()) return false;
    const auto* cmd = static_cast<const ChangeCameraCommand*>(other);
    m_new_state = cmd->m_new_state;
    return true;
}

// ============================================================================
// ChangeToneMapCommand
// ============================================================================
ChangeToneMapCommand::ChangeToneMapCommand(FlameGenome* genome, const ToneMapParams& old_params, const ToneMapParams& new_params,
                                           UpdateCallback callback, QUndoCommand* parent)
    : QUndoCommand(parent), m_genome(genome), m_old_params(old_params), m_new_params(new_params), m_callback(callback) {
    setText("Adjust Tone Mapping");
}

void ChangeToneMapCommand::undo() {
    if (!m_genome) return;
    m_genome->tone_map = m_old_params;
    if (m_callback) m_callback();
}

void ChangeToneMapCommand::redo() {
    if (!m_genome) return;
    m_genome->tone_map = m_new_params;
    if (m_callback) m_callback();
}

int ChangeToneMapCommand::id() const {
    return 4000;
}

bool ChangeToneMapCommand::mergeWith(const QUndoCommand* other) {
    if (other->id() != id()) return false;
    const auto* cmd = static_cast<const ChangeToneMapCommand*>(other);
    m_new_params = cmd->m_new_params;
    return true;
}

// ============================================================================
// ChangePaletteCommand
// ============================================================================
ChangePaletteCommand::ChangePaletteCommand(FlameGenome* genome, const Palette& old_pal, const Palette& new_pal,
                                           UpdateCallback callback, QUndoCommand* parent)
    : QUndoCommand(parent), m_genome(genome), m_old_pal(old_pal), m_new_pal(new_pal), m_callback(callback) {
    setText("Change Palette");
}

void ChangePaletteCommand::undo() {
    if (!m_genome) return;
    m_genome->palette = m_old_pal;
    if (m_callback) m_callback();
}

void ChangePaletteCommand::redo() {
    if (!m_genome) return;
    m_genome->palette = m_new_pal;
    if (m_callback) m_callback();
}

// ============================================================================
// AddXformCommand
// ============================================================================
AddXformCommand::AddXformCommand(FlameGenome* genome, const Xform& xform, size_t index,
                                 UpdateCallback callback, QUndoCommand* parent)
    : QUndoCommand(parent), m_genome(genome), m_xform(xform), m_index(index), m_callback(callback) {
    setText("Add Transform");
}

void AddXformCommand::undo() {
    if (!m_genome || m_index >= m_genome->xforms.size()) return;
    m_genome->xforms.erase(m_genome->xforms.begin() + m_index);
    m_genome->compile();
    if (m_callback) m_callback();
}

void AddXformCommand::redo() {
    if (!m_genome) return;
    if (m_index >= m_genome->xforms.size()) {
        m_genome->xforms.push_back(m_xform);
    } else {
        m_genome->xforms.insert(m_genome->xforms.begin() + m_index, m_xform);
    }
    m_genome->compile();
    if (m_callback) m_callback();
}

// ============================================================================
// DeleteXformCommand
// ============================================================================
DeleteXformCommand::DeleteXformCommand(FlameGenome* genome, size_t index,
                                       UpdateCallback callback, QUndoCommand* parent)
    : QUndoCommand(parent), m_genome(genome), m_index(index), m_callback(callback) {
    if (m_genome && index < m_genome->xforms.size()) {
        m_saved_xform = m_genome->xforms[index];
    }
    setText("Delete Transform");
}

void DeleteXformCommand::undo() {
    if (!m_genome) return;
    if (m_index >= m_genome->xforms.size()) {
        m_genome->xforms.push_back(m_saved_xform);
    } else {
        m_genome->xforms.insert(m_genome->xforms.begin() + m_index, m_saved_xform);
    }
    m_genome->compile();
    if (m_callback) m_callback();
}

void DeleteXformCommand::redo() {
    if (!m_genome || m_index >= m_genome->xforms.size()) return;
    m_saved_xform = m_genome->xforms[m_index];
    m_genome->xforms.erase(m_genome->xforms.begin() + m_index);
    m_genome->compile();
    if (m_callback) m_callback();
}

} // namespace ApoNeo::Core::Commands
