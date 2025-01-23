#pragma once
#include <glm/glm.hpp>

template<typename T>
T ceilDiv(T x, T y) {
    return (x + y - 1) / y;
}

glm::mat4 rotationFromEuler(const glm::vec3 &eulerAngles);

glm::mat4 makeTransform(const glm::vec3 &pos, const glm::vec3 &scale = glm::vec3(1.0), const glm::vec3 &eulerAngles = glm::vec3(0.0, 0.0, 0.0));
