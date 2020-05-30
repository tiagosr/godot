#pragma once

#include "bmesh.h"
#include "scene/3d/visual_instance_3d.h"

class BMeshInstance3D : public GeometryInstance3D {
	GDCLASS(BMeshInstance3D, GeometryInstance3D);

public:
	//
	//Ref<Mesh> get_mesh();
};
