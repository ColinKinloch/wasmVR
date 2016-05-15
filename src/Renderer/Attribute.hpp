#pragma once

#include <string>

enum AttributeType {
  POSITION,
  NORMAL,
  TEXCOORD,
  COLOR,
  JOINT,
  JOINTMATRIX,
  WEIGHT
}

struct Attribute {
  std::string name;
};
