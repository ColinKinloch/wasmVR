#pragma once

struct RendererContext {
  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context;
  
  GLuint framebuffer;
  GLuint texture_colour;
  GLuint texture_depth;
  
  GLuint program;
  
  GLint position_attribute;
  GLint normal_attribute;
  GLint colour_attribute;
  
  GLint normal_matrix_uniform;
  GLint model_view_projection_matrix_uniform;
};
