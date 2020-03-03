#include "TrackballCameraController.hpp"
#include "glfw.hpp"

#include <iostream>


bool TrackballCameraController::update(float elapsedTime) 
{
  if (glfwGetMouseButton(m_pWindow, GLFW_MOUSE_BUTTON_MIDDLE) &&
      !this->m_MiddleButtonPressed) {
    this->m_MiddleButtonPressed = true;
    glfwGetCursorPos(
        this->m_pWindow, &this->m_LastCursorPosition.x, &this->m_LastCursorPosition.y);
  } else if (!glfwGetMouseButton(m_pWindow, GLFW_MOUSE_BUTTON_MIDDLE) &&
             this->m_MiddleButtonPressed) {
    this->m_MiddleButtonPressed = false;
  }

  const auto cursorDelta = ([&]() {
    if (m_MiddleButtonPressed) {
      glm::dvec2 cursorPosition;
      glfwGetCursorPos(m_pWindow, &cursorPosition.x, &cursorPosition.y);
      const auto delta = cursorPosition - this->m_LastCursorPosition;
      this->m_LastCursorPosition = cursorPosition;
      return delta;
    }
    return dvec2(0);
  })();

  if (glfwGetKey(m_pWindow, GLFW_KEY_LEFT_SHIFT)) {
    float truckLeft = 0.01f * float(cursorDelta.x);
    float pedestalUp = 0.01f * float(cursorDelta.y);
    if (!(truckLeft || pedestalUp)) {
      std::cout << "khra" << std::endl;
      return false;
    }
     /** pan the camera (lateral movement orthogonal to the view direction) **/
    std::cout << "lateral movement orthogonal to the view direction" << std::endl;
    this->m_camera.moveLocal(truckLeft, pedestalUp, 0.f);
    return true;
  }

  if (glfwGetKey(m_pWindow, GLFW_KEY_LEFT_CONTROL)) {
   auto mouseOffset = 0.01f * float(cursorDelta.x);
    if (mouseOffset == 0.f) {
      return false;
    }
    /** Zoom/ Unzoom **/
    std::cout << "Zoom and Unzoom with Control" << std::endl;
    const glm::vec3 viewVector = this->m_camera.center() - this->m_camera.eye();
    const float viewVectorLength = glm::length(viewVector);
    if (mouseOffset > 0.f) {
        mouseOffset = glm::min(mouseOffset, viewVectorLength - 1e-4f);
    }

    const glm::vec3 front = viewVector / viewVectorLength;
    const glm::vec3 translationVector = mouseOffset * front;

    // Update camera with new eye position
    const glm::vec3 newEye = this->m_camera.eye() + translationVector;
    this->m_camera = Camera(newEye, this->m_camera.center(), this->m_worldUpAxis);

    return true;
  }

  /** else juste rotate **/
 // Rotate around target
  const float longitudeAngle = 0.01f * float(cursorDelta.y); // Vertical angle
  const float latitudeAngle = -0.01f * float(cursorDelta.x); // Horizontal angle
  if (!(longitudeAngle || latitudeAngle)) {
      return false;
  }

  const auto depthAxis = this->m_camera.eye() - this->m_camera.center();
  const auto latitudeRotationMatrix = rotate(glm::mat4(1), latitudeAngle, this->m_worldUpAxis);
  const auto horizontalAxis = m_camera.left();
  const auto rotationMatrix = rotate(latitudeRotationMatrix, longitudeAngle, horizontalAxis);
  const auto rotatedDepthAxis = glm::vec3(rotationMatrix * glm::vec4(depthAxis, 0));
  const auto newEye = this->m_camera.center() + rotatedDepthAxis;

  this->m_camera = Camera(newEye, this->m_camera.center(), this->m_worldUpAxis);

  return true;
}
