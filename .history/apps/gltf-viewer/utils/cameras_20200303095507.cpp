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

bool FirstPersonCameraController::update(float elapsedTime)
{
  if (glfwGetMouseButton(m_pWindow, GLFW_MOUSE_BUTTON_LEFT) &&
      !m_LeftButtonPressed) {
    m_LeftButtonPressed = true;
    glfwGetCursorPos(
        m_pWindow, &m_LastCursorPosition.x, &m_LastCursorPosition.y);
  } else if (!glfwGetMouseButton(m_pWindow, GLFW_MOUSE_BUTTON_LEFT) &&
             m_LeftButtonPressed) {
    m_LeftButtonPressed = false;
  }

  const auto cursorDelta = ([&]() {
    if (m_LeftButtonPressed) {
      dvec2 cursorPosition;
      glfwGetCursorPos(m_pWindow, &cursorPosition.x, &cursorPosition.y);
      const auto delta = cursorPosition - m_LastCursorPosition;
      m_LastCursorPosition = cursorPosition;
      return delta;
    }
    return dvec2(0);
  })();

  float truckLeft = 0.f;
  float pedestalUp = 0.f;
  float dollyIn = 0.f;
  float rollRightAngle = 0.f;

  if (glfwGetKey(m_pWindow, GLFW_KEY_W)) {
    dollyIn += m_fSpeed * elapsedTime;
  }

  // Truck left
  if (glfwGetKey(m_pWindow, GLFW_KEY_A)) {
    truckLeft += m_fSpeed * elapsedTime;
  }

  // Pedestal up
  if (glfwGetKey(m_pWindow, GLFW_KEY_UP)) {
    pedestalUp += m_fSpeed * elapsedTime;
  }

  // Dolly out
  if (glfwGetKey(m_pWindow, GLFW_KEY_S)) {
    dollyIn -= m_fSpeed * elapsedTime;
  }

  // Truck right
  if (glfwGetKey(m_pWindow, GLFW_KEY_D)) {
    truckLeft -= m_fSpeed * elapsedTime;
  }

  // Pedestal down
  if (glfwGetKey(m_pWindow, GLFW_KEY_DOWN)) {
    pedestalUp -= m_fSpeed * elapsedTime;
  }

  if (glfwGetKey(m_pWindow, GLFW_KEY_Q)) {
    rollRightAngle -= 0.001f;
  }
  if (glfwGetKey(m_pWindow, GLFW_KEY_E)) {
    rollRightAngle += 0.001f;
  }

  // cursor going right, so minus because we want pan left angle:
  const float panLeftAngle = -0.01f * float(cursorDelta.x);
  const float tiltDownAngle = 0.01f * float(cursorDelta.y);

  const auto hasMoved = truckLeft || pedestalUp || dollyIn || panLeftAngle ||
                        tiltDownAngle || rollRightAngle;
  if (!hasMoved) {
    return false;
  }

  m_camera.moveLocal(truckLeft, pedestalUp, dollyIn);
  m_camera.rotateLocal(rollRightAngle, tiltDownAngle, 0.f);
  m_camera.rotateWorld(panLeftAngle, m_worldUpAxis);

  return true;
}

bool TrackballCameraController::update(float elapsedTime) 
{
  if (glfwGetMouseButton(m_pWindow, GLFW_MOUSE_BUTTON_MIDDLE) &&
      !m_MiddleButtonPressed) {
    m_MiddleButtonPressed = true;
    glfwGetCursorPos(
        m_pWindow, &m_LastCursorPosition.x, &m_LastCursorPosition.y);
  } else if (!glfwGetMouseButton(m_pWindow, GLFW_MOUSE_BUTTON_MIDDLE) &&
             m_MiddleButtonPressed) {
    m_MiddleButtonPressed = false;
  }

  const auto cursorDelta = ([&]() {
    if (m_MiddleButtonPressed) {
      dvec2 cursorPosition;
      glfwGetCursorPos(m_pWindow, &cursorPosition.x, &cursorPosition.y);
      const auto delta = cursorPosition - m_LastCursorPosition;
      m_LastCursorPosition = cursorPosition;
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
    m_camera.moveLocal(truckLeft, pedestalUp, 0.f);
    return true;
  }

  if (glfwGetKey(m_pWindow, GLFW_KEY_LEFT_CONTROL)) {
   auto mouseOffset = 0.01f * float(cursorDelta.x);
    if (mouseOffset == 0.f) {
      return false;
    }
    /** Zoom/ Unzoom **/
    std::cout << "Zoom and Unzoom with Control" << std::endl;
    
    // todo Implement zoom
    const glm::vec3 viewVector = m_camera.center() - m_camera.eye();
    const float l = glm::length(viewVector);
    if (mouseOffset > 0.f) {
        mouseOffset = glm::min(mouseOffset, l - 1e-4f);
    }

    const glm::vec3 front = viewVector / l;
    const glm::vec3 translationVector = mouseOffset * front;

    // Update camera with new eye position
    const glm::vec3 newEye = m_camera.eye() + translationVector;
    m_camera = Camera(newEye, m_camera.center(), m_worldUpAxis);

    return true;
  }

  /** else juste rotate **/
 // Rotate around target
  const float longitudeAngle = 0.01f * float(cursorDelta.y); // Vertical angle
  const float latitudeAngle = -0.01f * float(cursorDelta.x); // Horizontal angle
  if (!(longitudeAngle || latitudeAngle)) {
      return false;
  }

  const glm::vec3 depthAxis = m_camera.getEye() - camera.center();
  const glm::mat4 latitudeRotationMatrix = rotate(glm::mat4(1), latitudeAngle, m_worldUpAxis);
  const glm::vec3 horizontalAxis = m_camera.getLeft();
  const glm::mat4 rotationMatrix = rotate(latitudeRotationMatrix, longitudeAngle, horizontalAxis);
  const glm::vec3 rotatedDepthAxis = glm::vec3(rotationMatrix * glm::vec4(depthAxis, 0));
  const glm::vec3 newEye = m_camera.center() + rotatedDepthAxis;

  m_camera = Camera(newEye, m_camera.center(), m_worldUpAxis);


  

  return true;



  // // cursor going right, so minus because we want pan left angle:
  // const float panLeftAngle = -0.01f * float(cursorDelta.x);
  // const float tiltDownAngle = 0.01f * float(cursorDelta.y);

  // const auto hasMoved = truckLeft || pedestalUp || dollyIn || panLeftAngle ||
  //                       tiltDownAngle || rollRightAngle;
  // if (!hasMoved) {
  //   return false;
  // }

  // m_camera.moveLocal(truckLeft, pedestalUp, dollyIn);
  // m_camera.rotateLocal(rollRightAngle, tiltDownAngle, 0.f);
  // m_camera.rotateWorld(panLeftAngle, m_worldUpAxis);

}
