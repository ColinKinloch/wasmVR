#include "VR.hpp"

#include <glm/detail/func_trigonometric.hpp>

using namespace emscripten;
using namespace glm;

bool vr_init() {
  return true;
}

void vr_set_node_from_state(Node &node, val state) {
  if(state["orientation"]["x"].as<bool>()) {
    node.orientation.x = state["orientation"]["x"].as<double>();
    node.orientation.y = state["orientation"]["y"].as<double>();
    node.orientation.z = state["orientation"]["z"].as<double>();
    node.orientation.w = state["orientation"]["w"].as<double>();
  }
  
  if(state["position"]["x"].as<bool>()) {
    node.position.x = state["x"].as<double>();
    node.position.y = state["y"].as<double>();
    node.position.z = state["z"].as<double>();
  }
}

void vr_set_eye_from_webvr(VREye &eye, val parameters) {
  if(parameters.as<bool>()) {
    eye.position = {
      parameters["eyeTranslation"]["x"].as<float>(),
      parameters["eyeTranslation"]["y"].as<float>(),
      parameters["eyeTranslation"]["z"].as<float>(),
    };
    eye.rectangle = {
      {
        parameters["renderRect"]["x"].as<int16_t>(),
        parameters["renderRect"]["y"].as<int16_t>(),
      },
      {
        parameters["renderRect"]["width"].as<int16_t>(),
        parameters["renderRect"]["height"].as<int16_t>(),
      }
    };
    eye.fov = {
      radians(parameters["currentFieldOfView"]["leftDegrees"].as<float>()),
      radians(parameters["currentFieldOfView"]["rightDegrees"].as<float>()),
      radians(parameters["currentFieldOfView"]["upDegrees"].as<float>()),
      radians(parameters["currentFieldOfView"]["downDegrees"].as<float>())
    };
  }
}
