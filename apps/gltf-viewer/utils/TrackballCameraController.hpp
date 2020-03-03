#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <utils/cameraControllerInterface.hpp>
#include <utils/TrackballCameraController.hpp>
#include <utils/FirstPersonCameraController.hpp>

struct GLFWwindow;

// todo Blender like camera
class TrackballCameraController : public CameraControllerInterface{
  private:
  float m_fSpeed;
public:
  TrackballCameraController(GLFWwindow *window,
    const glm::vec3 &worldUpAxis = glm::vec3(0, 1, 0)) :
    CameraControllerInterface(window, worldUpAxis){
  }

  // Update the view matrix based on input events and elapsed time
  // Return true if the view matrix has been modified
  bool update(float elapsedTime) override ;




};