#include "engine.h"

Eng::BoundingBox::BoundingBox(const glm::vec3& min, const glm::vec3& max) : min(min), max(max) {}

/// @brief Updates the bounding box with a new point
void Eng::BoundingBox::update(const glm::vec3& point) {
	min = glm::min(min, point);
	max = glm::max(max, point);
}
/// @brief Returns the center of the bounding box
glm::vec3 Eng::BoundingBox::getCenter() const {
	return (min + max) * 0.5f;
}
/// @brief Returns the size of the bounding box
glm::vec3 Eng::BoundingBox::getSize() const {
	return max - min;
}

/// @brief Returns the minimum corner of the bounding box
glm::vec3 Eng::BoundingBox::getMin() const {
	return min;
}

/// @brief Returns the maximum corner of the bounding box
glm::vec3 Eng::BoundingBox::getMax() const {
	return max;
}

std::vector<glm::vec3> Eng::BoundingBox::getVertices() {
	std::vector<glm::vec3> vertices = {
	{min.x, min.y, min.z},
	{max.x, min.y, min.z},
	{min.x, max.y, min.z},
	{max.x, max.y, min.z},
	{min.x, min.y, max.z},
	{max.x, min.y, max.z},
	{min.x, max.y, max.z},
	{max.x, max.y, max.z}
	};
	return vertices;
}

void Eng::BoundingBox::reset() {
	min = glm::vec3(FLT_MAX);
	max = glm::vec3(-FLT_MAX);
}