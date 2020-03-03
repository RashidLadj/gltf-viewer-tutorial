#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cameraControllerInterface.hpp>

struct GLFWwindow;

class FirstPersonCameraController : public CameraControllerInterface{
  private:
    float m_fSpeed = 0.f;

  public:
    explicit FirstPersonCameraController(GLFWwindow *window, float speed = 0.01f,
        const glm::vec3 &worldUpAxis = glm::vec3(0, 1, 0)) : ICameraController(window, worldUpAxis), m_fSpeed(speed){};

    explicit FirstPersonCameraController(GLF  Wwindow *window, GLfloat speed,
                                                             const glm::vec3 &worldUpAxis) :
            ICameraController(window, worldUpAxis), m_fSpeed(speed) {
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