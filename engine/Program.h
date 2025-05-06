#pragma once

/**
 * @class Program
 * @brief Links and manages a set of shaders into a GPU program for rendering.
 *
 * A Program encapsulates the OpenGL shader program object, allowing addition of
 * compiled shader objects, binding of attribute locations and texture samplers,
 * and retrieval of uniform locations. After building, it can be activated for
 * rendering and used to set uniform variables of various types.
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

	Program& bindAttribute(int location, const char* attribName);
	Program& bindSampler(int unitIndex, const char* samplerName);

	// Get/set:
	int getParamLocation(const char* name);

	void setMatrix(int param, const glm::mat4& mat);

	// Helper function for 3x3 matrices for the per-pixel lighting
	void setMatrix(int param, const glm::mat3& mat);

	void setFloat(int param, float value);

	void setInt(int param, int value);

	void setVec3(int param, const glm::vec3& vect);

	void setVec4(int param, const glm::vec4& vect);

private:
	// OGL id:
	unsigned int id;
	std::vector<std::shared_ptr<Eng::Shader>> shaders;
	std::unordered_map<int, std::string> attributeBindings;
	std::unordered_map<int, std::string> samplerBindings;
};
