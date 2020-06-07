#include "bmesh.h"
#include "servers/rendering_server.h"

void BMesh::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_update"), &BMesh::_update);

	ClassDB::bind_method(D_METHOD("set_material", "material"), &BMesh::set_material);
	ClassDB::bind_method(D_METHOD("get_material"), &BMesh::get_material);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "material", PROPERTY_HINT_RESOURCE_TYPE, "StandardMaterial3D,ShaderMaterial"), "set_material", "get_material");

	ClassDB::bind_method(D_METHOD("set_vertex"), &BMesh::set_vertex);
	ClassDB::bind_method(D_METHOD("get_vertex"), &BMesh::get_vertex);

	ClassDB::bind_method(D_METHOD("set_vertex_attribute"), &BMesh::set_vertex_attribute);
	ClassDB::bind_method(D_METHOD("get_vertex_attribute"), &BMesh::get_vertex_attribute);

	ClassDB::bind_method(D_METHOD("add_vertex"), &BMesh::_add_vertex);
	ClassDB::bind_method(D_METHOD("add_edge"), &BMesh::_add_edge);
	ClassDB::bind_vararg_method(METHOD_FLAGS_DEFAULT, "add_face", &BMesh::_add_face, MethodInfo("add_face"));

	ClassDB::bind_method(D_METHOD("find_edge"), &BMesh::find_edge);


}

void BMesh::_create_mesh_arrays(Array arrays) {
	bool has_uv1 = has_vertex_attribute("uv");
	bool has_uv2 = has_vertex_attribute("uv2");
	bool has_normal = has_vertex_attribute("normal");
	bool has_tangent = has_vertex_attribute("tangent");
	bool has_material = has_vertex_attribute("materialId");
	int max_materialId = 0;

	if (has_material) {
		for (size_t i = 0, c = faces.size(); i < c; ++i) {
			int id = faces[i]->attributes.get("materialId", max_materialId);
			max_materialId = MAX(max_materialId, id);
		}
	}

	PackedVector3Array points;
	PackedVector3Array normals;
	PackedFloat32Array tangents;
	PackedVector2Array uv1;
	PackedVector2Array uv2;
	static Variant zero_uv_var(Vector2(0.0f, 0.0f));

	points.resize(vertices.size());
	if (has_normal) normals.resize(vertices.size());
	if (has_uv1) uv1.resize(vertices.size());
	if (has_uv2) uv2.resize(vertices.size());
	if (has_tangent) tangents.resize(vertices.size() * 4);

	for (size_t i = 0, c = vertices.size(); i < c; ++i) {
		Ref<BMeshVertex> vtx = vertices[i];
		vtx->id = i;
		points.write[i] = vtx->point;
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
		faces.write[i]->id = i;
	}

	for (size_t i = 0; i <= max_materialId; ++i) {
		Array arr;
		arr.resize(RS::ARRAY_MAX);

		Vector<int> indices;

		for (size_t i = 0, c = faces.size(); i < c; ++i) {
			Ref<BMeshFace> const& face = faces[i];
			if (face->vertcount == 3) {
				BMeshLoop const* loop = face->loop.ptr();
				int ti = indices.size();
				indices.resize(ti + 3);
				indices.set(ti + 0, loop->vert->id); loop = loop->next.ptr();
				indices.set(ti + 2, loop->vert->id); loop = loop->next.ptr();
				indices.set(ti + 1, loop->vert->id);
			} else if (face->vertcount == 4) {
				Ref<BMeshLoop> loop = face->loop;
				int ti = indices.size();
				indices.resize(ti + 6);
				indices.set(ti + 0, loop->vert->id); loop = loop->next;
				indices.set(ti + 2, loop->vert->id); loop = loop->next;
				indices.set(ti + 1, loop->vert->id);
				//loop = face->loop->next->next;
				indices.set(ti + 3, loop->vert->id); loop = loop->next;
				indices.set(ti + 5, loop->vert->id); loop = loop->next;
				indices.set(ti + 4, loop->vert->id);
			} else {
				WARN_PRINT("The current implementation of BMesh only supports faces with 3 or 4 vertices");
			}
		}

		arr.set(RS::ARRAY_VERTEX, points);
		arr.set(RS::ARRAY_NORMAL, normals);
		arr.set(RS::ARRAY_TEX_UV, uv1);
		arr.set(RS::ARRAY_TEX_UV2, uv2);
		arr.set(RS::ARRAY_TANGENT, tangents);
		arr.set(RS::ARRAY_INDEX, indices);

		arrays.append(arr);
	}
}

void BMesh::_request_update() {
	if (pending_update_request) {
		return;
	}
	_update();
}

void BMesh::_update() const {
	Array arrays;
	const_cast<BMesh*>(this)->_create_mesh_arrays(arrays);

	const_cast<BMesh*>(this)->clear_surfaces();

	size_t mc = mc = arrays.size();

	for (size_t mi = 0; mi < mc; ++mi) {
		Array arr = arrays[mi];

		PackedVector3Array points = arr[RS::ARRAY_VERTEX];
		Vector<int> indices = arr[RS::ARRAY_INDEX];

		if (flip_faces) {
			PackedVector3Array normals = arr[RS::ARRAY_NORMAL];
			if (normals.size()) {
				size_t nc = normals.size();
				Vector3* nw = normals.ptrw();
				for (size_t i = 0; i < nc; ++i) {
					// invert normals
					nw[i] = -nw[i];
				}
				arr[RS::ARRAY_NORMAL] = normals;
			}
			if (indices.size()) {
				size_t ic = indices.size();
				int* ni = indices.ptrw();
				for (size_t i = 0; i < ic; i += 3) {
					SWAP(ni[i + 0], ni[i + 1]);
				}
				arr[RS::ARRAY_INDEX] = indices;
			}
		}

		if (!points.empty()) {
			const_cast<BMesh*>(this)->add_surface_from_arrays(primitive_type, arr);
		}
	}

	pending_update_request = false;

	clear_cache();
	const_cast<BMesh *>(this)->emit_changed();
}

Vector<Face3> BMesh::collect_faces() const {
	Vector<Face3> collected;
	size_t faces_count = faces.size();
	size_t collected_count = faces_count;
	collected.resize(collected_count);

	for (size_t fi = 0, ti = 0; fi < faces_count; ++fi) {
		if (ti >= collected_count) {
			// low-ball number of triangles from number of faces remaining
			collected_count += faces_count - fi;
			collected.resize(collected_count);
		}
		Ref<BMeshLoop> loop = faces[fi]->loop;
		collected.write[ti].vertex[0] = loop->vert->point; loop = loop->next;
		collected.write[ti].vertex[1] = loop->vert->point; loop = loop->next;
		collected.write[ti].vertex[2] = loop->vert->point;
		++ti;
		if (faces[fi]->vertcount == 4) {
			if (ti >= collected_count) {
				// if we get here, it should be
				// just an extra face to add
				collected_count++;
				collected.resize(collected_count);
			}
			collected.write[ti].vertex[0] = loop->vert->point; loop = loop->next;
			collected.write[ti].vertex[1] = loop->vert->point; loop = loop->next;
			collected.write[ti].vertex[2] = loop->vert->point;
			++ti;
		}
	}

	return collected;
}

inline void BMesh::set_vertex(int p_idx, Vector3 const & p_vec) {
	vertices.write[p_idx]->point = p_vec;
	pending_update_request = true;
}

inline void BMesh::set_vertex_attribute(int p_idx, String const & p_attr, Variant const & p_value) {
	vertices.write[p_idx]->attributes[p_attr] = p_value;
	pending_update_request = true;
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

	pending_update_request = true;

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

	pending_update_request = true;

	return f;
}

BMesh::BMesh() : ArrayMesh() {
	flip_faces = false;
	primitive_type = Mesh::PRIMITIVE_TRIANGLES;
	pending_update_request = false;
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

size_t BMeshEdge::fill_lines_vector3_array(PackedVector3Array & p_array, size_t p_offset) const {
	p_array.write[p_offset + 0] = vert1->point;
	p_array.write[p_offset + 1] = vert2->point;
	return 2;
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

size_t BMeshLoop::get_loop_edges_count() const {
	BMeshLoop const* loop = this;
	size_t edges = 0;
	do {
		edges++;
		loop = loop->next.ptr();
	} while (loop != this);
	return edges;
}

size_t BMeshLoop::fill_lines_vector3_array(PackedVector3Array & p_array, size_t p_offset) const {
	BMeshLoop const* loop = this;
	size_t vertices = 0;
	do {
		p_array.write[p_offset + vertices + 0] = loop->edge->vert1->point;
		p_array.write[p_offset + vertices + 1] = loop->edge->vert2->point;
		vertices+=2;
		loop = loop->next.ptr();
	} while (loop != this);
	return vertices;
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

size_t BMeshFace::fill_triangles_vector3_array(PackedVector3Array & p_array, size_t p_offset) const {
	if (vertcount == 3) {
		BMeshLoop const* iloop = loop.ptr();
		p_array.write[p_offset + 0] = iloop->vert->point; iloop = iloop->next.ptr();
		p_array.write[p_offset + 2] = iloop->vert->point; iloop = iloop->next.ptr();
		p_array.write[p_offset + 1] = iloop->vert->point;
		return 3;
	} else if (vertcount == 4) {
		BMeshLoop const* iloop = loop.ptr();
		p_array.write[p_offset + 0] = iloop->vert->point; iloop = iloop->next.ptr();
		p_array.write[p_offset + 2] = iloop->vert->point; iloop = iloop->next.ptr();
		p_array.write[p_offset + 1] = iloop->vert->point;
		p_array.write[p_offset + 3] = iloop->vert->point; iloop = iloop->next.ptr();
		p_array.write[p_offset + 5] = iloop->vert->point; iloop = iloop->next.ptr();
		p_array.write[p_offset + 4] = iloop->vert->point;
		return 6;
	}
	return 0;
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
	surface_set_material(0, mat);
}

inline Ref<Material> BMesh::get_material() const {
	if (pending_update_request) { _update(); }
	return surface_get_material(0);
}

int BMesh::get_surface_count() const {
	if (pending_update_request) { _update(); }
	return ArrayMesh::get_surface_count();
}

int BMesh::surface_get_array_len(int surface) const {
	if (pending_update_request) { _update(); }
	return ArrayMesh::surface_get_array_len(surface);
}

int BMesh::surface_get_array_index_len(int surface) const {
	if (pending_update_request) { _update(); }
	return ArrayMesh::surface_get_array_index_len(surface);
}

Array BMesh::surface_get_arrays(int surface) const {
	if (pending_update_request) { _update(); }
	return ArrayMesh::surface_get_arrays(surface);
}

Array BMesh::surface_get_blend_shape_arrays(int p_surface) const {
	if (pending_update_request) { _update(); }
	return ArrayMesh::surface_get_blend_shape_arrays(p_surface);
}

Dictionary BMesh::surface_get_lods(int surface) const {
	if (pending_update_request) { _update(); }
	return ArrayMesh::surface_get_lods(surface);
}

uint32_t BMesh::surface_get_format(int surface) const {
	if (pending_update_request) { _update(); }
	return ArrayMesh::surface_get_format(surface);
}

Mesh::PrimitiveType BMesh::surface_get_primitive_type(int surface) const {
	if (pending_update_request) { _update(); }
	return ArrayMesh::surface_get_primitive_type(surface);
}


Ref<Material> BMesh::surface_get_material(int surface) const {
	if (pending_update_request) { _update(); }
	return ArrayMesh::surface_get_material(surface);
}

int BMesh::get_blend_shape_count() const {
	if (pending_update_request) { _update(); }
	return ArrayMesh::get_blend_shape_count();
}

StringName BMesh::get_blend_shape_name(int p_index) const {
	if (pending_update_request) { _update(); }
	return ArrayMesh::get_blend_shape_name(p_index);
}

AABB BMesh::get_aabb() const {
	if (pending_update_request) { _update(); }
	return ArrayMesh::get_aabb();
}

RID BMesh::get_rid() const {
	if (pending_update_request) { _update(); }
	return ArrayMesh::get_rid();
}

size_t BMesh::get_num_tris() const {
	size_t count = 0;
	for (size_t i = 0, c = faces.size(); i < c; ++i) {
		count += faces[i]->get_triangles_count();
	}
	return count;
}

size_t BMesh::generate_tris(PackedVector3Array & p_array, size_t p_offset) const {
	size_t vert_count = 0;
	for (size_t i = 0, c = faces.size(); i < c; ++i) {

		vert_count += faces[i]->fill_triangles_vector3_array(p_array, p_offset + vert_count);
	}
	return vert_count;
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
		l->edge->loop = Ref<BMeshLoop>();
	} else {
		l->radial_prev->radial_next = l->radial_next;
		l->radial_next->radial_prev = l->radial_prev;
		if (l->edge->loop == l) {
			l->edge->loop = l->radial_next;
		}
	}

	l->next = Ref<BMeshLoop>();
	l->prev = Ref<BMeshLoop>();

	loops.erase(l);
	pending_update_request = true;
}

void BMesh::remove_face(Ref<BMeshFace> f) {
	Ref<BMeshLoop> l = f->loop;
	Ref<BMeshLoop> nextL;
	while (nextL != f->loop) {
		nextL = l->next;
		l->face = Ref<BMeshFace>();
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
