#include "Camera.h"
#include "util.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>

Camera::Camera()
{
    position = glm::vec3(0.0f, 0.0f, 0.0f);
    pitch = 0;
    yaw = 0;
    roll = 0;
    m_zNear = 0.1f;
    m_zFar  = 10000.f;
}


Camera::Camera(const glm::vec3& pos_in, float pitch_in, float yaw_in, float roll_in)
{
    position = pos_in;
    pitch = pitch_in;
    yaw = yaw_in;
    roll = roll_in;
    m_zNear = 0.1f;
    m_zFar  = 10000.f;
}

const glm::vec3 Camera::getPosition() const
{
    return position;
}


const glm::vec3 Camera::right() const
{
    return glm::vec3(glm::row(rotation(), 0));
}

const glm::vec3 Camera::up() const
{
    return glm::vec3(glm::row(rotation(), 1));
}

const glm::vec3 Camera::front() const
{
    return -glm::vec3(glm::row(rotation(), 2));
}

const glm::uvec2 Camera::getViewport()
{
    return viewport;
}

void Camera::setFov(float fov_in)
{
    fov = fov_in;
}

void Camera::setViewport(glm::uvec2 dims)
{
    viewport = dims;
}

void Camera::move(const glm::vec3& direction, float deltaTime, float speed)
{
    position += deltaTime * speed * direction;
}


void Camera::moveXDir(float direction, float deltaTime, float speed)
{
    position += deltaTime * speed * direction * right();
}


void Camera::moveYDir(float direction, float deltaTime, float speed)
{
    position += deltaTime * speed * direction * up();
}

void Camera::moveZDir(float direction, float deltaTime, float speed)
{
    position += deltaTime * speed * direction * front();
}

void Camera::rotateWithMouseInput(double xPos, double yPos)
{
    //Calculate the offset from given mouse positions.
    double xOffset = xPos - lastX;
    //GLFW returns mouse coordinates relative to top-left corner of the screen
    //X increases in right direction
    //Y increases in bottom direction
    double yOffset = yPos - lastY;
    lastX = xPos;
    lastY = yPos;

    xOffset *= 0.001;
    yOffset *= 0.001;
    
    yaw -= xOffset;
    pitch -= yOffset;
}

void Camera::rotateX(float angle)
{
    pitch += angle;
}

void Camera::rotateY(float angle)
{
    yaw += angle;
}

void Camera::rotateZ(float angle)
{
    roll += angle;
}

void Camera::updateLastMousePos(double x, double y)
{
    lastX = x;
    lastY = y;
}

glm::mat4 Camera::translation() const
{
    return glm::translate(glm::mat4(1.0f), -position);
}

glm::mat4 Camera::rotation() const
{
    return glm::transpose(rotationFromEuler(glm::vec3(pitch, yaw, roll)));
}

glm::mat4 Camera::view() const
{
    return rotation() * translation();
}

glm::mat4 Camera::projection() const
{
    float aspectRatio = static_cast<float>(viewport.x) / viewport.y;
    glm::mat4 projectionMat = glm::perspectiveRH_ZO(glm::radians(fov), aspectRatio, m_zNear, m_zFar);
    // Vulkan's coordinate system has an inverted y wrt OpenGL
    projectionMat[1][1] *= -1;
    return projectionMat;
}

float Camera::zNear() const
{
    return m_zNear;
}

float Camera::zFar() const
{
    return m_zFar;
}
