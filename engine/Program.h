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

	inline void bind(int location, const char* attribName)
	{
		glBindAttribLocation(glId, location, attribName);
	}

	// Get/set:
	int getParamLocation(const char* name);
	inline void setMatrix(int param, const glm::mat4& mat)
	{
		glUniformMatrix4fv(param, 1, GL_FALSE, glm::value_ptr(mat));
	}
	inline void setFloat(int param, float value)
	{
		glUniform1f(param, value);
	}
	inline void setInt(int param, int value)
	{
		glUniform1i(param, value);
	}
	inline void setVec3(int param, const glm::vec3& vect)
	{
		glUniform3fv(param, 1, glm::value_ptr(vect));
	}
	inline void setVec4(int param, const glm::vec4& vect)
	{
		glUniform4fv(param, 1, glm::value_ptr(vect));
	}
private:
	// OGL id:
	GLuint glId;
	std::vector<std::shared_ptr<Eng::Shader>> shaders;
};
