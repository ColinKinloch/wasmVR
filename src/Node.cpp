#include "Node.hpp"

#include <iostream>

using namespace std;

Node::Node(string Name):
  parent(nullptr),
  name(Name),
  position(),
  orientation(),
  scaling(1),
  transform(),
  transform_outdated(true),
  mesh(nullptr)
{}

bool Node::add(Node* node) {
  if(node->parent) {
    cerr<<"Cannot add \"" + node->name + "\" to \"" + name + "\", already parented to \"" + node->parent->name + "\""<<endl;
    return false;
  }
  children.insert({node->name, node});
  node->set_parent(this);
  return true;
}

void Node::set_parent(Node* node) {
  parent = node;
  transform_outdated = true;
}

glm::mat4 Node::get_transform() {
  if(transform_outdated) {
    transform = glm::translate(glm::mat4(), position);
    transform *= glm::mat4_cast(orientation);
    transform = glm::scale(transform, scaling);
    transform_outdated = true;
    if(parent) {
      transform = parent->get_transform() * transform;
    }
  }
  return transform;
}
//DEP
bool Node::path_to(Node *node, std::vector<Node*> *path) {
  if(node == this) {
    path->push_back(node);
    return true;
  }
  for(auto& child: children) {
    if(child.second->path_to(node, path)) {
      path->push_back(this);
      return true;
    }
  }
  return false;
}
