#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

class Camera
{
public:
    Camera();
    Camera(const glm::vec3& pos_in, const glm::quat& quat_in);

    //Getters
    const glm::vec3 getPosition() const;
    const glm::vec3 right() const;
    const glm::vec3 up() const;
    const glm::vec3 front() const; //gaze

    void setFov(float fov_in);

    //Transformations
    //Translation
    //Global move by a direction vector
    void move(const glm::vec3& direction, float deltaTime, float speed);
    //Camera relative movement
    //direction -1 or 1
    void moveXDir(float direction, float deltaTime, float speed);
    void moveYDir(float direction, float deltaTime, float speed);
    void moveZDir(float direction, float deltaTime, float speed);
    //Rotation
    void rotate(float angle, glm::vec3& axis);
    void rotateWithMouseInput(double xPos, double yPos);
    //Camera-Axis relative rotations
    void pitch(float angle);
    void yaw(float angle);
    void roll(float angle);

    void updateLastMousePos(double x, double y);

    //Matrices
    glm::mat4 translation() const;
    glm::mat4 rotation() const;
    glm::mat4 view() const;
    glm::mat4 projection(float aspectRatio) const;

private:
    glm::vec3 position;
    glm::quat orientation;
    float fov = 45.0f;
    //These are the last positions camera looks at (pixelwise)
    double lastX;
    double lastY;
};
