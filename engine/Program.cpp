#include "engine.h"
// GLEW
#include <GL/glew.h>

ENG_API Eng::Program::Program() : id(0)
{
}

ENG_API Eng::Program::~Program()
{
	glDeleteProgram(id);
}

ENG_API Eng::Program& Eng::Program::addShader(const std::shared_ptr<Eng::Shader>& shader)
{
	shaders.push_back(shader);
	return *this;
}

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
 * Returns the param location given its variable name.
 * @param name variable name
 * @return location ID or -1 if not found
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

void ENG_API Eng::Program::setMatrix(int param, const glm::mat4& mat)
{
	glUniformMatrix4fv(param, 1, GL_FALSE, glm::value_ptr(mat));
}

void ENG_API Eng::Program::setMatrix(int param, const glm::mat3& mat) {
	glUniformMatrix3fv(param, 1, GL_FALSE, glm::value_ptr(mat));
}

void ENG_API Eng::Program::setFloat(int param, float value)
{
	glUniform1f(param, value);
}

void ENG_API Eng::Program::setInt(int param, int value)
{
	glUniform1i(param, value);
}

void ENG_API Eng::Program::setVec3(int param, const glm::vec3& vect)
{
	glUniform3fv(param, 1, glm::value_ptr(vect));
}

void ENG_API Eng::Program::setVec4(int param, const glm::vec4& vect)
{
	glUniform4fv(param, 1, glm::value_ptr(vect));
}

ENG_API Eng::Program& Eng::Program::bindAttribute(int location, const char* attribName)
{
	attributeBindings[location] = attribName;
	return *this;
}

ENG_API Eng::Program& Eng::Program::bindSampler(int unitIndex, const char* samplerName)
{
	samplerBindings[unitIndex] = samplerName;
	return *this;
}

unsigned int ENG_API Eng::Program::getGlId() {
	return id;
}
