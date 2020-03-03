#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct GLFWwindow;

// Camera defined by an eye position, a center position and an up vector
class Camera{
  public:
    Camera() = default;

    Camera(glm::vec3 e, glm::vec3 c, glm::vec3 u);  

    glm::mat4 getViewMatrix() const;
    // Move the camera along its left axis.
    void truckLeft(float offset);

    void pedestalUp(float offset);

    void dollyIn(float offset);

    void moveLocal(float truckLeftOffset, float pedestalUpOffset, float dollyIn);

    void rollRight(float radians);


    void tiltDown(float radians);

    void panLeft(float radians);
  
    // All angles in radians
    void rotateLocal(float rollRight, float tiltDown, float panLeft);

    // Rotate around a world axis but keep the same position
    void rotateWorld(float radians, const glm::vec3 &axis);

    const glm::vec3 eye() const;

    const glm::vec3 center() const;

    const glm::vec3 up() const;

    const glm::vec3 front(bool normalize = true) const;

    const glm::vec3 left(bool normalize = true) const;

  private:
    glm::vec3 m_eye;
    glm::vec3 m_center;
    glm::vec3 m_up;
};