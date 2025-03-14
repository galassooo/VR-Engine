#pragma once

/**
 * @class Shader
 * @brief An abstract class to represent any possible Shader
 *
 * Complete
 * 
 */
class ENG_API Shader : public Eng::Object {
public:
	static const unsigned int MAX_LOGSIZE = 4096;  ///< Max output size in char for a shader log

	Shader();
	~Shader();
	bool load(const char* data);
	void render() override;
	
	GLuint getGlId();
protected:
	virtual GLuint create() = 0;
private:
	// OGL id:
	GLuint glId;
};
