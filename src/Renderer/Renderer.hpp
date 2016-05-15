#pragma once

#include <GLES3/gl3.h>
#include <html5.h>

#ifdef EMSCRIPTEN
#include "EmRenderer.hpp"
#else
#include "EGLRenderer.hpp"
#endif

bool renderer_init(RendererContext &renderer_context);
bool renderer_make_current(RendererContext &renderer_context);

// void render_recursive(glm::mat4 &camera_matrix, Node *node);
