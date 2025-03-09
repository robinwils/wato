#pragma once

#include <reactphysics3d/reactphysics3d.h>

#include <glm/ext/vector_float3.hpp>

#include "reactphysics3d/mathematics/Matrix3x3.h"
#include "reactphysics3d/mathematics/Vector3.h"

namespace rp3d = reactphysics3d;

#define RP3D_VEC3(v)  (rp3d::Vector3(v.x, v.y, v.z))
#define RP3D_MAT33(m) ()
