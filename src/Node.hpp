#pragma once

#include <string>
#include <unordered_map>

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <AL/al.h>

#include "Mesh.hpp"
#include "Camera.hpp"

struct Node {
  Node(std::string Name = "Untitled");

  uint32_t id;
  
  glm::vec3 position;
  glm::quat orientation;
  glm::vec3 scaling;
  
  glm::mat4 transform;
  
  bool transform_outdated;
  
  std::string name;
  
  Node* parent;
  
  Mesh* mesh;
  Camera* camera;
  
  std::vector<ALuint> audio_sources;
  
  std::unordered_map<std::string, Node*> children;
  
  void set_parent(Node* node);
  
  bool add(Node* node);
  
  glm::mat4 get_transform();
  
  bool path_to(Node *node, std::vector<Node*> *path);
};
