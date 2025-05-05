#pragma once

class ENG_API RenderPass {
public:
	enum BlendingMode{
		Standard,
		Additive,
		Transparent
	};

	RenderPass() = default;
	RenderPass(std::shared_ptr<Eng::Program>& program);
	~RenderPass() = default;

	void start(const glm::mat4& eyeProjectionMatrix, const glm::mat4& eyeViewMatrix, const glm::mat4& lightSpaceMatrix = glm::mat4(1.0f));
	void stop();
	void render(const std::shared_ptr<Eng::ListElement>& element);

protected:
	struct PrevState {
		int srcRGB;
		int dstRGB;
		bool blendingEnabled;
		unsigned char writeDepthMask;
		int viewport[4];
		int FBO;
	};

	PrevState previousState;

	virtual bool init();

	bool initialized = false;
	bool started = false;
	Eng::ShaderManager& shaderManager = Eng::ShaderManager::getInstance();

	std::shared_ptr<Eng::Program> renderProgram = nullptr;

	// Variables for Shading
	glm::mat4 eyeProjectionMatrix = glm::mat4(1.0f);
	glm::mat4 eyeViewMatrix = glm::mat4(1.0f);;
	glm::mat4 lightSpaceMatrix = glm::mat4(1.0f);;

	virtual void configRender() = 0;

	virtual void perElementConfig(const std::shared_ptr<Eng::ListElement>& element) = 0;
};