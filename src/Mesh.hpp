#pragma once

#include <glm/vec3.hpp>
#include "Primitive.hpp"

#include <cstdint>
#include <list>

struct Mesh {
  std::vector<Primitive> primitives;
};
