#pragma once

/**
 * @class BoundingBox
 * @brief Axis-aligned bounding box for 3D objects.
 *
 * Maintains minimum and maximum corner points, and provides
 * methods to update bounds, compute center, size, and retrieve
 * corner vertices. Automatically expands to include new points.
 */
class ENG_API BoundingBox {
public:
	BoundingBox() = default;
	BoundingBox(const glm::vec3& min, const glm::vec3& max);

	void update(const glm::vec3& point);

	glm::vec3 getCenter() const;

	glm::vec3 getSize() const;

	glm::vec3 getMin() const;

	glm::vec3 getMax() const;

	std::vector<glm::vec3> getVertices();

	void reset();

private:
	glm::vec3 min = glm::vec3(FLT_MAX); ///< Minimum corner of the bounding box
	glm::vec3 max = glm::vec3(-FLT_MAX); ///< Maximum corner of the bounding box
};