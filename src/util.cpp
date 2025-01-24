#include "util.h"

#include <glm/gtx/quaternion.hpp>

glm::mat4 rotationFromEuler(const glm::vec3 &eulerAngles) {
    glm::quat orientation = glm::quat(1.0, 0.0, 0.0, 0.0);

    orientation = orientation * glm::normalize(glm::angleAxis(eulerAngles.y, glm::vec3(0.0, 1.0, 0.0)));
    orientation = orientation * glm::normalize(glm::angleAxis(eulerAngles.x, glm::vec3(1.0, 0.0, 0.0)));
    orientation = orientation * glm::normalize(glm::angleAxis(eulerAngles.z, glm::vec3(0.0, 0.0, -1.0)));

    return glm::toMat4(orientation);
}

glm::mat4 makeTransform(const glm::vec3 &pos, const glm::vec3 &scale, const glm::vec3 &eulerAngles) {
    return glm::scale(glm::translate(glm::mat4(1.0), pos), scale) * rotationFromEuler(eulerAngles);
}
