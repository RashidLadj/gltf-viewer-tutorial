#include <utils/camera/ICameraController.hpp>
    
ICameraController::ICameraController(GLFWwindow *t_window, const glm::vec3 &t_worldUpAxis) :
        window(t_window), worldUpAxis(t_worldUpAxis),
        camera(glm::vec3(0), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0)) {
};

void ICameraController::setCamera(const Camera &camera) {
    this->camera = camera;
};

const Camera &ICameraController::getCamera() const {
    return this->camera;
};

void ICameraController::setWorldUpAxis(const glm::vec3 &worldUpAxis) {
    this->worldUpAxis = worldUpAxis;
};

const glm::vec3 &ICameraController::getWorldUpAxis() const {
    return this->worldUpAxis;
};

glm::dvec2 ICameraController::computeCursorDelta() {
    glm::dvec2 cursorPosition;
    glfwGetCursorPos(this->window, &cursorPosition.x, &cursorPosition.y);
    const auto delta = cursorPosition - this->lastCursorPosition;
    this->lastCursorPosition = cursorPosition;
    return delta;
};
