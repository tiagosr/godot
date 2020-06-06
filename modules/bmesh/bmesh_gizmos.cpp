#include "bmesh_gizmos.h"

BMesh3DGizmoPlugin::BMesh3DGizmoPlugin() {
	Color face_gizmo_color = EDITOR_DEF("editors/3d_gizmos/gizmo_colors/bmesh_face", Color(1.0f, 0.4f, 0.0f, 0.15));
	Color edge_selected_gizmo_color = EDITOR_DEF("editors/3d_gizmos/gizmo_colors/bmesh_edge_selected", Color(1.0f, 0.4f, 0.0f, 1));
	Color edge_unselected_gizmo_color = EDITOR_DEF("editors/3d_gizmos/gizmo_colors/bmesh_edge_unselected", Color(0.0f, 0.0f, 0.0f, 1));

	create_material("bmesh_face_material", face_gizmo_color);
	create_material("bmesh_edge_selected_material", edge_selected_gizmo_color);
	create_material("bmesh_edge_unselected_material", edge_unselected_gizmo_color);

	create_handle_material("handles");
}

BMesh3DGizmoPlugin* BMeshEditorPlugin::singleton = nullptr;

void BMeshEditorPlugin::_edit_mode_changed(int p_idx) {
	btn_mesh_edit_pos->set_pressed(p_idx == 0);
	btn_mesh_edit_attrs->set_pressed(p_idx == 1);
}

void BMeshEditorPlugin::_edit_element_changed(int p_idx) {
	btn_mesh_vertices->set_pressed(p_idx == 0);
	btn_mesh_edges->set_pressed(p_idx == 1);
	btn_mesh_loops->set_pressed(p_idx == 2);
	btn_mesh_faces->set_pressed(p_idx == 3);
}

void BMeshEditorPlugin::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE) {
		btn_mesh_edit_pos->connect("pressed", callable_mp(this, &BMeshEditorPlugin::_edit_mode_changed), make_binds(0));
		btn_mesh_edit_attrs->connect("pressed", callable_mp(this, &BMeshEditorPlugin::_edit_mode_changed), make_binds(1));

		btn_mesh_vertices->connect("pressed", callable_mp(this, &BMeshEditorPlugin::_edit_element_changed), make_binds(0));
		btn_mesh_edges->connect("pressed", callable_mp(this, &BMeshEditorPlugin::_edit_element_changed), make_binds(1));
		btn_mesh_loops->connect("pressed", callable_mp(this, &BMeshEditorPlugin::_edit_element_changed), make_binds(2));
		btn_mesh_faces->connect("pressed", callable_mp(this, &BMeshEditorPlugin::_edit_element_changed), make_binds(3));
	}
}

void BMeshEditorPlugin::_bind_methods() {
}

bool BMeshEditorPlugin::forward_spatial_gui_input(Camera3D * p_camera, Ref<InputEvent> const & p_event) {
	if (!mesh) return false;

	///TODO: Implement

	return false;
}

bool BMeshEditorPlugin::handles(Object * p_object) const {
	return p_object->is_class("BMeshInstance3D");
}

void BMeshEditorPlugin::edit(Object * p_object) {
	if (p_object) {
		mesh_instance = Object::cast_to<BMeshInstance3D>(p_object);
		mesh = mesh_instance->get_mesh().ptr();
		if (mesh) {
			mesh->emit_signal("changed");
		}
	} else {
		BMesh* prev = mesh;
		mesh = nullptr;
		if (prev) {
			prev->emit_signal("changed");
		}
	}
}

void BMeshEditorPlugin::make_visible(bool p_visible) {
	if (p_visible) {
		sep_1->show();
		btn_mesh_edit_pos->show();
		btn_mesh_edit_attrs->show();
		sep_2->show();
		btn_mesh_vertices->show();
		btn_mesh_edges->show();
		btn_mesh_loops->show();
		btn_mesh_faces->show();
		sep_3->show();
		handle_menu->show();
	}
}

BMeshEditorPlugin::BMeshEditorPlugin(EditorNode* editor) {
	Ref<BMesh3DGizmoPlugin> gizmo_plugin(memnew(BMesh3DGizmoPlugin));
	Node3DEditor::get_singleton()->add_gizmo_plugin(gizmo_plugin);

	sep_1 = memnew(VSeparator);
	sep_1->hide();
	Node3DEditor::get_singleton()->add_control_to_menu_panel(sep_1);

	btn_mesh_edit_pos = memnew(ToolButton);
	btn_mesh_edit_pos->set_icon(EditorNode::get_singleton()->get_gui_base()->get_theme_icon("CurveEdit", "EditorIcons"));
	btn_mesh_edit_pos->set_toggle_mode(true);
	btn_mesh_edit_pos->set_focus_mode(Control::FOCUS_NONE);
	btn_mesh_edit_pos->hide();
	btn_mesh_edit_pos->set_tooltip(TTR("Edit vertex positions"));
	Node3DEditor::get_singleton()->add_control_to_menu_panel(btn_mesh_edit_pos);

	btn_mesh_edit_attrs = memnew(ToolButton);
	btn_mesh_edit_attrs->set_icon(EditorNode::get_singleton()->get_gui_base()->get_theme_icon("CurveEdit", "EditorIcons"));
	btn_mesh_edit_attrs->set_toggle_mode(true);
	btn_mesh_edit_attrs->set_focus_mode(Control::FOCUS_NONE);
	btn_mesh_edit_attrs->hide();
	btn_mesh_edit_attrs->set_tooltip(TTR("Edit vertex attributes"));
	Node3DEditor::get_singleton()->add_control_to_menu_panel(btn_mesh_edit_attrs);

	sep_2 = memnew(VSeparator);
	sep_2->hide();
	Node3DEditor::get_singleton()->add_control_to_menu_panel(sep_2);

	btn_mesh_vertices = memnew(ToolButton);
	btn_mesh_vertices->set_icon(EditorNode::get_singleton()->get_gui_base()->get_theme_icon("CurveEdit", "EditorIcons"));
	btn_mesh_vertices->set_toggle_mode(true);
	btn_mesh_vertices->set_focus_mode(Control::FOCUS_NONE);
	btn_mesh_vertices->hide();
	btn_mesh_vertices->set_tooltip(TTR("Edit vertices"));
	Node3DEditor::get_singleton()->add_control_to_menu_panel(btn_mesh_vertices);

	btn_mesh_edges = memnew(ToolButton);
	btn_mesh_edges->set_icon(EditorNode::get_singleton()->get_gui_base()->get_theme_icon("CurveEdit", "EditorIcons"));
	btn_mesh_edges->set_toggle_mode(true);
	btn_mesh_edges->set_focus_mode(Control::FOCUS_NONE);
	btn_mesh_edges->hide();
	btn_mesh_edges->set_tooltip(TTR("Edit edges"));
	Node3DEditor::get_singleton()->add_control_to_menu_panel(btn_mesh_edges);

	btn_mesh_loops = memnew(ToolButton);
	btn_mesh_loops->set_icon(EditorNode::get_singleton()->get_gui_base()->get_theme_icon("CurveEdit", "EditorIcons"));
	btn_mesh_loops->set_toggle_mode(true);
	btn_mesh_loops->set_focus_mode(Control::FOCUS_NONE);
	btn_mesh_loops->hide();
	btn_mesh_loops->set_tooltip(TTR("Edit loops"));
	Node3DEditor::get_singleton()->add_control_to_menu_panel(btn_mesh_loops);

	btn_mesh_faces = memnew(ToolButton);
	btn_mesh_faces->set_icon(EditorNode::get_singleton()->get_gui_base()->get_theme_icon("CurveEdit", "EditorIcons"));
	btn_mesh_faces->set_toggle_mode(true);
	btn_mesh_faces->set_focus_mode(Control::FOCUS_NONE);
	btn_mesh_faces->hide();
	btn_mesh_faces->set_tooltip(TTR("Edit faces"));
	Node3DEditor::get_singleton()->add_control_to_menu_panel(btn_mesh_faces);

	sep_3 = memnew(VSeparator);
	sep_3->hide();
	Node3DEditor::get_singleton()->add_control_to_menu_panel(sep_3);


	handle_menu = memnew(MenuButton);
	handle_menu->set_text(TTR("Options"));
	handle_menu->hide();
	Node3DEditor::get_singleton()->add_control_to_menu_panel(handle_menu);

}

bool BMesh3DGizmoPlugin::has_gizmo(Node3D* p_spatial) {
	return Object::cast_to<BMeshInstance3D>(p_spatial);
}

String BMesh3DGizmoPlugin::get_name() const {
	return "BMesh";
}

int BMesh3DGizmoPlugin::get_priority() const {
	return -1;
}

bool BMesh3DGizmoPlugin::is_selectable_when_hidden() const {
	return true;
}

void BMesh3DGizmoPlugin::redraw(EditorNode3DGizmo * p_gizmo) {
	BMeshInstance3D* instance =  Object::cast_to<BMeshInstance3D>(p_gizmo->get_spatial_node());

	BMesh *mesh = instance->get_mesh().ptr();

	p_gizmo->clear();

	if (!mesh) {
		return;
	}

	Ref<Material> handles_material = get_material("handles");

	Ref<Material> face_material = get_material("bmesh_face_material", p_gizmo);
	Ref<Material> edge_selected_material = get_material("bmesh_selected_material", p_gizmo);
	Ref<Material> edge_unselected_material = get_material("bmesh_unselected_material", p_gizmo);

	Vector<Ref<BMeshVertex>> const& vertices = mesh->get_vertices();
	Vector<Ref<BMeshEdge>> const& edges = mesh->get_edges();
	Vector<Ref<BMeshFace>> const& faces = mesh->get_faces();

	PackedVector3Array unselectedEdgeLines;
	PackedVector3Array selectedEdgeLines;
	
	PackedInt32Array selectedEdgeIds;

	switch (element_mode) {
	case EDGES:
		selectedEdgeIds = selected_edge_indices;
		break;
	case LOOPS:
		for (size_t i = 0, c = selected_loop_indices.size(); i < c; ++i) {
			
		}
		break;
	}

	

	unselectedEdgeLines.resize((edges.size() - selectedEdgeIds.size()) * 2);
	selectedEdgeLines.resize(selectedEdgeIds.size() * 2);


	for (size_t i = 0, unselected = 0, selected = 0, c = edges.size(); i < c; ++i) {
		if (selectedEdgeIds.find(edges[i]->id) >= 0) {
			selected += edges[i]->fill_lines_vector3_array(selectedEdgeLines, selected);
		} else {
			unselected += edges[i]->fill_lines_vector3_array(unselectedEdgeLines, unselected);
		}
	}

	switch (element_mode) {
	case EDGES:
	case LOOPS:
		p_gizmo->add_collision_segments(unselectedEdgeLines);
		p_gizmo->add_collision_segments(selectedEdgeLines);
		break;
	case FACES:
	{
		Ref<TriangleMesh> trimesh;
		trimesh.instance();
		PackedVector3Array facesTriangles;
		facesTriangles.resize(mesh->get_num_tris()*3);
		mesh->generate_tris(facesTriangles, 0);
		trimesh->create(facesTriangles);
		p_gizmo->add_collision_triangles(trimesh);
		break;
	}
	}

	if (p_gizmo->is_selected()) {

		if (element_mode == VERTICES)
		{
			PackedVector3Array points;
			points.resize(mesh->get_vertices().size());
			for (size_t i = 0, c = vertices.size(); i < c; ++i) {
				points.write[i] = vertices[i]->point;
			}
			p_gizmo->add_handles(points, handles_material);
		} else {
			p_gizmo->add_lines(unselectedEdgeLines, edge_unselected_material);
			p_gizmo->add_lines(selectedEdgeLines, edge_selected_material);

			if (element_mode == FACES) {
				Ref<TriangleMesh> trimesh;
				trimesh.instance();
				size_t trianglesCount = 0;
				Vector<Ref<BMeshFace>> const& faces = mesh->get_faces();
				for (size_t i = 0, c = selected_face_indices.size(); i < c; i++) {
					trianglesCount += faces[selected_face_indices[i]]->get_triangles_count();
				}
				PackedVector3Array facesTriangles;
				facesTriangles.resize(trianglesCount * 3);
				for (size_t i = 0, offset = 0, c = selected_face_indices.size(); i < c; i++) {
					offset += faces[selected_face_indices[i]]->fill_triangles_vector3_array(facesTriangles, offset);
				}
				trimesh->create(facesTriangles);
				p_gizmo->add_mesh(trimesh, false, nullptr, face_material);
			}
		}
	}
}

String BMesh3DGizmoPlugin::get_handle_name(EditorNode3DGizmo const * p_gizmo, int p_idx) const {
	BMeshInstance3D* instance = Object::cast_to<BMeshInstance3D>(p_gizmo->get_spatial_node());
	BMesh *mesh = instance->get_mesh().ptr();
	switch (edit_mode) {
	case POSITIONS:
		switch (element_mode) {
		case VERTICES:
			return TTR("Vertex #{_} position").format(p_idx);
		case EDGES:
			return TTR("Edge #{_} position").format(p_idx);
		case LOOPS:
			return TTR("Loop #{_} position").format(p_idx);
		case FACES:
			return TTR("Face #{_} position").format(p_idx);
		}
	case ATTRIBUTES:
		switch (element_mode) {
		case VERTICES:
		{
			Vector<Ref<BMeshAttributeDefinition>> const& attrs = mesh->get_vertex_attributes();
			return TTR("Vertex #{i} attribute {a}").format(p_idx, "{i}").format(attribute_name, "{a}");
		}
		case EDGES:
		{
			Vector<Ref<BMeshAttributeDefinition>> const& attrs = mesh->get_edge_attributes();
			return TTR("Edge #{i} attribute {a}").format(p_idx, "{i}").format(attribute_name, "{a}");
		}
		case LOOPS:
		{
			Vector<Ref<BMeshAttributeDefinition>> const& attrs = mesh->get_loop_attributes();
			return TTR("Loop #{i} attribute {a}").format(p_idx, "{i}").format(attribute_name, "{a}");;
		}
		case FACES:
		{
			Vector<Ref<BMeshAttributeDefinition>> const& attrs = mesh->get_face_attributes();
			return TTR("Face #{i} attribute {a}").format(p_idx, "{i}").format(attribute_name, "{a}");;
		}
		}
	}
	return String();
}

Variant BMesh3DGizmoPlugin::get_handle_value(EditorNode3DGizmo * p_gizmo, int p_idx) const {
	BMeshInstance3D* instance = Object::cast_to<BMeshInstance3D>(p_gizmo->get_spatial_node());
	BMesh *mesh = instance->get_mesh().ptr();
	switch (edit_mode) {
	case POSITIONS:
		switch (element_mode) {
		case VERTICES:
			return mesh->get_vertex(p_idx);
		}
		break;
	case ATTRIBUTES:
		switch (element_mode) {
		case VERTICES:
			return mesh->get_vertex_attribute(p_idx, attribute_name);
		}
	}
	return Variant();
}

void BMesh3DGizmoPlugin::set_handle(EditorNode3DGizmo * p_gizmo, int p_idx, Camera3D * p_camera, Point2 const & p_point) {
	BMeshInstance3D* instance = Object::cast_to<BMeshInstance3D>(p_gizmo->get_spatial_node());
	BMesh *mesh = instance->get_mesh().ptr();

}

void BMesh3DGizmoPlugin::commit_handle(EditorNode3DGizmo * p_gizmo, int p_idx, Variant const & p_restore, bool p_cancel) {
	BMeshInstance3D* instance = Object::cast_to<BMeshInstance3D>(p_gizmo->get_spatial_node());
	BMesh *mesh = instance->get_mesh().ptr();

	UndoRedo *ur = Node3DEditor::get_singleton()->get_undo_redo();

	switch (edit_mode) {
	case POSITIONS:
		switch (element_mode) {
		case VERTICES:
			if (p_cancel) {
				mesh->set_vertex(p_idx, p_restore);
				return;
			}
			ur->create_action(TTR("Set Vertex Position"));
			ur->add_do_method(mesh, "set_vertex", p_idx, mesh->get_vertex(p_idx));
			ur->add_undo_method(mesh, "set_vertex", p_idx, p_restore);
			ur->commit_action();
			return;
		}
		break;
	case ATTRIBUTES:
		switch (element_mode) {
		case VERTICES:
			if (p_cancel) {
				mesh->set_vertex_attribute(p_idx, attribute_name, p_restore);
				return;
			}
			ur->create_action(TTR("Set Vertex Attribute"));
			ur->add_do_method(mesh, "set_vertex_attribute", p_idx, attribute_name, mesh->get_vertex_attribute(p_idx, attribute_name));
			ur->add_undo_method(mesh, "set_vertex_attribute", p_idx, attribute_name, p_restore);
			ur->commit_action();
			return;
		}
	}
}
