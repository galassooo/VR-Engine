#pragma once

class ENG_API MultipassRenderer : public Eng::Renderer {
public:
	MultipassRenderer() = default;
	~MultipassRenderer() = default;
	bool init();
	void render(const std::shared_ptr<Eng::List>& renderList) override;

private:
	void runPass(const std::shared_ptr<Eng::RenderPass>& pass, const std::shared_ptr<Eng::RenderPassContext>& context);

	std::shared_ptr<Eng::VertexShader> basicVertexShader;
	std::shared_ptr<Eng::FragmentShader> basicFragmentShader;

	std::shared_ptr<Eng::Program> baseColorProgram;

	std::shared_ptr<Eng::ColorPass> baseColorPass;
	std::shared_ptr<Eng::LightPass> lightingPass;

	const std::shared_ptr<Eng::RenderPassContext> meshContext = std::make_shared<Eng::RenderPassContext>(Eng::CullingMode::Sphere, Eng::RenderLayer::Opaque);
	const std::shared_ptr<Eng::RenderPassContext> lightContext = std::make_shared<Eng::RenderPassContext>(Eng::CullingMode::Sphere, Eng::RenderLayer::Lights);
};
