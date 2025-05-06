#include "engine.h"
// GLEW
#include <GL/glew.h>

/**
 * @brief Constructs an empty Program.
 *
 * Initializes the internal program ID to zero.
 */
ENG_API Eng::Program::Program() : id(0)
{
}

/**
 * @brief Destroys the Program, deleting the OpenGL program object.
 */
ENG_API Eng::Program::~Program()
{
	glDeleteProgram(id);
}

/**
 * @brief Attaches a compiled Shader object to this Program.
 *
 * @param shader Shared pointer to the Shader to attach.
 * @return Reference to this Program for method chaining.
 */
ENG_API Eng::Program& Eng::Program::addShader(const std::shared_ptr<Eng::Shader>& shader)
{
	shaders.push_back(shader);
	return *this;
}

/**
 * @brief Creates and links the OpenGL shader program.
 *
 * Deletes any existing program, recreates it, attaches all shaders,
 * links and validates the program, and applies attribute bindings.
 * @return True on successful link and validation; false on failure.
 */
bool ENG_API Eng::Program::build()
{
	// Delete if already used:
	if (id)
		glDeleteProgram(id);

	// Create program:
	id = glCreateProgram();
	if (id == 0)
	{
		std::cout << "[ERROR] Unable to create program" << std::endl;
		return false;
	}

	for (const auto& shader : shaders) {
		glAttachShader(id, shader->getGlId());
	}

	// Link program:
	glLinkProgram(id);

	// Verify program:
	int status;
	char buffer[MAX_LOGSIZE];
	int length = 0;
	memset(buffer, 0, MAX_LOGSIZE);

	glGetProgramiv(id, GL_LINK_STATUS, &status);
	glGetProgramInfoLog(id, MAX_LOGSIZE, &length, buffer);
	if (status == false)
	{
		std::cout << "[ERROR] Program link error: " << buffer << std::endl;
		return false;
	}

	glValidateProgram(id);
	glGetProgramiv(id, GL_VALIDATE_STATUS, &status);
	if (status == GL_FALSE)
	{
		std::cout << "[ERROR] Unable to validate program" << std::endl;
		return false;
	}

	for (const auto& attrib : attributeBindings) {
		glBindAttribLocation(id, attrib.first, attrib.second.c_str());
	}

	// Done:
	return true;
}

/**
 * @brief Activates this Program for rendering.
 *
 * Calls glUseProgram and sets sampler uniforms to their bound units.
 */
void ENG_API Eng::Program::render()
{
	// Activate program:
	if (id) {
		glUseProgram(id);

		for (const auto& sampler : samplerBindings) {
			GLint samplerLocation = glGetUniformLocation(id, sampler.second.c_str());
			glUniform1i(samplerLocation, sampler.first);
		}
	}
	else
	{
		std::cerr << "[ERROR] Invalid program for render" << std::endl;
	}
}

/**
 * @brief Retrieves the location of a uniform variable by name.
 *
 * @param name Name of the uniform variable.
 * @return Location ID, or -1 if not found.
 */
int ENG_API Eng::Program::getParamLocation(const char* name)
{
	if (name == nullptr)
	{
		std::cerr << "[ERROR] Invalid params" << std::endl;
		return 0;
	}

	// Return location:
	int r = glGetUniformLocation(id, name);
	/*
	if (r == -1)
		std::cout << "[ERROR] Param '" << name << "' not found" << std::endl;
	*/
	return r;
}

/**
 * @brief Uploads a 4×4 matrix uniform to the shader.
 *
 * @param param Uniform location obtained via getParamLocation().
 * @param mat   The matrix to set.
 */
void ENG_API Eng::Program::setMatrix(int param, const glm::mat4& mat)
{
	glUniformMatrix4fv(param, 1, GL_FALSE, glm::value_ptr(mat));
}

/**
 * @brief Uploads a 3×3 matrix uniform to the shader.
 *
 * @param param Uniform location obtained via getParamLocation().
 * @param mat   The matrix to set.
 */
void ENG_API Eng::Program::setMatrix(int param, const glm::mat3& mat) {
	glUniformMatrix3fv(param, 1, GL_FALSE, glm::value_ptr(mat));
}

/**
 * @brief Uploads a float uniform to the shader.
 *
 * @param param Uniform location.
 * @param value Float value to upload.
 */
void ENG_API Eng::Program::setFloat(int param, float value)
{
	glUniform1f(param, value);
}

/**
 * @brief Uploads an int uniform to the shader.
 *
 * @param param Uniform location.
 * @param value Integer value to upload.
 */
void ENG_API Eng::Program::setInt(int param, int value)
{
	glUniform1i(param, value);
}

/**
 * @brief Uploads a vec3 uniform to the shader.
 *
 * @param param Uniform location.
 * @param vect  glm::vec3 value to upload.
 */
void ENG_API Eng::Program::setVec3(int param, const glm::vec3& vect)
{
	glUniform3fv(param, 1, glm::value_ptr(vect));
}

/**
 * @brief Uploads a vec4 uniform to the shader.
 *
 * @param param Uniform location.
 * @param vect  glm::vec4 value to upload.
 */
void ENG_API Eng::Program::setVec4(int param, const glm::vec4& vect)
{
	glUniform4fv(param, 1, glm::value_ptr(vect));
}

/**
 * @brief Binds a vertex attribute location for linking.
 *
 * Must be called before build().
 * @param location Explicit attribute index.
 * @param attribName Name of the attribute in the shader.
 * @return Reference to this Program.
 */
ENG_API Eng::Program& Eng::Program::bindAttribute(int location, const char* attribName)
{
	attributeBindings[location] = attribName;
	return *this;
}

/**
 * @brief Binds a texture sampler uniform to a texture unit.
 *
 * Must be called before render().
 * @param unitIndex Texture unit index (0-based).
 * @param samplerName Name of the sampler uniform.
 * @return Reference to this Program.
 */
ENG_API Eng::Program& Eng::Program::bindSampler(int unitIndex, const char* samplerName)
{
	samplerBindings[unitIndex] = samplerName;
	return *this;
}

/**
 * @brief Returns the internal OpenGL program handle.
 *
 * @return GLuint program ID.
 */
unsigned int ENG_API Eng::Program::getGlId() {
	return id;
}
