#pragma once

/**
 * @class Program
 * @brief A class to represent a combination of shaders for rendering
 *
 * To Complete
 *
 */
class ENG_API Program : public Eng::Object {
public:
	static const unsigned int MAX_LOGSIZE = 4096;  ///< Max output size in char for a shader log

	Program();
	~Program();
	Program& addShader(const std::shared_ptr<Eng::Shader>& shader);
	bool build();
	void render() override;

	unsigned int getGlId();

	void bind(int location, const char* attribName);

	// Get/set:
	int getParamLocation(const char* name);
	void setMatrix(int param, const glm::mat4& mat);

	void setFloat(int param, float value);

	void setInt(int param, int value);

	void setVec3(int param, const glm::vec3& vect);

	void setVec4(int param, const glm::vec4& vect);

private:
	// OGL id:
	unsigned int id;
	std::vector<std::shared_ptr<Eng::Shader>> shaders;
};
