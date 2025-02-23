#pragma once

/**
 * @class ListElement
 * @brief Represents an element in the render list.
 *
 * A ListElement stores a reference to a scene graph node, its transformation matrix in world coordinates,
 * and its associated material properties. It is used to organize and render nodes in the scene.
 */
class ENG_API ListElement{
public:
	ListElement(const std::shared_ptr<Eng::Node>& node, const glm::mat4& worldCoordinates);

	Eng::RenderLayer getLayer() const;
	std::shared_ptr<Eng::Node> getNode() const;
	glm::mat4 getWorldCoordinates() const;

private:
	///< Pointer to node
	std::shared_ptr<Eng::Node> node;
	///< Node world coordinates
	glm::mat4 worldCoordinates;
	///< Layer for sorting (Lights, Opaque, Transparent).
	Eng::RenderLayer layer;
};