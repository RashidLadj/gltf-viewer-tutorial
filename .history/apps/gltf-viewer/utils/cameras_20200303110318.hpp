#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct GLFWwindow;

// Camera defined by an eye position, a center position and an up vector
class Camera
{
public:
  Camera() = default;

  Camera(glm::vec3 e, glm::vec3 c, glm::vec3 u);  

  glm::mat4 getViewMatrix() const;
  // Move the camera along its left axis.
  void truckLeft(float offset);

  void pedestalUp(float offset)
  {
    const auto translationVector = offset * m_up;
    m_eye += translationVector;
    m_center += translationVector;
  }

  void dollyIn(float offset)
  {
    const auto front = normalize(m_center - m_eye);
    const auto translationVector = offset * front;
    m_eye += translationVector;
    m_center += translationVector;
  }

  void moveLocal(float truckLeftOffset, float pedestalUpOffset, float dollyIn)
  {
    const auto front = normalize(m_center - m_eye);
    const auto left = normalize(cross(m_up, front));
    const auto translationVector =
        truckLeftOffset * left + pedestalUpOffset * m_up + dollyIn * front;
    m_eye += translationVector;
    m_center += translationVector;
  }

  void rollRight(float radians)
  {
    const auto front = m_center - m_eye;
    const auto rollMatrix = glm::rotate(glm::mat4(1), radians, front);

    m_up = glm::vec3(rollMatrix * glm::vec4(m_up, 0.f));
  }

  void tiltDown(float radians)
  {
    const auto front = m_center - m_eye;
    const auto left = cross(m_up, front);
    const auto tiltMatrix = glm::rotate(glm::mat4(1), radians, left);

    const auto newFront = glm::vec3(tiltMatrix * glm::vec4(front, 0.f));
    m_center = m_eye + newFront;
    m_up = glm::vec3(tiltMatrix * glm::vec4(m_up, 0.f));
  }

  void panLeft(float radians)
  {
    const auto front = m_center - m_eye;
    const auto panMatrix = glm::rotate(glm::mat4(1), radians, m_up);

    const auto newFront = glm::vec3(panMatrix * glm::vec4(front, 0.f));
    m_center = m_eye + newFront;
  }

  // All angles in radians
  void rotateLocal(float rollRight, float tiltDown, float panLeft)
  {
    const auto front = m_center - m_eye;
    const auto rollMatrix = glm::rotate(glm::mat4(1), rollRight, front);

    m_up = glm::vec3(rollMatrix * glm::vec4(m_up, 0.f));

    const auto left = cross(m_up, front);

    const auto tiltMatrix = glm::rotate(glm::mat4(1), tiltDown, left);

    const auto newFront = glm::vec3(tiltMatrix * glm::vec4(front, 0.f));
    m_center = m_eye + newFront;

    m_up = glm::vec3(tiltMatrix * glm::vec4(m_up, 0.f));

    const auto panMatrix = glm::rotate(glm::mat4(1), panLeft, m_up);

    const auto newNewFront = glm::vec3(panMatrix * glm::vec4(newFront, 0.f));
    m_center = m_eye + newNewFront;
  }

  // Rotate around a world axis but keep the same position
  void rotateWorld(float radians, const glm::vec3 &axis)
  {
    const auto rotationMatrix = glm::rotate(glm::mat4(1), radians, axis);

    const auto front = m_center - m_eye;
    const auto newFront = glm::vec3(rotationMatrix * glm::vec4(front, 0));
    m_center = m_eye + newFront;
    m_up = glm::vec3(rotationMatrix * glm::vec4(m_up, 0));
  }

  const glm::vec3 eye() const { return m_eye; }

  const glm::vec3 center() const { return m_center; }

  const glm::vec3 up() const { return m_up; }

  const glm::vec3 front(bool normalize = true) const
  {
    const auto f = m_center - m_eye;
    return normalize ? glm::normalize(f) : f;
  }

  const glm::vec3 left(bool normalize = true) const
  {
    const auto f = front(false);
    const auto l = glm::cross(m_up, f);
    return normalize ? glm::normalize(l) : l;
  }

private:
  glm::vec3 m_eye;
  glm::vec3 m_center;
  glm::vec3 m_up;
};



