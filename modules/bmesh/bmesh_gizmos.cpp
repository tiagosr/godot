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

EditorPluginBMesh::EditorPluginBMesh(EditorNode* editor) {
	Ref<BMesh3DGizmoPlugin> gizmo_plugin(memnew(BMesh3DGizmoPlugin));
	Node3DEditor::get_singleton()->add_gizmo_plugin(gizmo_plugin);
}

bool BMesh3DGizmoPlugin::has_gizmo(Node3D* p_spatial) {
	return Object::cast_to<BMesh>(p_spatial);
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
	BMesh *mesh = Object::cast_to<BMesh>(p_gizmo->get_spatial_node());

	p_gizmo->clear();

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

	switch (elementMode) {
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

	switch (elementMode) {
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

		if (elementMode == VERTICES)
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
		}
	}
}

String BMesh3DGizmoPlugin::get_handle_name(EditorNode3DGizmo const * p_gizmo, int p_idx) const {
	BMesh *mesh = Object::cast_to<BMesh>(p_gizmo->get_spatial_node());
	switch (editMode) {
	case POSITIONS:
		switch (elementMode) {
		case VERTICES:
			return "Vertex position";
		case EDGES:
			return "Edge position";
		case LOOPS:
			return "Loop position";
		case FACES:
			return "Face position";
		}
	case ATTRIBUTES:
		switch (elementMode) {
		case VERTICES:
		{
			Vector<Ref<BMeshAttributeDefinition>> const& attrs = mesh->get_vertex_attributes();
			ERR_FAIL_INDEX_V(p_idx, attrs.size(), String());
			return attrs[p_idx]->name;
		}
		case EDGES:
		{
			Vector<Ref<BMeshAttributeDefinition>> const& attrs = mesh->get_edge_attributes();
			ERR_FAIL_INDEX_V(p_idx, attrs.size(), String());
			return attrs[p_idx]->name;
		}
		case LOOPS:
		{
			Vector<Ref<BMeshAttributeDefinition>> const& attrs = mesh->get_loop_attributes();
			ERR_FAIL_INDEX_V(p_idx, attrs.size(), String());
			return attrs[p_idx]->name;
		}
		case FACES:
		{
			Vector<Ref<BMeshAttributeDefinition>> const& attrs = mesh->get_face_attributes();
			ERR_FAIL_INDEX_V(p_idx, attrs.size(), String());
			return attrs[p_idx]->name;
		}
		}
	}
	return String();
}

Variant BMesh3DGizmoPlugin::get_handle_value(EditorNode3DGizmo * p_gizmo, int p_idx) const {
	return Variant();
}

void BMesh3DGizmoPlugin::set_handle(EditorNode3DGizmo * p_gizmo, int p_idx, Camera3D * p_camera, Point2 const & p_point) {
}

void BMesh3DGizmoPlugin::commit_handle(EditorNode3DGizmo * p_gizmo, int p_idx, Variant const & p_restore, bool p_cancel) {
}
