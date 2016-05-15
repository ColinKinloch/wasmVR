#pragma once

#include "Node.hpp"

#include <emscripten/bind.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

struct VRContext {
  emscripten::val hmvr_device;
  emscripten::val position_device;
};

struct VRFOV {
  float left;
  float right;
  float bottom;
  float top;
};

struct VRRect {
  glm::ivec2 position;
  glm::ivec2 size;
};

struct VREye {
  glm::vec3 position;
  VRRect rectangle;
  VRFOV fov;
};

bool vr_init();

void vr_set_node_from_state(Node &node, emscripten::val state);
void vr_set_eye_from_webvr(VREye &eye, emscripten::val parameters);
