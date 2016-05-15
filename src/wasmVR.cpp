#define TAU (2 * M_PI)

#include <iostream>
#include <vector>
#include <random>
#include <cstdint>
#include <fstream>
#include <chrono>

#include <emscripten.h>
#include <emscripten/bind.h>
#include <html5.h>

//#include <EGL/egl.h>
//#include <EGL/eglext.h>
#include <GLES3/gl3.h>

#include <AL/al.h>

#include <vorbis/vorbisfile.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <json.hpp>

#include "Node.hpp"
#include "Camera.hpp"

#include "Audio.hpp"
#include "Renderer/Renderer.hpp"
#include "VR.hpp"

using namespace std;
using namespace emscripten;
using namespace nlohmann;

float r,g,b;

val navigator = val::global("navigator");
val screen = val::global("screen");
val window = val::global("window");

val hmvr_device = val::undefined();
val position_device = val::undefined();

VREye left_eye_params;
VREye right_eye_params;

bool touching;

GLuint triangle_buffer;
GLuint post_program;

chrono::system_clock::time_point start_time;
chrono::system_clock::time_point previous_time;
chrono::system_clock::time_point touch_time;

RendererContext renderer_context;
AudioContext audio_context;

float perf_pixel_ratio = 0.5f;
float device_pixel_ratio = 1.f;
uint16_t width = 100;
uint16_t height = 100;
bool fullscreen = false;

Node* root;
Node* viewer;
Node* listener;
Node* eye_left;
Node* eye_right;
vector<Node*> cubes;

bool init_vr() {
  //TODO Make better when emscripten promises exists
  return val::global("Module").call<bool>("getVRDevices");
  //val::module_property("").call<bool>("getVRDevices");
  //val::module_property("getVRDevices").call<bool>("");
}


void request_fullscreen() {
  EmscriptenFullscreenStrategy strategy = {
    EMSCRIPTEN_FULLSCREEN_SCALE_DEFAULT,
    EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_HIDEF,
    EMSCRIPTEN_FULLSCREEN_FILTERING_NEAREST
  };
  emscripten_request_fullscreen_strategy("canvas", true, &strategy);
  emscripten_request_pointerlock("canvas", true);
  // TODO: Figure this out
  screen["orientation"].call<void>("lock", string("landscape-primary"));
  EM_ASM( dispatchEvent(new Event('resize')); );
}

int on_double_click(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData) {
  cout<<"Double Click"<<endl;
  request_fullscreen();
  return 0;
}

void start_touch() {
  touching = true;
  navigator.call<void>("vibrate", 10);
}
void stop_touch() {
  touching = false;
}

int on_mousedown(int eventType, const EmscriptenMouseEvent *event, void *userData) {
  start_touch();
  return true;
}
int on_touchstart(int eventType, const EmscriptenTouchEvent *event, void *userData) {
  cout<<"touch"<<endl;
  start_touch();
  return 0;
}
int on_mouseup(int eventType, const EmscriptenMouseEvent *event, void *userData) {
  stop_touch();
  return true;
}
int on_touchend(int eventType, const EmscriptenTouchEvent *event, void *userData) {
  cout<<"untouch"<<endl;
  stop_touch();
  return 0;
}

int on_mousemove(int eventType, const EmscriptenMouseEvent *event, void *userData) {
  viewer->orientation *= glm::quat(glm::vec3(event->movementY, event->movementX, 0) * -0.005f);
//  viewer->orientation *= glm::quat(glm::vec3(1, 1, 0));
  return 0;
}

void poll_size() {
  int cw, ch, cf;
  emscripten_get_canvas_size(&cw, &ch, &cf);
  width = cw;
  height = ch;
  fullscreen = cf;
  device_pixel_ratio = emscripten_get_device_pixel_ratio();
}

void resize(int w, int h, float r) {
  emscripten_set_canvas_size(r * w, r * h);
  /*eglQuerySurface(display, surface, EGL_WIDTH, &width);
  eglQuerySurface(display, surface, EGL_HEIGHT, &height);*/
  poll_size();
  
  glBindTexture(GL_TEXTURE_2D, renderer_context.texture_colour);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  glBindTexture(GL_TEXTURE_2D, renderer_context.texture_depth);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
  
  cout<<"Resize: "<<w<<'x'<<h<<'x'<<device_pixel_ratio<<'='<<width<<'x'<<height<<endl;
  glViewport(0, 0, width, height);
}

int on_resize(int eventType, const EmscriptenUiEvent *event, void *userData) {
  if(fullscreen) {
    resize(screen["width"].as<float>(), screen["height"].as<float>(), perf_pixel_ratio * device_pixel_ratio);
  } else {
    resize(event->windowInnerWidth, event->windowInnerHeight, perf_pixel_ratio * device_pixel_ratio);
  }
  return 0;
}
int on_fullscreenchange(int eventType, const EmscriptenFullscreenChangeEvent *event, void *userData) {
  cout<<"Fullscreen: ";
  if(event->isFullscreen) {
    cout<<"on"<<endl;
    resize(screen["width"].as<float>(), screen["height"].as<float>(), perf_pixel_ratio * device_pixel_ratio);
  } else {
    resize(event->screenWidth, event->screenHeight, perf_pixel_ratio * device_pixel_ratio);
  }
  
  return 0;
}

void render_recursive(glm::mat4 &camera_matrix, Node *node) {
  glm::mat4 model_view_matrix = node->get_transform();
  glm::mat3 normal_matrix = glm::inverseTranspose(glm::mat3(model_view_matrix));
  glm::mat4 model_view_projection_matrix = camera_matrix * model_view_matrix;
  
  if(node->mesh) {
    glUseProgram(renderer_context.program);
    //for(int i = 0; i < node->mesh->primitives.size(); ++i) {
    //for(auto it = node->mesh->primitives.begin(); it != node->mesh->primitives.end(); ++it); 
    for(auto primitive: node->mesh->primitives) {
      using vertex_type = decltype(primitive.vertices)::value_type;
      //using index_type = decltype(primitive.indices)::value_type;
      
      //Setup techniques
      //GLuint program = programs[primitive.material.technique.program];
      //glUseProgram(program);
      glUniformMatrix3fv(renderer_context.normal_matrix_uniform, 1, GL_FALSE, glm::value_ptr(normal_matrix));
      glUniformMatrix4fv(renderer_context.model_view_projection_matrix_uniform, 1, GL_FALSE, glm::value_ptr(model_view_projection_matrix));
      
      glBindVertexArray(primitive.vertex_array);
      glBindBuffer(GL_ARRAY_BUFFER, primitive.vertex_buffer);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, primitive.index_buffer);
      
      glEnableVertexAttribArray(renderer_context.position_attribute);
      glEnableVertexAttribArray(renderer_context.normal_attribute);
      glEnableVertexAttribArray(renderer_context.colour_attribute);
      
      if(primitive.indices.size())
        glDrawElements(primitive.mode, primitive.indices.size(), GL_UNSIGNED_BYTE, nullptr);
      else
        glDrawArrays(primitive.mode, 0, sizeof(vertex_type) * primitive.vertices.size());
      
      glDisableVertexAttribArray(renderer_context.position_attribute);
      glDisableVertexAttribArray(renderer_context.normal_attribute);
      glDisableVertexAttribArray(renderer_context.colour_attribute);
    }
  }
  
  //OpenAL Sources
  for(auto audio_source: node->audio_sources) {
    ALint state;
    alGetSourcei(audio_source, AL_SOURCE_STATE, &state);
    if(state == AL_PLAYING) {
      glm::vec3 source_direction = glm::eulerAngles(glm::quat_cast(model_view_matrix));
      glm::vec3 translation = model_view_matrix * glm::vec4(0, 0, 0, 1);
      
      alSourcefv(audio_source, AL_POSITION, glm::value_ptr(translation));
      alSourcefv(audio_source, AL_DIRECTION, glm::value_ptr(source_direction));
    }
  }
  for(auto child: node->children) {
    render_recursive(camera_matrix, child.second);
  }
}

void render_left() {
  VRRect r = left_eye_params.rectangle;
  float p = perf_pixel_ratio;
  glViewport(r.position.x * p, r.position.y * p, r.size.x * p, r.size.y * p);
  //glViewport(0, 0, width / 2, height);
  glm::mat4 view = eye_left->camera->projection * glm::inverse(eye_right->get_transform());
  render_recursive(view, root);
}
void render_right() {
  VRRect r = right_eye_params.rectangle;
  float p = perf_pixel_ratio;
  glViewport(r.position.x * p, r.position.y * p, r.size.x * p, r.size.y * p);
//  glViewport(width / 2, 0, width / 2, height);
  glm::mat4 view = eye_right->camera->projection * glm::inverse(eye_right->get_transform());
  render_recursive(view, root);
}
void render_screen() {
  glViewport(0, 0, width, height);
  glm::mat4 view = eye_left->camera->projection * glm::inverse(eye_right->get_transform());
  render_recursive(view, root);
}

void render() {
  renderer_make_current(renderer_context);
  glClearColor(r, g, b, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  glm::quat orientation;
  glm::vec3 translation;
  translation = listener->get_transform() * glm::vec4(0, 0, 0, 1);
  orientation = glm::quat_cast(listener->get_transform());
  glm::vec3 fw = glm::rotate(orientation, glm::vec3(0, 0, -1));
  glm::vec3 up = glm::rotate(orientation, glm::vec3(0, 1, 0));
  float listener_orientation[6] = {
    fw.x, fw.y, fw.z,
    up.x, up.y, up.z
  };
  alListenerfv(AL_POSITION, glm::value_ptr(translation));
  //cout<<translation.z - cubes[2]->position.z<<endl;
  //alListenerfv(AL_VELOCITY, glm::value_ptr(viewer->position));
  alListenerfv(AL_ORIENTATION, listener_orientation);
  
  if(hmvr_device.as<bool>()) {
    render_left();
    render_right();
  }
  else {
    render_screen();
  }
} 

void move_viewer(Node* viewer) {
  
  if(position_device.as<bool>()) {
    val state = position_device.call<val>("getState");
    if(state["orientation"]["x"].as<bool>()) {
      viewer->orientation.x = state["orientation"]["x"].as<double>();
      viewer->orientation.y = state["orientation"]["y"].as<double>();
      viewer->orientation.z = state["orientation"]["z"].as<double>();
      viewer->orientation.w = state["orientation"]["w"].as<double>();
    }
    
    if(state["position"]["x"].as<bool>()) {
      viewer->position.x = state["x"].as<double>();
      viewer->position.y = state["y"].as<double>();
      viewer->position.z = state["z"].as<double>();
    }
  }
}

void game_loop(chrono::system_clock::time_point current_time, chrono::system_clock::duration frame_length) {
  if(!touching) touch_time = current_time;
  
  uint32_t t = chrono::duration_cast<chrono::milliseconds>(current_time - start_time).count();
  //TODO Cannot go under 15fps
  uint16_t dt = chrono::duration_cast<chrono::microseconds>(frame_length).count();
  //TODO Touch duration lasts 1 minute
  uint16_t touch_duration = chrono::duration_cast<chrono::milliseconds>(current_time - touch_time).count();
  
  r = (1 + sin(0.004 * t)) / 8;
  g = (1 + sin(0.001 * t)) / 8;
  b = (1 + sin(0.008 * t)) / 8;
  
  cubes[0]->orientation = glm::quat(glm::vec3(0.004 * t, 0, 0.0001 * t));
  cubes[0]->scaling = glm::vec3(20);
  cubes[1]->scaling = glm::vec3(5);
  cubes[2]->scaling = glm::vec3(40);
  
//  move_viewer(viewer);
  
  if(position_device.as<bool>())
    vr_set_node_from_state(*viewer, position_device.call<val>("getState"));
  
  cubes[0]->position.z = 30 * sin(0.001 * t);
  cubes[0]->position.x = -50;
  cubes[1]->position.x = 15;
  cubes[1]->position.y = 5 * sin(0.002 * t);
  cubes[2]->position.x = 100 * sin(0.0008 * t);
  cubes[2]->position.z = 100 * cos(0.0008 * t);
  cubes[3]->position.x = 65 + 150 * sin(0.0008 * t);
  cubes[3]->position.z = 150 * cos(0.0002 * t);
  cubes[3]->position.y = 25 + 5 * sin(0.002 * t);
//  cubes[3]->position.y = 25;
  
  // Projections
  float near = 0.01f;
  float far = 10000.f;
  if(hmvr_device.as<bool>()) {
    vr_set_eye_from_webvr(left_eye_params, hmvr_device.call<val>("getEyeParameters", string("left")));
    vr_set_eye_from_webvr(right_eye_params, hmvr_device.call<val>("getEyeParameters", string("right")));
    eye_left->position = left_eye_params.position;
    eye_right->position = right_eye_params.position;
    VRFOV f = left_eye_params.fov;
    float tt = glm::tan(f.top);
    float bt = glm::tan(f.bottom);
    float lt = glm::tan(f.left);
    float rt = glm::tan(f.right);
    float xs = 2.f / (lt + rt);
    float ys = 2.f / (tt + bt);
    glm::mat4 p = {
      xs, 0, 0, 0,
      0, ys, 0, 0,
      -((lt - rt) * xs * 0.5), ((tt - bt) * ys * 0.5), -(near + far) / (far - near), -1,
      0, 0, -(2.0 * far * near) / (far - near), 0
    };
    eye_left->camera->projection = p;
    
    f = right_eye_params.fov;
    tt = glm::tan(f.top);
    bt = glm::tan(f.bottom);
    lt = glm::tan(f.left);
    rt = glm::tan(f.right);
    xs = 2.f / (lt + rt);
    ys = 2.f / (tt + bt);
    p = {
      xs, 0, 0, 0,
      0, ys, 0, 0,
      -((lt - rt) * xs * 0.5), ((tt - bt) * ys * 0.5), -(near + far) / (far - near), -1,
      0, 0, -(2.0 * far * near) / (far - near), 0
    };
    eye_right->camera->projection = p;
  } else {
    float ar = (float)width / height;
    eye_left->camera->projection = glm::perspective(1.5f, ar, near, far);
  }
  
  if(touching) {
    viewer->position += glm::rotate(viewer->orientation, glm::vec3(0, 0, -0.5));
  }
  //if(touch_duration > 3000) {
  //  window["location"].call<void>("reload");
  //}
}

void main_loop() {
  chrono::system_clock::time_point current_time = chrono::system_clock::now();
  chrono::system_clock::duration frame_length = current_time - previous_time;

  game_loop(current_time, frame_length);
  //glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxb);

  vector<GLenum> targets = {
    GL_COLOR_ATTACHMENT0
  };
  //glBindFramebuffer(GL_FRAMEBUFFER, renderer_context.framebuffer);
  //glDrawBuffers(targets.size(), targets.data());
  //render();
  
  glBindFramebuffer(GL_FRAMEBUFFER, NULL);
  //glDrawBuffers(0, nullptr);
  render();
  previous_time = current_time;
}

ALuint create_audio_buffer(string path) {
  //ifstream file(path);
  OggVorbis_File vf;
  FILE *file;
  int current_section = 0;
  
  if(ov_fopen(path.c_str(), &vf) < 0) {
    cerr<<"Failed to open ogg file"<<endl;
    return 0;
  }
  
  ALint format;
  vorbis_info *vi = ov_info(&vf, -1);
  switch(vi->channels) {
    case 1: {
      format = AL_FORMAT_MONO16;
      break;
    }
    case 2: {
      format = AL_FORMAT_STEREO16;
      break;
    }
  }
  
  vector<int8_t> samples;
  
  int8_t buf[4096];
  
  size_t accum = 0;
  while(true) {
    size_t ret = ov_read(&vf, (char*)buf, sizeof(buf), 0, 2, 1, nullptr);
    if(ret <= 0) break;
    samples.resize(accum + ret);
    memcpy(&samples[accum], buf, ret);
    accum += ret;
  }
  
  GLuint error;
  ALuint buffer;
  alGenBuffers(1, &buffer);
  alBufferData(buffer, format, samples.data(), samples.size() * sizeof(decltype(samples)::value_type), vi->rate);
  if((error = alGetError()) != AL_NO_ERROR)
  {
    cout<<"Error loading:"<<error<<endl;
  }
  return buffer;
}

GLuint create_shader(GLenum type, string path) {
  GLuint shader = glCreateShader(type);
  
  ifstream file(path);
  string shader_source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

  const GLchar* shader_sources[] = { shader_source.c_str() };
  const GLint shader_source_sizes[] = { static_cast<GLint>(shader_source.length()) };
  glShaderSource(shader, 1, shader_sources, shader_source_sizes);
  glCompileShader(shader);

  GLint isCompiled = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
  if(isCompiled == GL_FALSE) {
    GLint maxLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
    GLchar errorLog[maxLength];
    glGetShaderInfoLog(shader, maxLength, &maxLength, errorLog);
    cerr<<errorLog<<endl;
    glDeleteShader(shader);
  }
  
  return shader;
}

GLuint create_program(map<GLenum, string> sources) {
  GLuint program = glCreateProgram();
  vector<GLuint> shaders;
  for(auto source: sources) shaders.push_back(create_shader(source.first, source.second));
  for(auto shader: shaders) glAttachShader(program, shader);
  glLinkProgram(program);
  for(auto shader:shaders) glDeleteShader(shader);
  return program;
}

void start() {
  emscripten_set_main_loop(main_loop, 0, true);
  start_time = previous_time = touch_time = chrono::system_clock::now();
  
}

void set_vr_devices(val hmvrDevice, val positionDevice) {
  hmvr_device = hmvrDevice;
  position_device = positionDevice;
}
void set_orientation() {
  poll_size();
  cout<<"GOGOGOGOG"<<endl;
}

EMSCRIPTEN_BINDINGS() {
  emscripten::function("setVRDevices", &set_vr_devices);
  emscripten::function("setOrientation", &set_orientation);
}

int main() {
  if(!renderer_init(renderer_context)) return EXIT_FAILURE;
  if(!init_vr()) cerr<<"Not VR"<<endl;
  if(!audio_init(audio_context)) return EXIT_FAILURE;
  poll_size();

  // OpenGL
  renderer_make_current(renderer_context);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);
  
  glClearColor(0,0,0.3,1.0);
  glClearDepthf(1.0);
  
  glGenFramebuffers(1, &renderer_context.framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, renderer_context.framebuffer);
  
  glGenTextures(1, &renderer_context.texture_colour);
  glGenTextures(1, &renderer_context.texture_depth);
  
  glBindTexture(GL_TEXTURE_2D, renderer_context.texture_colour);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderer_context.texture_colour, 0);
  
  glBindTexture(GL_TEXTURE_2D, renderer_context.texture_depth);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, renderer_context.texture_depth, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, NULL);
  
  renderer_context.program = create_program({
    {GL_VERTEX_SHADER, "data/simple.glslv"},
    {GL_FRAGMENT_SHADER, "data/simple.glslf"}
  });
  glUseProgram(renderer_context.program);
  
  //Post processing
  vector<uint8_t> triangle = {
    0, 0,
    4, 0,
    0, 4
  };
  glGenBuffers(1, &triangle_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, triangle_buffer);
  glBufferData(GL_ARRAY_BUFFER, triangle.size() * sizeof(uint8_t), triangle.data(), GL_STATIC_DRAW);
  
  post_program = create_program({
    {GL_VERTEX_SHADER, "data/post.glslv"},
    {GL_FRAGMENT_SHADER, "data/post.glslf"}
  });
  
  root = new Node("Root");
  viewer = new Node("Viewer");
  
  Node* cursor;
  cursor = new Node("Cursor");
  cursor->position.z = -25;
  cursor->scaling = glm::vec3(0.5);
  viewer->add(cursor);
  root->add(viewer);
  viewer->position.y = 20;
  eye_left = new Node("Left Eye");
  eye_left->camera = new Camera();
  eye_right = new Node("Right Eye");
  eye_right->camera = new Camera();
  viewer->add(eye_left);
  viewer->add(eye_right);
  listener = new Node("Listener");
  viewer->add(listener);
  cubes.push_back(new Node("Cube 0"));
  cubes.push_back(new Node("Cube 1"));
  cubes.push_back(new Node("Cube 2"));
  cubes.push_back(new Node("Cube 3"));
  cubes.push_back(new Node("Ocean"));
  cubes[4]->position.z = 200;
  //cubes[2]->add(viewer);
  //cubes.push_back(new Node("Cube 4"));
  Mesh* mesh = new Mesh();
  auto primitive = mesh->primitives.emplace(mesh->primitives.begin());
  primitive->indices = {
    0, 1, 2,
    0, 2, 3,
    0, 3, 1,
    3, 2, 1
  };
  float angle = TAU / 3;
  glm::vec4 top = {0, 1, 0, 1};
  glm::vec4 front = glm::rotateX(top, angle);
  primitive->vertices = {
    {
      top,
      {0, 0, 0},
      {1, 0, 1, 1}
    },
    {
      front,//{0.5, -0.5, 0, 1},
      {0, 0, 0},
      {0, 1, 0, 1}
    },
    {
      glm::rotateY(front, angle),
      {0, 0, 0},
      {0, 1, 1, 1}
    },
    {
      glm::rotateY(front, -angle),
      {0, 0, 0},
      {1, 1, 1, 1}
    }
  };
  primitive->mode = GL_TRIANGLES;
  for(auto&& cube: cubes ) {
    cube->mesh = mesh;
    root->add(cube);
  }
  cursor->mesh = mesh;
  
  renderer_context.position_attribute = glGetAttribLocation(renderer_context.program, "position");
  renderer_context.normal_attribute = glGetAttribLocation(renderer_context.program, "normal");
  renderer_context.colour_attribute = glGetAttribLocation(renderer_context.program, "colour");
  
  renderer_context.normal_matrix_uniform = glGetUniformLocation(renderer_context.program, "normalMatrix");
  renderer_context.model_view_projection_matrix_uniform = glGetUniformLocation(renderer_context.program, "modelViewProjectionMatrix");

  glGenVertexArrays(1, &primitive->vertex_array);
  glBindVertexArray(primitive->vertex_array);
  
  using vertex_type = decltype(primitive->vertices)::value_type;
  using index_type = decltype(primitive->indices)::value_type;

  glGenBuffers(1, &primitive->vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, primitive->vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, primitive->vertices.size() * sizeof(vertex_type), primitive->vertices.data(), GL_STATIC_DRAW);

  glGenBuffers(1, &primitive->index_buffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, primitive->index_buffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, primitive->indices.size() * sizeof(index_type), primitive->indices.data(), GL_STATIC_DRAW);
  
  glEnableVertexAttribArray(renderer_context.position_attribute);
  glEnableVertexAttribArray(renderer_context.normal_attribute);
  glEnableVertexAttribArray(renderer_context.colour_attribute);
  
  size_t size = sizeof(vertex_type);
  glVertexAttribPointer(renderer_context.position_attribute, 4, GL_FLOAT, GL_FALSE, size, (void*) offsetof(vertex_type, position));
  glVertexAttribPointer(renderer_context.normal_attribute, 3, GL_FLOAT, GL_FALSE, size, (void*) offsetof(vertex_type, normal));
  glVertexAttribPointer(renderer_context.colour_attribute, 4, GL_FLOAT, GL_FALSE, size, (void*) offsetof(vertex_type, colour));
  
  glDisableVertexAttribArray(renderer_context.position_attribute);
  glDisableVertexAttribArray(renderer_context.normal_attribute);
  glDisableVertexAttribArray(renderer_context.colour_attribute);
  
  // OpenAL
  audio_make_current(audio_context);
  
  alListenerf(AL_GAIN, 0.05);
  
  //alDistanceModel(AL_LINEAR_DISTANCE);
  alDistanceModel(AL_EXPONENT_DISTANCE);
  //alDopplerFactor(0.5);
  //alDopplerVelocity(1000.25);
  //alSpeedOfSound(150);
  
  ALuint bird_buffer, ocean_buffer;
  bird_buffer = create_audio_buffer("data/bs.ogg");
  ocean_buffer = create_audio_buffer("data/cow.ogg");
  
  ALuint bird_source, ocean_source;
  std::vector<ALuint> audio_sources;
  audio_sources.resize(3);
  
  alGenSources(audio_sources.size(), audio_sources.data());
  for(auto audio_source: audio_sources) {
    alSourcef(audio_source, AL_REFERENCE_DISTANCE, 0.1f);
    alSourcei(audio_source, AL_SOURCE_RELATIVE, AL_FALSE);
    alSourcei(audio_source, AL_SOURCE_TYPE, AL_STATIC);
  }
  bird_source = audio_sources[1];
  ocean_source = audio_sources[2];
  alSourcei(bird_source, AL_BUFFER, bird_buffer);
  alSourcei(ocean_source, AL_BUFFER, ocean_buffer);
  
  alSourcef(bird_source, AL_GAIN, 0.25);
  alSourcef(ocean_source, AL_GAIN, 0.5);
  
  alSourcei(bird_source, AL_LOOPING, AL_TRUE);
  alSourcei(ocean_source, AL_LOOPING, AL_TRUE);
  alSourcePlay(bird_source);
  alSourcePlay(ocean_source);
  cubes[3]->audio_sources.push_back(bird_source);
  cubes[4]->audio_sources.push_back(ocean_source);
  
  
  emscripten_set_resize_callback(nullptr, nullptr, true, on_resize);
  emscripten_set_fullscreenchange_callback("canvas", nullptr, true, on_fullscreenchange);
  emscripten_set_dblclick_callback("canvas", nullptr, true, on_double_click);
  
  //Activate events
  emscripten_set_mousedown_callback("canvas", nullptr, true, on_mousedown);
  emscripten_set_mouseup_callback("canvas", nullptr, true, on_mouseup);
  emscripten_set_touchstart_callback("canvas", nullptr, true, on_touchstart);
  emscripten_set_touchend_callback("canvas", nullptr, true, on_touchend);
  
  //Move events
  emscripten_set_mousemove_callback("canvas", nullptr, true, on_mousemove);
  
  width = window["innerWidth"].as<int>();
  height = window["innerHeight"].as<int>();
  emscripten_set_canvas_size(width, height);
  start();
  
  return EXIT_SUCCESS;
}
