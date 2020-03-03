#include "cameras.hpp"
#include "glfw.hpp"

#include <iostream>

// Good reference here to map camera movements to lookAt calls
// http://learnwebgl.brown37.net/07_cameras/camera_movement.html

using namespace glm;

struct ViewFrame
{
  vec3 left;
  vec3 up;
  vec3 front;
  vec3 eye;

  ViewFrame(vec3 l, vec3 u, vec3 f, vec3 e) : left(l), up(u), front(f), eye(e)
  {
  }
};

ViewFrame fromViewToWorldMatrix(const mat4 &viewToWorldMatrix)
{
  return ViewFrame{-vec3(viewToWorldMatrix[0]), vec3(viewToWorldMatrix[1]),
      -vec3(viewToWorldMatrix[2]), vec3(viewToWorldMatrix[3])};
}

Camera::Camera(glm::vec3 e, glm::vec3 c, glm::vec3 u) :
        eye(e), center(c), up(u) {
  const glm::vec3 front = this->center - this->eye;
  const glm::vec3 left = cross(this->up, front);
  
  assert(left != glm::vec3(0));
  
  this->up = normalize(cross(front, left));
};

