#include "engine.h"

/**
 * @brief Constructs a BoundingBox with given minimum and maximum corners.
 *
 * @param min Initial minimum corner of the bounding box.
 * @param max Initial maximum corner of the bounding box.
 */
Eng::BoundingBox::BoundingBox(const glm::vec3& min, const glm::vec3& max) : min(min), max(max) {}

/**
 * @brief Updates the bounding box to include the specified point.
 *
 * Expands the min and max corners as needed to encompass the new point.
 *
 * @param point 3D point to include in the bounding box.
 */
void Eng::BoundingBox::update(const glm::vec3& point) {
	min = glm::min(min, point);
	max = glm::max(max, point);
}

/**
 * @brief Computes the center point of the bounding box.
 *
 * @return glm::vec3 Coordinates of the bounding box center.
 */
glm::vec3 Eng::BoundingBox::getCenter() const {
	return (min + max) * 0.5f;
}

/**
 * @brief Computes the size (extent) of the bounding box along each axis.
 *
 * @return glm::vec3 Difference between max and min corners.
 */
glm::vec3 Eng::BoundingBox::getSize() const {
	return max - min;
}

/**
 * @brief Retrieves the minimum corner of the bounding box.
 *
 * @return glm::vec3 The current minimum corner coordinates.
 */
glm::vec3 Eng::BoundingBox::getMin() const {
	return min;
}

/**
 * @brief Retrieves the maximum corner of the bounding box.
 *
 * @return glm::vec3 The current maximum corner coordinates.
 */
glm::vec3 Eng::BoundingBox::getMax() const {
	return max;
}

/**
 * @brief Computes and returns all eight corner vertices of the bounding box.
 *
 * @return std::vector<glm::vec3> List of eight corner positions.
 */
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

/**
 * @brief Resets the bounding box to an empty state.
 *
 * Sets min to large positive values and max to large negative values,
 * making the box invalid until updated with points.
 */
void Eng::BoundingBox::reset() {
	min = glm::vec3(FLT_MAX);
	max = glm::vec3(-FLT_MAX);
}