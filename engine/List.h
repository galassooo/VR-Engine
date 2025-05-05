#pragma once

#include <memory>

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
	~List();

	void addNode(const std::shared_ptr<Eng::Node>& node, const glm::mat4& finalMatrix);
	void render() override;
	void clear();

	std::vector<std::shared_ptr<Eng::ListElement>> getElements() const;

	void setEyeViewMatrix(glm::mat4& viewMatrix);
	void setEyeProjectionMatrix(glm::mat4& eyeProjectionMatrix);

	glm::mat4 getEyeViewMatrix();
	glm::mat4 getEyeProjectionMatrix();

	std::shared_ptr<Eng::BoundingBox> getSceneBoundingBox();

	std::vector<glm::vec3> getEyeFrustumCorners();

	void iterateAndRender(const std::shared_ptr<Eng::RenderPass>& renderPass, const std::shared_ptr<Eng::RenderPassContext>& context);

private:
	/** @brief Sorted collection of renderable nodes with their world coordinates and materials.
	 *
	 * The elements are sorted by render layer (lights first, then opaque objects,
	 * and finally transparent objects) to ensure correct rendering order.
	 */
	std::vector<std::shared_ptr<Eng::ListElement>> elements;
	///> Maximum number of lights supported by OpenGL
	static const int MAX_LIGHTS = 8;
	int lightsCount = 0;

	glm::mat4 eyeViewMatrix;
	glm::mat4 eyeProjectionMatrix;

	struct CullingSphere;
	std::unique_ptr<CullingSphere> cullingSphere;

	std::shared_ptr<Eng::BoundingBox> sceneBoundingBox = nullptr;
	std::unique_ptr<std::vector<glm::vec3>> currentFrustumCorners = nullptr;

	bool shouldCull(const std::shared_ptr<Eng::ListElement>& element, Eng::CullingMode mode);
	bool isWithinCullingSphere(const std::shared_ptr<Eng::Mesh>& mesh);

	std::vector<glm::vec3> computeFrustumCorners(glm::mat4 projectionMatrix, glm::mat4 viewMatrix);

	void updateCullingSphere();

	std::shared_ptr<Eng::RenderPass> latestRenderPass = nullptr;
	std::shared_ptr<Eng::RenderPassContext> latestRenderPassContext = nullptr;
};



