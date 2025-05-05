#pragma once

class ENG_API ShadowPass : public Eng::RenderPass {
public:
	ShadowPass(std::shared_ptr<Eng::Program>& program, std::shared_ptr<Eng::Fbo>& shadowMapFbo);
	~ShadowPass() = default;

private:

	void configRender() override;

	void perElementConfig(const std::shared_ptr<Eng::ListElement>& element) override;

	std::shared_ptr<Eng::Fbo> shadowMapFbo;
};
