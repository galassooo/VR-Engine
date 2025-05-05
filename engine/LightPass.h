#pragma once

class ENG_API LightPass : public Eng::RenderPass {
public:
	LightPass(std::shared_ptr<Eng::VertexShader>& vs, std::shared_ptr<Eng::List> renderList = nullptr);
	~LightPass() = default;

	void setRenderList(std::shared_ptr<Eng::List>& renderList);

private:
	std::shared_ptr<Eng::Fbo> shadowMapFbo;
	unsigned int shadowMapTexture = 0;

	std::shared_ptr<Eng::List> renderList;
	std::shared_ptr<Eng::Program> spotProgram;
	std::shared_ptr<Eng::Program> pointProgram;
	std::shared_ptr<Eng::Program> dirProgram;
	std::shared_ptr<Eng::Program> shadowMapProgram;

	std::shared_ptr<Eng::ColorPass> pointPass;
	std::shared_ptr<Eng::ColorPass> spotPass;
	std::shared_ptr<Eng::ColorPass> dirPass;
	std::shared_ptr<Eng::ShadowPass> shadowPass;

	std::shared_ptr<Eng::RenderPassContext> opaqueContext = std::make_shared<Eng::RenderPassContext>(Eng::CullingMode::Sphere, Eng::RenderLayer::Opaque);
	std::shared_ptr<Eng::RenderPassContext> transparentContext = std::make_shared<Eng::RenderPassContext>(Eng::CullingMode::Sphere, Eng::RenderLayer::Transparent);
	std::shared_ptr<Eng::RenderPassContext> shadowContext = std::make_shared<Eng::RenderPassContext>(Eng::CullingMode::None, Eng::RenderLayer::Opaque);

	std::shared_ptr<Eng::VertexShader> vertexShader;

	void configRender() override;

	void perElementConfig(const std::shared_ptr<Eng::ListElement>& element) override;

	bool init() override;

	bool setupShadowMap(int width, int height);
};
