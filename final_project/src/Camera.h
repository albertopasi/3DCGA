#ifndef CAMERA_H
#define CAMERA_H

#include <framework/window.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec2.hpp>

// Enumeration to define different camera view modes
enum ViewMode {
    DEFAULT,
    THIRD_PERSON_VIEW,
    BIRDS_EYE_VIEW
};

class Camera {
public:
    // Constructors: default and parameterized to initialize position, forward direction, and target
    Camera(Window* pWindow);
    Camera(Window* pWindow, const glm::vec3& position, const glm::vec3& forward, glm::vec3 startTarget);

    // Enable or disable user interaction with the camera
    void setUserInteraction(bool enabled);

    void updateInput(ViewMode);

    void useDefaultControls();
    void movePosition(const glm::vec3 translation);

    // Getters for camera position, view matrix, and forward direction
    glm::vec3 cameraPos() const;
    glm::mat4 viewMatrix() const;
    glm::vec3 forward() const;

    void setCameraPos(glm::vec3 newPos);
    void useBirdsEyeControls();
    void useThirdPersonControls();
    void rotateX(float angle);
    void rotateY(float angle);
    void rotateAround(const glm::vec3& center, float angle, const glm::vec3& axis);

private:
    void zoom(float zoomFactor);
    void pan(const glm::vec2& offset);


private:
    static constexpr glm::vec3 s_yAxis { 0, 1, 0 };

    // Camera position, forward direction, and up vector
    glm::vec3 m_position { 0 };
    glm::vec3 m_forward { 0, 0, -1 };
    glm::vec3 m_up { 0, 1, 0 };

    Window* m_pWindow; // Pointer to the window
    bool m_userInteraction { true }; // Flag to enable or disable user interaction
    glm::dvec2 m_prevCursorPos { 0 }; // Previous cursor position for calculating cursor delta
    glm::vec3 m_target; // Target point, used in some view modes

    // Movement and control speeds
    float m_moveSpeed;
    float m_lookSpeed;
    float m_zoomSpeed;
    float m_panSpeed;
};

#endif