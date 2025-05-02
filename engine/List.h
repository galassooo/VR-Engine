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
	bool initShaders();
	std::vector<std::shared_ptr<Eng::ListElement>> getElements() const;

	void setEyeViewMatrix(glm::mat4& viewMatrix);
	void setEyeProjectionMatrix(glm::mat4& eyeProjectionMatrix);

	void setGlobalLightColor(const glm::vec3& globalColor);

	void setCurrentFBO(Eng::Fbo* fbo) { currentFBO = std::shared_ptr<Eng::Fbo>(fbo, [](Eng::Fbo*) {}); }

	std::shared_ptr<Eng::BoundingBox> getSceneBoundingBox();

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
	glm::mat4 lightProjectionMatrix;
	glm::mat4 lightSpaceMatrix;

	glm::vec3 globalLightColor = glm::vec3(0.0f, 0.0f, 0.0f);

	struct CullingSphere;
	std::unique_ptr<CullingSphere> cullingSphere;

	std::shared_ptr<Eng::BoundingBox> sceneBoundingBox = nullptr;
	std::unique_ptr<std::vector<glm::vec3>> currentFrustumCorners = nullptr;

	std::vector<glm::vec3> getEyeFrustumCorners();

	std::shared_ptr<Eng::Fbo> shadowMapFbo;
	unsigned int shadowMapTexture = 0;

	bool isWithinCullingSphere(Eng::Mesh* mesh);

	std::vector<glm::vec3> computeFrustumCorners(glm::mat4 projectionMatrix, glm::mat4 viewMatrix);
	glm::mat4 computeLightProjectionMatrix(const glm::mat4& lightViewMatrix, const std::vector<glm::vec3>& boundingBoxCorners);

	bool setupShadowMap(int width, int height);
	void renderPass(bool isAdditive, bool useCulling);
	void shadowPass(std::shared_ptr <Eng::DirectionalLight>& light);
	void renderTransparentPass();

	std::shared_ptr<Eng::VertexShader> basicVertexShader;
	std::shared_ptr<Eng::VertexShader> shadowMapVertexShader;
	std::shared_ptr<Eng::VertexShader> dirLightVertexShader;

	std::shared_ptr<Eng::FragmentShader> basicFragmentShader;
	std::shared_ptr<Eng::FragmentShader> directionalFragmentShader;
	std::shared_ptr<Eng::FragmentShader> pointFragmentShader;
	std::shared_ptr<Eng::FragmentShader> spotFragmentShader;
	std::shared_ptr<Eng::FragmentShader> shadowMapFragmentShader;

	std::shared_ptr<Eng::Program> baseColorProgram;
	std::shared_ptr<Eng::Program> dirLightProgram;
	std::shared_ptr<Eng::Program> pointLightProgram;
	std::shared_ptr<Eng::Program> spotLightProgram;
	std::shared_ptr<Eng::Program> shadowMapProgram;

	std::shared_ptr<Eng::Fbo> currentFBO = nullptr;
};



