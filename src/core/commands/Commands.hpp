#pragma once

#include "core/Affine2D.hpp"
#include "core/FlameGenome.hpp"
#include <QUndoCommand>
#include <functional>
#include <memory>
#include <string>

namespace ApoNeo::Core::Commands {

using UpdateCallback = std::function<void()>;

/// @brief Command for modifying Pre-Affine or Post-Affine matrix of an Xform
class ChangeXformMatrixCommand : public QUndoCommand {
public:
    ChangeXformMatrixCommand(FlameGenome* genome, size_t xform_idx, bool is_post,
                             const Affine2D& old_matrix, const Affine2D& new_matrix,
                             UpdateCallback callback, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override;
    bool mergeWith(const QUndoCommand* other) override;

private:
    FlameGenome* m_genome;
    size_t m_xform_idx;
    bool m_is_post;
    Affine2D m_old_matrix;
    Affine2D m_new_matrix;
    UpdateCallback m_callback;
};

/// @brief Command for modifying a variation weight on an Xform
class ChangeVariationWeightCommand : public QUndoCommand {
public:
    ChangeVariationWeightCommand(FlameGenome* genome, size_t xform_idx,
                                 const std::string& var_name, double old_weight, double new_weight,
                                 UpdateCallback callback, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override;
    bool mergeWith(const QUndoCommand* other) override;

    const std::string& var_name() const { return m_var_name; }

private:
    FlameGenome* m_genome;
    size_t m_xform_idx;
    std::string m_var_name;
    double m_old_weight;
    double m_new_weight;
    UpdateCallback m_callback;
};

/// @brief Command for modifying a variation parameter variable (e.g. julian_power)
class ChangeParamValueCommand : public QUndoCommand {
public:
    ChangeParamValueCommand(FlameGenome* genome, size_t xform_idx,
                            const std::string& param_name, double old_val, double new_val,
                            UpdateCallback callback, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override;
    bool mergeWith(const QUndoCommand* other) override;

    const std::string& param_name() const { return m_param_name; }

private:
    FlameGenome* m_genome;
    size_t m_xform_idx;
    std::string m_param_name;
    double m_old_val;
    double m_new_val;
    UpdateCallback m_callback;
};

/// @brief Command for modifying camera properties (scale, pan, zoom, rotate)
class ChangeCameraCommand : public QUndoCommand {
public:
    struct CameraState {
        double scale = 150.0;
        double center_x = 0.0;
        double center_y = 0.0;
        double rotate = 0.0;
        double zoom = 0.0;
    };

    ChangeCameraCommand(FlameGenome* genome, const CameraState& old_state, const CameraState& new_state,
                        UpdateCallback callback, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override;
    bool mergeWith(const QUndoCommand* other) override;

private:
    FlameGenome* m_genome;
    CameraState m_old_state;
    CameraState m_new_state;
    UpdateCallback m_callback;
};

/// @brief Command for modifying tone mapping parameters (gamma, brightness, vibrancy)
class ChangeToneMapCommand : public QUndoCommand {
public:
    ChangeToneMapCommand(FlameGenome* genome, const ToneMapParams& old_params, const ToneMapParams& new_params,
                         UpdateCallback callback, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override;
    bool mergeWith(const QUndoCommand* other) override;

private:
    FlameGenome* m_genome;
    ToneMapParams m_old_params;
    ToneMapParams m_new_params;
    UpdateCallback m_callback;
};

/// @brief Command for changing palette
class ChangePaletteCommand : public QUndoCommand {
public:
    ChangePaletteCommand(FlameGenome* genome, const Palette& old_pal, const Palette& new_pal,
                         UpdateCallback callback, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    FlameGenome* m_genome;
    Palette m_old_pal;
    Palette m_new_pal;
    UpdateCallback m_callback;
};

/// @brief Command for adding an Xform
class AddXformCommand : public QUndoCommand {
public:
    AddXformCommand(FlameGenome* genome, const Xform& xform, size_t index,
                    UpdateCallback callback, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    FlameGenome* m_genome;
    Xform m_xform;
    size_t m_index;
    UpdateCallback m_callback;
};

/// @brief Command for deleting an Xform
class DeleteXformCommand : public QUndoCommand {
public:
    DeleteXformCommand(FlameGenome* genome, size_t index,
                       UpdateCallback callback, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    FlameGenome* m_genome;
    Xform m_saved_xform;
    size_t m_index;
    UpdateCallback m_callback;
};

} // namespace ApoNeo::Core::Commands
