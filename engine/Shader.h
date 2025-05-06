#pragma once

/**
 * @class Shader
 * @brief Abstract base class representing a GPU shader.
 *
 * Shader provides a common interface for creating, loading, and
 * rendering GPU shader programs. Derived classes must implement the
 * create() method to generate the underlying OpenGL shader object.
 */
class ENG_API Shader : public Eng::Object {
public:
	static const unsigned int MAX_LOGSIZE = 4096;  ///< Max output size in char for a shader log

	Shader();
	~Shader();
	bool load(const char* data);
	void render() override;
	
	unsigned int getGlId();
protected:
	virtual unsigned int create() = 0;
private:
	// OGL id:
	unsigned int id;
};
