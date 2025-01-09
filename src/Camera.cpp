#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>

Camera::Camera()
{
    position = glm::vec3(0.0f, 0.0f, 0.0f);
    orientation = glm::quat(0.0, 0.0, 0.0, 1.0);
}


Camera::Camera(const glm::vec3& pos_in, const glm::quat& quat_in)
{
    position = pos_in;
    orientation = quat_in;
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

void Camera::setFov(float fov_in)
{
    fov = fov_in;
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

void Camera::rotate(float angle, glm::vec3& axis)
{
    glm::quat rot = glm::normalize(glm::angleAxis(angle, axis));

    orientation = orientation * rot;
}

void Camera::rotateWithMouseInput(double xPos, double yPos)
{
    //Calculate the offset from given mouse positions.
    double xOffset = xPos - lastX;
    //GLFW returns mouse coordinates relative to top-left corner of the screen
    //X increases in right direction
    //Y increases in bottom direction
    double yOffset = lastY - yPos; //We need to subtract in reverse order since y coordinates ranges in reverse order we want 
    lastX = xPos;
    lastY = yPos;

    xOffset *= 0.001;
    yOffset *= 0.001;

    //TODO: Try to convert them into actual angles, you are just assigning offsets as angles.
    //Note that as we move the mouse to the right, we actually decrement the yaw angle so we need to subtract.
    
    yaw(xOffset);
    pitch(-yOffset);

    //yaw = glm::mod(yaw - xOffset, 360.0);
    //pitch += yOffset;
}

void Camera::pitch(float angle)
{
    glm::quat rot = glm::normalize(glm::angleAxis(angle, right()));

    orientation = orientation * rot;
}

void Camera::yaw(float angle)

{
    glm::quat rot = glm::normalize(glm::angleAxis(angle, up()));

    orientation = orientation * rot;
}

void Camera::roll(float angle)
{
    glm::quat rot = glm::normalize(glm::angleAxis(angle, front()));

    orientation = orientation * rot;
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
    return glm::toMat4(orientation);
}

glm::mat4 Camera::view() const
{
    return rotation() * translation();
}

glm::mat4 Camera::projection(float aspectRatio) const
{
    glm::mat4 projectionMat = glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 10000.f);
    // Vulkan's coordinate system has an inverted y wrt OpenGL
    projectionMat[1][1] *= -1;
    return projectionMat;
}
