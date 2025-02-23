#pragma once

/**
 * @class List
 * @brief A render list that extends Object to integrate with the scene graph.
 *
 * The List class stores a collection of nodes in the correct rendering order:
 * lights are pushed to the front, and other nodes are pushed to the back.
 */
class ENG_API List final : public Eng::Object {
public:
	List();

	void addNode(const std::shared_ptr<Eng::Node>& node, const glm::mat4& finalMatrix);
	void render() override;
	void clear();
	std::vector<std::shared_ptr<Eng::ListElement>> getElements() const;

	void setViewMatrix(glm::mat4& viewMatrix);
	
private:
	/** @brief Sorted collection of renderable nodes with their world coordinates and materials.
	 *
	 * The elements are sorted by render layer (lights first, then opaque objects,
	 * and finally transparent objects) to ensure correct rendering order.
	 */
	std::vector<std::shared_ptr<Eng::ListElement>> elements;
	///> Maximum number of lights supported by OpenGL
	static const int MAX_LIGHTS = 8;

	glm::mat4 viewMatrix;
};

