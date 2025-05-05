#pragma once

class ENG_API ColorPass : public Eng::RenderPass {
public:
	ColorPass(std::shared_ptr<Eng::Program> program, BlendingMode mode, glm::vec3 globalLightColor = glm::vec3(0.0f, 0.0f, 0.0f));
	~ColorPass() = default;
	void setBlendingMode(BlendingMode mode) { this->mode = mode; }
	void setGlobalLightColor(glm::vec3& color);
private:
	BlendingMode mode = Standard;

	// Variables for Shading
	glm::vec3 globalLightColor;

	void configRender() override;

	void perElementConfig(const std::shared_ptr<Eng::ListElement>& element) override;
};
