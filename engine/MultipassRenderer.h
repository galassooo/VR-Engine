#pragma once

class ENG_API MultipassRenderer {
public:
	static MultipassRenderer& getInstance();
	bool init();
	void render(Eng::List& renderList);

	void setCurrentFBO(Eng::Fbo* fbo) { currentFBO = std::shared_ptr<Eng::Fbo>(fbo, [](Eng::Fbo*) {}); }
private:
	MultipassRenderer() = default;
	~MultipassRenderer() = default;
	bool initialized = false;

	void renderPass(bool isAdditive, bool useCulling);
	void shadowPass(std::shared_ptr <Eng::DirectionalLight>& light);
	void renderTransparentPass();

	std::shared_ptr<Eng::Fbo> shadowMapFbo;
	unsigned int shadowMapTexture = 0;

	bool setupShadowMap(int width, int height);
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

	// Light matrices caculated during shadow pass
	glm::mat4 lightProjectionMatrix;
	glm::mat4 lightSpaceMatrix;
};
