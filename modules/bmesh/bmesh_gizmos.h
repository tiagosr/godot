#ifndef BMESH_GIZMOS_H
#define BMESH_GIZMOS_H

#include "bmesh.h"
#include "bmesh_instance.h"
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
	GizmoEditMode edit_mode;
	GizmoElementMode element_mode;
	String attribute_name;
	
	PackedInt32Array selected_vertex_indices;
	PackedInt32Array selected_edge_indices;
	PackedInt32Array selected_face_indices;
	PackedInt32Array selected_loop_indices;
public:

	// EditorNode3DGizmo overrides
	virtual bool has_gizmo(Node3D* p_spatial);
	virtual String get_name() const;
	virtual int get_priority() const;
	virtual bool is_selectable_when_hidden() const;

	virtual void redraw(EditorNode3DGizmo* p_gizmo);
	virtual String get_handle_name(EditorNode3DGizmo const* p_gizmo, int p_idx) const;
	virtual Variant get_handle_value(EditorNode3DGizmo* p_gizmo, int p_idx) const;
	virtual void set_handle(EditorNode3DGizmo* p_gizmo, int p_idx, Camera3D* p_camera, Point2 const& p_point);
	virtual void commit_handle(EditorNode3DGizmo* p_gizmo, int p_idx, Variant const& p_restore, bool p_cancel);
	// ~EditorNode3DGizmo overrides
	BMesh3DGizmoPlugin();
};

class BMeshEditorPlugin : public EditorPlugin {
    GDCLASS(BMeshEditorPlugin, EditorPlugin);

private:
	BMeshInstance3D* mesh_instance;
	BMesh* mesh;

	Separator* sep_1;
	ToolButton* btn_mesh_edit_pos;
	ToolButton* btn_mesh_edit_attrs;
	Separator* sep_2;
	ToolButton* btn_mesh_vertices;
	ToolButton* btn_mesh_edges;
	ToolButton* btn_mesh_loops;
	ToolButton* btn_mesh_faces;
	Separator* sep_3;
	MenuButton* handle_menu;
	MenuButton* attr_menu;

	EditorNode* editor;

	void _edit_mode_changed(int p_idx);
	void _edit_element_changed(int p_idx);

protected:
	void _notification(int p_what);
	static void _bind_methods();
public:

	BMesh* get_edited_mesh() { return mesh; }
	static BMesh3DGizmoPlugin* singleton;

	// EditorPlugin overrides
	virtual bool has_main_screen() const { return false; }
	virtual bool forward_spatial_gui_input(Camera3D* p_camera, Ref<InputEvent> const& p_event);
	virtual bool handles(Object* p_object) const;
	virtual void edit(Object* p_object);
	virtual void make_visible(bool p_visible);
	// ~EditorPlugin overrides


	BMeshEditorPlugin(EditorNode *editor);
};

#endif // BMESH_GIZMOS_H
