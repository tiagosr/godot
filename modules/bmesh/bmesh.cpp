#include "bmesh.h"
#include "servers/rendering_server.h"

void BMesh::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_update"), &BMesh::_update);

	ClassDB::bind_method(D_METHOD("set_material", "material"), &BMesh::set_material);
	ClassDB::bind_method(D_METHOD("get_material"), &BMesh::get_material);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "material", PROPERTY_HINT_RESOURCE_TYPE, "StandardMaterial3D,ShaderMaterial"), "set_material", "get_material");

}

void BMesh::_create_mesh_array(Array arr) {
	Vector<Vector3> points;
	Vector<Vector3> normals;
	Vector<float> tangents;
	Vector<Vector2> uv1;
	Vector<Vector2> uv2;
	Vector<int> indices;
	static Variant zero_uv_var(Vector2(0.0f, 0.0f));

	points.resize(vertices.size());
	uv1.resize(vertices.size());
	uv2.resize(vertices.size());

	bool has_uv1 = has_vertex_attribute("uv");
	bool has_uv2 = has_vertex_attribute("uv2");
	bool has_tangent = has_vertex_attribute("tangent");

	for (size_t i = 0, c = vertices.size(); i < c; ++i) {
		Ref<BMeshVertex> vtx = vertices[i];
		vtx->id = i;
		points.set(i, vtx->point);
		if (has_uv1) {
			Variant v = vtx->attributes.get("uv", zero_uv_var);
			if (v.get_type() == Variant::Type::VECTOR2) {
				uv1.set(i, v);
			} else {
				uv1.set(i, Vector2(0, 0));
			}
		} else {
			uv1.set(i, Vector2(0, 0));
		}
		if (has_uv2) {
			Variant v = vtx->attributes.get("uv2", zero_uv_var);
			if (v.get_type() == Variant::Type::VECTOR2) {
				uv2.set(i, v);
			} else {
				uv2.set(i, Vector2(0, 0));
			}
		} else {
			uv2.set(i, Vector2(0, 0));
		}
	}
	for (size_t i = 0, c = faces.size(); i < c; ++i) {

	}

}

void BMesh::_request_update() {
	if (pending_update_request) {
		return;
	}
	_update();
}

void BMesh::_update() const {
	Array arr;
	arr.resize(RS::ARRAY_MAX);
	const_cast<BMesh*>(this)->_create_mesh_array(arr);

	Vector<Vector3> points = arr[RS::ARRAY_VERTEX];
	aabb = AABB();

	size_t pc = points.size();
	ERR_FAIL_COND(pc == 0);

	Vector3 const* r = points.ptr();
	for (size_t i = 0; i < pc; ++i) {
		if (i == 0) {
			aabb.position = r[i];
		} else {
			aabb.expand_to(r[i]);
		}
	}

	Vector<int> indices = arr[RS::ARRAY_INDEX];

	if (flip_faces) {
		Vector<Vector3> normals = arr[RS::ARRAY_NORMAL];
		if (normals.size() && indices.size()) {
			size_t nc = normals.size();
			Vector3* nw = normals.ptrw();
			for (size_t i = 0; i < nc; ++i) {
				// invert normals
				nw[i] = -nw[i];
			}

			size_t ic = indices.size();
			int* ni = indices.ptrw();
			for (size_t i = 0; i < ic; i += 3) {
				SWAP(nw[i + 0], nw[i + 1]);
			}
			arr[RS::ARRAY_NORMAL] = normals;
			arr[RS::ARRAY_INDEX] = indices;
		}
	}

	array_len = pc;
	index_array_len = indices.size();

	RenderingServer::get_singleton()->mesh_clear(rid);
	RenderingServer::get_singleton()->mesh_add_surface_from_arrays(rid, (RenderingServer::PrimitiveType)primitive_type, arr);
	RenderingServer::get_singleton()->mesh_surface_set_material(rid, 0, material.is_null() ? RID() : material->get_rid());

	pending_update_request = false;

	clear_cache();
	const_cast<BMesh *>(this)->emit_changed();
}

Ref<BMeshEdge> BMesh::add_edge(Ref<BMeshVertex> vert1, Ref<BMeshVertex> vert2) {
	ERR_FAIL_COND_V_MSG(vert1 == vert2, Ref<BMeshEdge>(), "vertices must not be the same");

	Ref<BMeshEdge> edge = find_edge(vert1, vert2);
	if (edge.is_valid()) return edge;

	edge.instance();
	edge->vert1 = vert1;
	edge->vert2 = vert2;
	ensure_edge_attributes(edge);
	edges.append(edge);

	if (vert1->edge.is_null()) {
		vert1->edge = edge;
		edge->next1 = edge;
		edge->prev1 = edge;
	} else {
		edge->next1 = vert1->edge->get_next(*vert1);
		edge->prev1 = vert1->edge;
		edge->next1->set_prev(*vert1, edge);
		edge->prev1->set_next(*vert1, edge);
	}

	if (vert2->edge.is_null()) {
		vert2->edge = edge;
		edge->next2 = edge;
		edge->prev2 = edge;
	} else {
		edge->next2 = vert2->edge->get_next(*vert2);
		edge->prev2 = vert2->edge;
		edge->next2->set_prev(*vert2, edge);
		edge->prev2->set_next(*vert2, edge);
	}

	return edge;
}

Ref<BMeshFace> BMesh::add_face(Vector<Ref<BMeshVertex>> const & fVerts) {
	if (fVerts.empty()) return Ref<BMeshFace>();

	for (size_t i = 0; i < fVerts.size(); i++) {
		ERR_FAIL_COND_V_MSG(fVerts[i].is_null(), Ref<BMeshFace>(), "Invalid vertex");
	}

	Vector<Ref<BMeshEdge>> fEdges;
	fEdges.resize(fVerts.size());
	size_t i, i_prev = fVerts.size() - 1;
	for (i = 0; i < fVerts.size(); ++i) {
		fEdges.set(i_prev, add_edge(fVerts[i_prev], fVerts[i]));
		i_prev = i;
	}

	Ref<BMeshFace> f;
	f.instance();
	ensure_face_attributes(f);
	faces.append(f);

	for (i = 0; i < fVerts.size(); ++i) {
		Ref<BMeshLoop> loop(memnew(BMeshLoop(fVerts[i], fEdges[i], f)));
		ensure_loop_attributes(loop);
		loops.append(loop);
	}

	f->vertcount = fVerts.size();
	return f;
}

BMesh::BMesh() {
	flip_faces = false;
	rid = RenderingServer::get_singleton()->mesh_create();
	primitive_type = Mesh::PRIMITIVE_TRIANGLES;

	pending_update_request = true;
}

BMesh::~BMesh() {
	RenderingServer::get_singleton()->free(rid);
}

Vector<Ref<BMeshEdge>> BMeshVertex::neighbor_edges() const {
	Vector<Ref<BMeshEdge>> edges;
	if (edge.is_valid()) {
		Ref<BMeshEdge> it = edge;
		do {
			edges.append(it);
			it = it->get_next(this);
		} while (it != edge);
	}
	return edges;
}

Vector<Ref<BMeshFace>> BMeshVertex::neighbor_faces() const {
	Vector<Ref<BMeshFace>> faces;
	if (edge.is_valid()) {
		Ref<BMeshEdge> it = edge;
		do {
			faces.append_array(it->neighbor_faces());
			it = it->get_next(this);
		} while (it != edge);
	}
	return faces;
}

inline Vector<Ref<BMeshFace>> BMeshEdge::neighbor_faces() const {
	Vector<Ref<BMeshFace>> faces;
	if (loop.is_valid()) {
		Ref<BMeshLoop> it = loop;
		do {
			faces.append(it->face);
			it = it->radial_next;
		} while (it != loop);
	}
	return faces;
}

void BMeshLoop::set_face(Ref<BMeshFace> f) {
	CRASH_COND_MSG(f.is_null(), "face is invalid");

	Ref<BMeshFace> thisref(this);
	if (f->loop.is_null()) {
		f->loop = thisref;
		next = thisref;
		prev = thisref;
	} else {
		prev = f->loop;
		next = f->loop->next;

		f->loop->next->prev = thisref;
		f->loop->next = thisref;

		f->loop = thisref;
	}
	face = f;
}

void BMeshLoop::set_edge(Ref<BMeshEdge> e) {
	CRASH_COND_MSG(e.is_null(), "edge is invalid");

	Ref<BMeshFace> thisref(this);
	if (e->loop.is_null()) {
		e->loop = thisref;
		radial_next = thisref;
		radial_prev = thisref;
	} else {
		radial_prev = e->loop;
		radial_next = e->loop->radial_next;

		e->loop->radial_next->radial_prev = thisref;
		e->loop->radial_next = thisref;

		e->loop = thisref;
	}
	edge = e;
}

Vector<Ref<BMeshVertex>> BMeshFace::neighbor_vertices() const {
	Vector<Ref<BMeshVertex>> verts;

	if (loop.is_valid()) {
		Ref<BMeshLoop> it = loop;
		do {
			verts.append(it->vert);
			it = it->next;
		} while (it != loop);
	}
	return verts;
}

Vector<Ref<BMeshEdge>> BMeshFace::neighbor_edges() const {
	Vector<Ref<BMeshEdge>> edges;

	if (loop.is_valid()) {
		Ref<BMeshLoop> it = loop;
		do {
			edges.append(it->edge);
			it = it->next;
		} while (it != loop);
	}
	return edges;
}

Ref<BMeshAttributeDefinition> BMesh::add_face_attribute(Ref<BMeshAttributeDefinition> attrib) {
	if (has_face_attribute(attrib->name)) return attrib;
	faceAttributes.append(attrib);
	for (size_t i = 0, c = vertices.size(); i < c; ++i) {
		Ref<BMeshVertex> vert = vertices[i];
		vert->attributes[attrib->name] = attrib->defaultValue.duplicate();
	}
	return attrib;
}

Ref<BMeshAttributeDefinition> BMesh::add_face_attribute(String name, BMeshAttributeDefinition::Type type, int dimensions) {
	return add_face_attribute(memnew(BMeshAttributeDefinition(name, type, dimensions)));
}

void BMesh::ensure_face_attributes(Ref<BMeshFace> f) {
	for (size_t i = 0, c = faceAttributes.size(); i < c; ++i) {
		Ref<BMeshAttributeDefinition> attr = faceAttributes[i];
		if (!f->attributes.has(attr->name)) {
			f->attributes[attr->name] = attr->defaultValue.duplicate();
		} else if (!attr->check_value(f->attributes[attr->name])) {
			WARN_PRINT("Face attribute not compatible with attribute definition, reverting to default value");
			f->attributes[attr->name] = attr->defaultValue.duplicate();
		}
	}
}

void BMesh::set_material(Ref<Material> const & mat) {
	material = mat;
	if (!pending_update_request) {
		RenderingServer::get_singleton()->mesh_surface_set_material(rid, 0, material.is_null() ? RID() : material->get_rid());
		_change_notify();
		emit_changed();
	}
}

int BMesh::get_surface_count() const {
	if (pending_update_request) { _update(); }
	return 1;
}

int BMesh::surface_get_array_len(int surface) const {
	ERR_FAIL_INDEX_V(surface, 1, -1);
	if (pending_update_request) { _update(); }
	return array_len;
}

int BMesh::surface_get_array_index_len(int surface) const {
	ERR_FAIL_INDEX_V(surface, 1, -1);
	if (pending_update_request) { _update(); }
	return index_array_len;
}

Array BMesh::surface_get_arrays(int surface) const {
	ERR_FAIL_INDEX_V(surface, 1, Array());
	if (pending_update_request) { _update(); }
	return RenderingServer::get_singleton()->mesh_surface_get_arrays(rid, 0);
}

Array BMesh::surface_get_blend_shape_arrays(int p_surface) const {
	return Array(); // not currently supported
}

Dictionary BMesh::surface_get_lods(int surface) const {
	return Dictionary(); // not currently supported
}

uint32_t BMesh::surface_get_format(int surface) const {
	ERR_FAIL_INDEX_V(surface, 1, 0);
	uint32_t uv_flags = (has_vertex_attribute("uv") ? RS::ARRAY_FORMAT_TEX_UV : 0) | (has_vertex_attribute("uv2") ? RS::ARRAY_FORMAT_TEX_UV2 : 0);
	uint32_t tangent_flags = (has_vertex_attribute("tangent") ? RS::ARRAY_FORMAT_TANGENT : 0);
	return RS::ARRAY_FORMAT_VERTEX | RS::ARRAY_FORMAT_NORMAL | tangent_flags | uv_flags | RS::ARRAY_FORMAT_INDEX | RS::ARRAY_COMPRESS_DEFAULT;
}

Mesh::PrimitiveType BMesh::surface_get_primitive_type(int surface) const {
	ERR_FAIL_INDEX_V(surface, 1, Mesh::PrimitiveType::PRIMITIVE_POINTS);
	return primitive_type;
}

void BMesh::surface_set_material(int surface, Ref<Material> const & material) {
	ERR_FAIL_INDEX(surface, 1);

	set_material(material);
}

Ref<Material> BMesh::surface_get_material(int surface) const {
	ERR_FAIL_INDEX_V(surface, 1, nullptr);

	return material;
}

int BMesh::get_blend_shape_count() const {
	return 0;
}

StringName BMesh::get_blend_shape_name(int p_index) const {
	return StringName();
}

AABB BMesh::get_aabb() const {
	if (pending_update_request) { _update(); }
	return aabb;
}

RID BMesh::get_rid() const {
	if (pending_update_request) { _update(); }
	return rid;
}

Ref<BMeshEdge> BMesh::find_edge(Ref<BMeshVertex> vert1, Ref<BMeshVertex> vert2)
{
	CRASH_COND(vert1 == vert2);
	if (vert1->edge.is_null() || vert2->edge.is_null()) return nullptr;

	Ref<BMeshEdge> e1 = vert1->edge;
	Ref<BMeshEdge> e2 = vert2->edge;

	do {
		if (e1->contains_vertex(*vert2)) return e1;
		if (e2->contains_vertex(*vert1)) return e2;
		e1 = e1->get_next(*vert1);
		e2 = e2->get_next(*vert2);
	} while (e1 != vert1->edge && e2 != vert2->edge);

	return nullptr;
}

void BMesh::remove_vertex(Ref<BMeshVertex> v) {
	while (v->edge.is_valid()) {
		remove_edge(v->edge);
	}

	vertices.erase(v);
	pending_update_request = true;
}

void BMesh::remove_edge(Ref<BMeshEdge> e) {
	while (e->loop.is_valid()) {
		remove_loop(e->loop);
	}

	// remove edge references in vertices
	if (e == e->vert1->edge) e->vert1->edge = (e->next1 != e) ? e->next1 : nullptr;
	if (e == e->vert2->edge) e->vert2->edge = (e->next2 != e) ? e->next2 : nullptr;

	// remove from the linked lists
	e->prev1->set_next(*e->vert1, e->next1);
	e->next1->set_prev(*e->vert1, e->prev1);

	e->prev2->set_next(*e->vert2, e->next2);
	e->next2->set_prev(*e->vert2, e->prev2);

	edges.erase(e);
	pending_update_request = true;
}

void BMesh::remove_loop(Ref<BMeshLoop> l) {
	if (l->face.is_valid()) {
		remove_face(l->face);
		return;
	}

	if (l->radial_next == l) {
		l->edge->loop = nullptr;
	} else {
		l->radial_prev->radial_next = l->radial_next;
		l->radial_next->radial_prev = l->radial_prev;
		if (l->edge->loop == l) {
			l->edge->loop = l->radial_next;
		}
	}

	l->next = nullptr;
	l->prev = nullptr;

	loops.erase(l);
	pending_update_request = true;
}

void BMesh::remove_face(Ref<BMeshFace> f) {
	Ref<BMeshLoop> l = f->loop;
	Ref<BMeshLoop> nextL = nullptr;
	while (nextL != f->loop) {
		nextL = l->next;
		l->face = nullptr;
		remove_loop(l);
		l = nextL;
	}
	faces.erase(f);
	pending_update_request = true;
}

bool BMesh::has_vertex_attribute(String const& attribName) const {
	for (size_t i = 0, c = vertexAttributes.size(); i < c; ++i) {
		if (vertexAttributes[i]->name == attribName) {
			return true;
		}
	}
	return false;
}

Ref<BMeshAttributeDefinition> BMesh::add_vertex_attribute(Ref<BMeshAttributeDefinition> attrib) {
	if (has_vertex_attribute(attrib)) return attrib;
	vertexAttributes.append(attrib);
	for (size_t i = 0, c = vertices.size(); i < c; ++i) {
		Ref<BMeshVertex> vert = vertices[i];
		vert->attributes[attrib->name] = attrib->defaultValue.duplicate();
	}
	return attrib;
}

Ref<BMeshAttributeDefinition> BMesh::add_vertex_attribute(String name, BMeshAttributeDefinition::Type type, int dimensions) {
	return add_vertex_attribute(memnew(BMeshAttributeDefinition(name, type, dimensions)));
}

void BMesh::ensure_vertex_attributes(Ref<BMeshVertex> v) {
	for (size_t i = 0, c = vertexAttributes.size(); i < c; ++i) {
		Ref<BMeshAttributeDefinition> attr = vertexAttributes[i];
		if (!v->attributes.has(attr->name)) {
			v->attributes[attr->name] = attr->defaultValue.duplicate();
		} else if (!attr->check_value(v->attributes[attr->name])) {
			WARN_PRINT("Vertex attribute not compatible with attribute definition, reverting to default value");
			v->attributes[attr->name] = attr->defaultValue.duplicate();
		}
	}
}

bool BMesh::has_edge_attribute(String attribName) const {
	for (size_t i = 0, c = edgeAttributes.size(); i < c; ++i) {
		if (edgeAttributes[i]->name == attribName) {
			return true;
		}
	}
	return false;
}

Ref<BMeshAttributeDefinition> BMesh::add_edge_attribute(Ref<BMeshAttributeDefinition> attrib) {
	if (has_edge_attribute(attrib->name)) return attrib;
	edgeAttributes.append(attrib);
	for (size_t i = 0, c = vertices.size(); i < c; ++i) {
		Ref<BMeshVertex> vert = vertices[i];
		vert->attributes[attrib->name] = attrib->defaultValue.duplicate();
	}
	return attrib;
}

Ref<BMeshAttributeDefinition> BMesh::add_edge_attribute(String name, BMeshAttributeDefinition::Type type, int dimensions) {
	return add_edge_attribute(memnew(BMeshAttributeDefinition(name, type, dimensions)));
}

void BMesh::ensure_edge_attributes(Ref<BMeshEdge> e) {
	for (size_t i = 0, c = edgeAttributes.size(); i < c; ++i) {
		Ref<BMeshAttributeDefinition> attr = edgeAttributes[i];
		if (!e->attributes.has(attr->name)) {
			e->attributes[attr->name] = attr->defaultValue.duplicate();
		} else if (!attr->check_value(e->attributes[attr->name])) {
			WARN_PRINT("edge attribute not compatible with attribute definition, reverting to default value");
			e->attributes[attr->name] = attr->defaultValue.duplicate();
		}
	}
}

bool BMesh::has_loop_attribute(String const & attribName) const {
	for (size_t i = 0, c = loopAttributes.size(); i < c; ++i) {
		if (loopAttributes[i]->name == attribName) {
			return true;
		}
	}
	return false;
}

Ref<BMeshAttributeDefinition> BMesh::add_loop_attribute(Ref<BMeshAttributeDefinition> attrib) {
	if (has_loop_attribute(attrib->name)) return attrib;
	loopAttributes.append(attrib);
	for (size_t i = 0, c = vertices.size(); i < c; ++i) {
		Ref<BMeshVertex> vert = vertices[i];
		vert->attributes[attrib->name] = attrib->defaultValue.duplicate();
	}
	return attrib;
}

Ref<BMeshAttributeDefinition> BMesh::add_loop_attribute(String name, BMeshAttributeDefinition::Type type, int dimensions) {
	return add_loop_attribute(memnew(BMeshAttributeDefinition(name, type, dimensions)));
}

void BMesh::ensure_loop_attributes(Ref<BMeshLoop> l) {
	for (size_t i = 0, c = loopAttributes.size(); i < c; ++i) {
		Ref<BMeshAttributeDefinition> attr = loopAttributes[i];
		if (!l->attributes.has(attr->name)) {
			l->attributes[attr->name] = attr->defaultValue;
		} else if (!attr->check_value(l->attributes[attr->name])) {
			WARN_PRINT("Loop attribute not compatible with attribute definition, reverting to default value");
			l->attributes[attr->name] = attr->defaultValue;
		}
	}
}

bool BMesh::has_face_attribute(String attribName) const {
	for (size_t i = 0, c = faceAttributes.size(); i < c; ++i) {
		if (faceAttributes[i]->name == attribName) {
			return true;
		}
	}
	return false;
}
