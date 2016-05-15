#include "Renderer.hpp"

#include <iostream>

using namespace std;

bool renderer_init(RendererContext &renderer_context) {
  EmscriptenWebGLContextAttributes attributes;
  emscripten_webgl_init_context_attributes(&attributes);
  attributes.alpha = false;
  attributes.depth = true;
  attributes.antialias = false;
  attributes.majorVersion = 2;
  attributes.minorVersion = 0;
  attributes.enableExtensionsByDefault = true;
  renderer_context.context = emscripten_webgl_create_context("canvas", &attributes);
  return true;
}
bool renderer_make_current(RendererContext &renderer_context) {
  if (emscripten_webgl_make_context_current(renderer_context.context) == EMSCRIPTEN_RESULT_SUCCESS) return true;
  else return false;
}

//bool init_graphics() {
//  display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
//  EGLint maj, min;
//  if(!eglInitialize(display, &maj, &min)) {
//    cout<<"Failed to initialise GL."<<endl;
//    return false;
//  };
//  EGLint configCount;
//  eglGetConfigs(display, nullptr, 0, &configCount);
//  vector<EGLConfig> configs(configCount);
//  eglGetConfigs(display, configs.data(), configCount, &configCount);
//  eglChooseConfig(display, nullptr, &config, 1, &configCount);
//  
//  EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
//  surface = eglCreateWindowSurface(display, config, NULL, nullptr);
//  context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
//  return true;
//}
//bool renderer_make_current() {
//  return eglMakeCurrent(display, surface, surface, context);
//}
