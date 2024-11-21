#include "Camera.h"

#include <iostream>


Camera::Camera(Window* pWindow)
    : Camera(pWindow, glm::vec3(0), glm::vec3(0, 0, -1), glm::vec3(0))
{
}

Camera::Camera(Window* pWindow, const glm::vec3& pos, const glm::vec3& forward, glm::vec3 startTarget)
    : m_position(pos)
    , m_forward(glm::normalize(forward)) // Ensure the forward vector is normalized
    , m_pWindow(pWindow)
    , m_target(startTarget)
    , m_moveSpeed(0.03f)
    , m_lookSpeed(0.0015f)
    , m_zoomSpeed(0.001f)
    , m_panSpeed(2.5f)
{
}

void Camera::setUserInteraction(bool enabled)
{
    m_userInteraction = enabled; // Enable or disable user input handling
}

glm::vec3 Camera::cameraPos() const
{
    return m_position; // Return the current camera position
}

void Camera::setCameraPos(glm::vec3 newPos){
    m_position = newPos; // Set the camera position
}

glm::vec3 Camera::forward() const {
    return m_forward; // Return the forward direction of the camera
}

glm::mat4 Camera::viewMatrix() const
{
    return glm::lookAt(m_position, m_position + m_forward, m_up); // Compute the view matrix
}

void Camera::rotateX(float angle)
{   
    // Rotate the camera around the horizontal axis (X)
    const glm::vec3 horAxis = glm::cross(s_yAxis, m_forward);

    m_forward = glm::normalize(glm::angleAxis(angle, horAxis) * m_forward);
    m_up = glm::normalize(glm::cross(m_forward, horAxis));
}

void Camera::rotateY(float angle)
{   
    // Rotate the camera around the vertical axis (Y)
    const glm::vec3 horAxis = glm::cross(s_yAxis, m_forward);

    m_forward = glm::normalize(glm::angleAxis(angle, s_yAxis) * m_forward);
    m_up = glm::normalize(glm::cross(m_forward, horAxis));
}

void Camera::rotateAround(const glm::vec3& center, float angle, const glm::vec3& axis) {
    //Calculate the camera’s position relative to the center (target position)
    glm::vec3 relativePosition = m_position - center;

    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(angle), axis);
    glm::vec3 rotatedPosition = glm::vec3(rotationMatrix * glm::vec4(relativePosition, 1.0f));

    //Update the camera’s position by adding the center back to the rotated relative position
    m_position = center + rotatedPosition;
    m_forward = glm::normalize(center - m_position);

    //Recompute the `m_up` vector to maintain proper orientation
    const glm::vec3 horAxis = glm::cross(s_yAxis, m_forward);
    m_up = glm::normalize(glm::cross(m_forward, horAxis));
}


void Camera::zoom(float zoomFactor) {
    m_position += glm::normalize(m_position) * zoomFactor * m_zoomSpeed;
}

void Camera::pan(const glm::vec2& offset) {
    // Calculate right and forward directions for panning
    glm::vec3 right = glm::normalize(glm::cross(m_target - m_position, m_up));
    glm::vec3 forward = glm::normalize(glm::cross(m_up, right));     

    // Pan the camera along X and Z, while keeping Y constant
    m_position += right * offset.x * m_panSpeed;
    m_position += forward * offset.y * m_panSpeed;

    // Update target position to remain centered
    m_target += right * offset.x * m_panSpeed;
    m_target += forward * offset.y * m_panSpeed;
}
void Camera::useBirdsEyeControls() {
    const glm::dvec2 cursorPos = m_pWindow->getCursorPos();
    const glm::vec2 delta = m_lookSpeed * glm::vec2(m_prevCursorPos - cursorPos);
    m_prevCursorPos = cursorPos;

    m_pWindow->registerScrollCallback(
        [this](const glm::vec2& offset) {
            zoom(offset.y);
        });
    if(m_pWindow->isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
        if (delta.x != 0.0f && delta.y != 0.0f)
            pan(glm::vec2(-delta.x, -delta.y));
    }
}

void Camera::movePosition(const glm::vec3 translation){
    m_position += translation;
    m_target += translation;
}

void Camera::useThirdPersonControls() {

}
void Camera::useDefaultControls() {
    const glm::vec3 right = glm::normalize(glm::cross(m_forward, m_up));
    if (m_pWindow->isKeyPressed(GLFW_KEY_A))
        m_position -= m_moveSpeed * right;
    if (m_pWindow->isKeyPressed(GLFW_KEY_D))
        m_position += m_moveSpeed * right;
    if (m_pWindow->isKeyPressed(GLFW_KEY_W))
        m_position += m_moveSpeed * m_forward;
    if (m_pWindow->isKeyPressed(GLFW_KEY_S))
        m_position -= m_moveSpeed * m_forward;
    if (m_pWindow->isKeyPressed(GLFW_KEY_SPACE))
        m_position += m_moveSpeed * m_up;
    if (m_pWindow->isKeyPressed(GLFW_KEY_C))
        m_position -= m_moveSpeed * m_up;

    const glm::dvec2 cursorPos = m_pWindow->getCursorPos();
    const glm::vec2 delta = m_lookSpeed * glm::vec2(m_prevCursorPos - cursorPos);
    m_prevCursorPos = cursorPos;

    if (m_pWindow->isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
        if (delta.x != 0.0f)
            rotateY(delta.x);
        if (delta.y != 0.0f)
            rotateX(delta.y);
    }
}

void Camera::updateInput(ViewMode view_mode)
{
    if (m_userInteraction) {
        if(view_mode == DEFAULT)
            useDefaultControls();
        else if (view_mode == BIRDS_EYE_VIEW)
            useBirdsEyeControls();
        else if (view_mode == THIRD_PERSON_VIEW)
            useThirdPersonControls();

    } else {
        m_prevCursorPos = m_pWindow->getCursorPos();
    }
}