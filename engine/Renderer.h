#pragma once

class ENG_API Renderer {
public:
	Renderer() = default;
	~Renderer() = default;
	virtual bool init() = 0;
	virtual void render(const std::shared_ptr<Eng::List>& renderList) = 0;
	void setGlobalLightColor(glm::vec3 color);
protected:
	std::shared_ptr<Eng::List> renderList = nullptr;
	Eng::ShaderManager& shaderManager = Eng::ShaderManager::getInstance();
	glm::mat4 eyeProjectionMatrix = glm::mat4(1.0f);
	glm::mat4 eyeViewMatrix = glm::mat4(1.0f);
	glm::vec3 globalLightColor = glm::vec3(0.0f);
	bool initialized = false;
};
