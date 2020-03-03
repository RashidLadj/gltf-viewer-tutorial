#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cameraControllerInterface.hpp>

struct GLFWwindow;

// todo Blender like camera
class TrackballCameraController : public CameraControllerInterface{
  private:
  float m_fspeed;
public:
  TrackballCameraController(GLFWwindow *window, float speed = 1.f,
      const glm::vec3 &worldUpAxis = glm::vec3(0, 1, 0)) :
      CameraControllerInterface(window, worldUpAxis),
      m_fSpeed(speed)
  {
  }

  // Controller attributes, if put in a GUI, should be adapted
  void setSpeed(float speed) { m_fSpeed = speed; }

  float getSpeed() const { return m_fSpeed; }

  void increaseSpeed(float delta)
  {
    m_fSpeed += delta;
    m_fSpeed = glm::max(m_fSpeed, 0.f);
  }

  const glm::vec3 &getWorldUpAxis() const { return m_worldUpAxis; }

  void setWorldUpAxis(const glm::vec3 &worldUpAxis)
  {
    m_worldUpAxis = worldUpAxis;
  }

  // Update the view matrix based on input events and elapsed time
  // Return true if the view matrix has been modified
  bool update(float elapsedTime);

  // Get the view matrix
  const Camera &getCamera() const { return m_camera; }

  void setCamera(const Camera &camera) { m_camera = camera; }



};