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
        m_eye(e), m_center(c), m_up(u) {
  const auto front = m_center - m_eye;
  const auto left = cross(m_up, front);
  assert(left != glm::vec3(0));
  m_up = normalize(cross(front, left));
};

glm::mat4 Camera::getViewMatrix() const { 
  return glm::lookAt(m_eye, m_center, m_up); 
};

  // Move the camera along its left axis.
void Camera::truckLeft(float offset){
  const auto front = m_center - m_eye;
  const auto left = normalize(cross(m_up, front));
  const auto translationVector = offset * left;
  m_eye += translationVector;
  m_center += translationVector;
};

void Camera::pedestalUp(float offset){
  const auto translationVector = offset * m_up;
  m_eye += translationVector;
  m_center += translationVector;
};

void Camera::dollyIn(float offset){
  const auto front = normalize(m_center - m_eye);
  const auto translationVector = offset * front;
  m_eye += translationVector;
  m_center += translationVector;
};

void Camera::moveLocal(float truckLeftOffset, float pedestalUpOffset, float dollyIn){
  const auto front = normalize(m_center - m_eye);
  const auto left = normalize(cross(m_up, front));
  const auto translationVector =
      truckLeftOffset * left + pedestalUpOffset * m_up + dollyIn * front;
  m_eye += translationVector;
  m_center += translationVector;
}

void Camera::rollRight(float radians){
  const auto front = m_center - m_eye;
  const auto rollMatrix = glm::rotate(glm::mat4(1), radians, front);

  m_up = glm::vec3(rollMatrix * glm::vec4(m_up, 0.f));
}
