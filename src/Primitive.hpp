#pragma once

#include <vector>
#include <cstdint>
#include <GLES3/gl3.h>

#include "Vertex.hpp"

struct Primitive {
  std::vector<uint8_t> indices;
  std::vector<Vertex> vertices;
  
  //Skeleton?

  GLuint mode;

  GLuint vertex_buffer;
  GLuint index_buffer;
  GLuint vertex_array;
};
