#pragma once

#include <glm/glm.hpp>
#include <cameras.hpp>


 class CameraControllerInterface {
    protected:
        GLFWwindow *m_pWindow = nullptr;
        glm::vec3 m_worldUpAxis;
        float m_fSpeed;

        // Current camera
        Camera m_camera;

       // Input event state
        bool m_MiddleButtonPressed = false;
        bool m_LeftButtonPressed = false;
        bool m_RightButtonPressed = false;
        glm::dvec2 m_LastCursorPosition = glm::dvec2();


    protected:

        //[[nodiscard]] glm::dvec2 computeCursorDelta();

    public:

        explicit CameraControllerInterface(GLFWwindow *window, const glm::vec3 &worldUpAxis = glm::vec3(0, 1, 0), float speed = 1.f);

        virtual ~CameraControllerInterface() = default;

        void setCamera(const Camera &camera);

        const Camera &getCamera() const;

        virtual bool update(GLfloat elapsedTime) = 0;

        void setSpeed(float speed) { m_fSpeed = speed; }

        float getSpeed() const { return m_fSpeed; }

        void increaseSpeed(float delta){
            m_fSpeed += delta;
            m_fSpeed = glm::max(m_fSpeed, 0.f);
        }
};