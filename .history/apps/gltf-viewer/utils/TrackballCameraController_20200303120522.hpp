#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cameraControllerInterface.hpp>

struct GLFWwindow;

// todo Blender like camera
class TrackballCameraController : public CameraControllerInterface{
  private:
  float m_fSpeed;
public:
  TrackballCameraController(GLFWwindow *window, float speed = 1.f,
    const glm::vec3 &worldUpAxis = glm::vec3(0, 1, 0)) :
    CameraControllerInterface(window, worldUpAxis, speed),
    m_fSpeed(speed){
  }

  // Update the view matrix based on input events and elapsed time
  // Return true if the view matrix has been modified
  bool update(float elapsedTime);

  // Get the view matrix
  const Camera &getCamera() const { return m_camera; }

  void setCamera(const Camera &camera) { m_camera = camera; }



};