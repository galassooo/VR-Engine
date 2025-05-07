#pragma once

class ENG_API RenderPipeline {
public:
	RenderPipeline();
	~RenderPipeline();
	bool init();
	void runOn(Eng::List* renderList);
private:
	// Forward declarations
	struct RenderContext;
	struct StatusCache;

	bool setupShadowMap(int width, int height);

	void renderPass(const std::shared_ptr<RenderContext>& context);

	void shadowPass(std::shared_ptr <Eng::DirectionalLight>& light, Eng::List* renderList);


	std::shared_ptr<Eng::Fbo> shadowMapFbo;
	unsigned int shadowMapTexture = 0;

	glm::mat4 lightSpaceMatrix = glm::mat4(1.0f);

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

	std::unique_ptr<StatusCache> prevStatus;
};
