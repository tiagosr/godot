#ifndef BMESH_GIZMOS_H
#define BMESH_GIZMOS_H

#include "bmesh.h"
#include "editor/editor_plugin.h"
#include "editor/node_3d_editor_gizmos.h"

class BMesh3DGizmoPlugin : public EditorNode3DGizmoPlugin {
    GDCLASS(BMesh3DGizmoPlugin, EditorNode3DGizmoPlugin);

protected:
	enum GizmoEditMode {
		POSITIONS,
		ATTRIBUTES
	};
	enum GizmoElementMode {
		VERTICES,
		EDGES,
		LOOPS,
		FACES
	};
	GizmoEditMode editMode;
	GizmoElementMode elementMode;
	
	PackedInt32Array selected_vertex_indices;
	PackedInt32Array selected_edge_indices;
	PackedInt32Array selected_face_indices;
	PackedInt32Array selected_loop_indices;
public:
	virtual bool has_gizmo(Node3D* p_spatial) override;
	virtual String get_name() const override;
	virtual int get_priority() const override;
	virtual bool is_selectable_when_hidden() const override;

	virtual void redraw(EditorNode3DGizmo* p_gizmo) override;
	virtual String get_handle_name(EditorNode3DGizmo const* p_gizmo, int p_idx) const override;
	virtual Variant get_handle_value(EditorNode3DGizmo* p_gizmo, int p_idx) const override;
	virtual void set_handle(EditorNode3DGizmo* p_gizmo, int p_idx, Camera3D* p_camera, Point2 const& p_point) override;
	virtual void commit_handle(EditorNode3DGizmo* p_gizmo, int p_idx, Variant const& p_restore, bool p_cancel) override;

    BMesh3DGizmoPlugin();
};

class EditorPluginBMesh : public EditorPlugin {
    GDCLASS(EditorPluginBMesh, EditorPlugin);

public:
    EditorPluginBMesh(EditorNode *editor);
};

#endif // BMESH_GIZMOS_H
