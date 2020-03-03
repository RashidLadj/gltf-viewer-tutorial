#pragma once

#include <glm/glm.hpp>
#include <cameras.hpp>


 class CameraControllerInterface {
    protected:
        GLFWwindow *m_pWindow = nullptr;
        glm::vec3 m_worldUpAxis;
        float m_fspeed = 0.f;

        // Current camera
        Camera m_camera;

       // Input event state
        bool m_MiddleButtonPressed = false;
        bool m_LeftButtonPressed = false;
        bool m_RightButtonPressed = false;
        glm::dvec2 lastCursorPosition = glm::dvec2();


    protected:

        //[[nodiscard]] glm::dvec2 computeCursorDelta();

    public:

        explicit ICameraController(GLFWwindow *window, const glm::vec3 &worldUpAxis = glm::vec3(0, 1, 0));

        virtual ~ICameraController() = default;

        void setCamera(const Camera &camera);

        const Camera &getCamera() const;

        virtual bool update(GLfloat elapsedTime) = 0;
};