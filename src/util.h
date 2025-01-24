#pragma once
#include <glm/glm.hpp>

# define M_PI		3.14159265358979323846	/* pi */
# define M_PI_2		1.57079632679489661923	/* pi/2 */
# define M_PI_4		0.78539816339744830962	/* pi/4 */
# define M_1_PI		0.31830988618379067154	/* 1/pi */
# define M_2_PI		0.63661977236758134308	/* 2/pi */

template<typename T>
T ceilDiv(T x, T y) {
    return (x + y - 1) / y;
}

glm::mat4 rotationFromEuler(const glm::vec3 &eulerAngles);

glm::mat4 makeTransform(const glm::vec3 &pos, const glm::vec3 &scale = glm::vec3(1.0), const glm::vec3 &eulerAngles = glm::vec3(0.0, 0.0, 0.0));
