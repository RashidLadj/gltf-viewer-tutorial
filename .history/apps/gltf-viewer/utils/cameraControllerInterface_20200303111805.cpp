#include <cameraControllerInterface.hpp>
    
CameraControllerInterface::CameraControllerInterface(GLFWwindow *t_window, const glm::vec3 &t_worldUpAxis) :
        m_pWindow(t_window), m_worldUpAxis(t_worldUpAxis),
        m_camera(glm::vec3(0), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0)) {
};

void CameraControllerInterface::setCamera(const Camera &camera) {
    this->camera = camera;
};

const Camera &CameraControllerInterface::getCamera() const {
    return this->camera;
};

void CameraControllerInterface::setWorldUpAxis(const glm::vec3 &worldUpAxis) {
    this->worldUpAxis = worldUpAxis;
};

const glm::vec3 &CameraControllerInterface::getWorldUpAxis() const {
    return this->worldUpAxis;
};

glm::dvec2 CameraControllerInterface::computeCursorDelta() {
    glm::dvec2 cursorPosition;
    glfwGetCursorPos(this->window, &cursorPosition.x, &cursorPosition.y);
    const auto delta = cursorPosition - this->lastCursorPosition;
    this->lastCursorPosition = cursorPosition;
    return delta;
};
