#pragma once

#include <glm/glm.hpp>
#include <cameras.hpp>



 class CameraControllerInterface {
    protected:
        GLFWwindow *m_pwindow = nullptr;
        Camera m_camera;

        glm::vec3 m_worldUpAxis;
        float m_fspeed

        glm::dvec2 lastCursorPosition = glm::dvec2();
        bool leftButtonPressed = false;
        bool middleButtonPressed = false;
        bool rightButtonPressed = false;

    protected:

        //[[nodiscard]] glm::dvec2 computeCursorDelta();

    public:

        explicit ICameraController(GLFWwindow *window, const glm::vec3 &worldUpAxis = glm::vec3(0, 1, 0));

        virtual ~ICameraController() = default;

        void setCamera(const Camera &camera);

        const Camera &getCamera() const;

        virtual bool update(GLfloat elapsedTime) = 0;
};