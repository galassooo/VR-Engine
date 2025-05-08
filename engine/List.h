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

	bool isWithinCullingSphere(const std::shared_ptr<Eng::Mesh>& mesh);

	void setEyeViewMatrix(glm::mat4& viewMatrix);
	void setEyeProjectionMatrix(glm::mat4& eyeProjectionMatrix);

	void setGlobalLightColor(const glm::vec3& globalColor);

	void setCurrentFBO(Eng::Fbo* fbo) { currentFBO = std::shared_ptr<Eng::Fbo>(fbo, [](Eng::Fbo*) {}); }

	std::vector<std::shared_ptr<Eng::ListElement>> getElements() const;

	std::shared_ptr<Eng::BoundingBox> getSceneBoundingBox();

	std::vector<glm::vec3> getEyeFrustumCorners();

	Eng::ListIterator getLayerIterator(const Eng::RenderLayer& layer);

	glm::vec3 getGlobalLightColor() const { return globalLightColor; }

	glm::mat4 getEyeViewMatrix() const { return eyeViewMatrix; }

	glm::mat4 getEyeProjectionMatrix() const { return eyeProjectionMatrix; }

private:
	// Forward declarations

	struct CullingSphere;

	/** @brief Sorted collection of renderable nodes with their world coordinates and materials.
	 *
	 * The elements are sorted by render layer (lights first, then opaque objects,
	 * and finally transparent objects) to ensure correct rendering order.
	 */
	std::vector<std::shared_ptr<Eng::ListElement>> elements;
	///> Maximum number of lights supported by OpenGL
	static const int MAX_LIGHTS = 8;

	// Externally set private values 

	glm::mat4 eyeViewMatrix;
	glm::mat4 eyeProjectionMatrix;
	glm::vec3 globalLightColor = glm::vec3(0.0f, 0.0f, 0.0f);

	std::shared_ptr<Eng::Fbo> currentFBO = nullptr;

	// Computed private values

	int lightsCount = 0;
	std::shared_ptr<Eng::BoundingBox> sceneBoundingBox = nullptr;

	//  Cached values

	std::unique_ptr<CullingSphere> cullingSphereCached = nullptr;
	std::unique_ptr<std::vector<glm::vec3>> frustumCornersCached = nullptr;
	std::shared_ptr<Eng::ListIterator> lightsIteratorCached = nullptr;
	std::shared_ptr<Eng::ListIterator> opaqueIteratorCached = nullptr;
	std::shared_ptr<Eng::ListIterator> transparentIteratorCached = nullptr;

	// Private Methods

	std::shared_ptr<std::vector<std::shared_ptr<Eng::ListElement>>> getElements(const Eng::RenderLayer& layer);
	void computeCullingSphere();
	std::vector<glm::vec3> computeFrustumCorners(glm::mat4 projectionMatrix, glm::mat4 viewMatrix);
};



