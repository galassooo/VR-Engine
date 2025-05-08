#include "Engine.h"
// Glew (include it before GL.h):
#include <GL/glew.h>

/**
 * @brief Constructs a new Fbo object.
 *
 * Initializes internal attachment arrays, MRT cache, and generates
 * an OpenGL framebuffer object.
 */
ENG_API Eng::Fbo::Fbo()
{
	// Init reserved data:
	memset(glRenderBufferId, 0, sizeof(unsigned int) * MAX_ATTACHMENTS);
	for (unsigned int c = 0; c < Fbo::MAX_ATTACHMENTS; c++)
	{
		texture[c] = 0;
		drawBuffer[c] = -1;	// -1 means empty
	}
	nrOfMrts = 0;
	mrt = nullptr;

	// Allocate OGL data:
	glGenFramebuffers(1, &glId);
}

	 
/**
 * @brief Destroys the Fbo object.
 *
 * Deletes the MRT cache, render buffers, and the OpenGL framebuffer.
 */
ENG_API Eng::Fbo::~Fbo()
{
	// Release reserved data:
	if (mrt)
		delete[] mrt;
	for (unsigned int c = 0; c < Fbo::MAX_ATTACHMENTS; c++)
		if (glRenderBufferId[c])
			glDeleteRenderbuffers(1, &glRenderBufferId[c]);
	glDeleteFramebuffers(1, &glId);
}

 
/**
 * @brief Retrieves the texture ID attached at the given index.
 *
 * @param textureNumber Index of the texture attachment (0..MAX_ATTACHMENTS-1).
 * @return GLuint texture ID, or 0 if index is out of range or not attached.
 */
unsigned int ENG_API Eng::Fbo::getTexture(unsigned int textureNumber)
{
	if (textureNumber < Fbo::MAX_ATTACHMENTS)
		return texture[textureNumber];
	else
		return 0;
}
	 
/**
 * @brief Checks the completeness of the framebuffer.
 *
 * Binds the FBO and queries its status. Prints an error message
 * if the framebuffer is not complete.
 *
 * @return True if framebuffer is complete; false otherwise.
 */
bool ENG_API Eng::Fbo::isOk()
{
	// Make FBO current:
	render();

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "[ERROR] FBO not complete (error: " << status << ")" << std::endl;
		return false;
	}

	// Done:
	return true;
}
 
/**
 * @brief Attaches a texture to the framebuffer.
 *
 * Binds the FBO, attaches the specified texture to a color or depth attachment,
 * and updates the draw buffer and size fields.
 *
 * @param textureNumber Index in the internal texture array (0..MAX_ATTACHMENTS-1).
 * @param operation Attachment operation (BIND_COLORTEXTURE or BIND_DEPTHTEXTURE).
 * @param texture OpenGL texture ID to bind.
 * @param param1 Additional parameter (e.g., color attachment index).
 * @param param2 Additional parameter (unused).
 * @return True on successful binding; false on failure.
 */
bool ENG_API Eng::Fbo::bindTexture(unsigned int textureNumber, unsigned int operation, unsigned int texture, int param1, int param2)
{
	// Safety net:
	if (textureNumber >= Fbo::MAX_ATTACHMENTS)
	{
		std::cout << "[ERROR] Invalid params" << std::endl;
		return false;
	}

	// Bind buffer:
	render();

	// Perform operation:   
	switch (operation)
	{
	case BIND_COLORTEXTURE:
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + param1, GL_TEXTURE_2D, texture, 0);
		drawBuffer[textureNumber] = param1;
		break;

	case BIND_DEPTHTEXTURE:
		// glReadBuffer(GL_NONE);
	 // glDrawBuffer(GL_NONE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture, 0);
		break;

	default:
		std::cout << "[ERROR] Invalid operation" << std::endl;
		return false;
	}

	this->texture[textureNumber] = texture;


	glBindTexture(GL_TEXTURE_2D, texture);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sizeX);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &sizeY);
	return updateMrtCache();
}

/**
 * @brief Attaches and initializes a render buffer.
 *
 * Binds the FBO, recreates the specified render buffer, allocates storage,
 * attaches it as a depth buffer, and updates size fields.
 *
 * @param renderBuffer Index of the render buffer (0..MAX_ATTACHMENTS-1).
 * @param operation Attachment operation (BIND_DEPTHBUFFER).
 * @param sizeX Width of the buffer.
 * @param sizeY Height of the buffer.
 * @return True on successful allocation and binding; false on failure.
 */
bool ENG_API Eng::Fbo::bindRenderBuffer(unsigned int renderBuffer, unsigned int operation, int sizeX, int sizeY)
{
	// Safety net:
	if (renderBuffer >= Fbo::MAX_ATTACHMENTS)
	{
		std::cout << "[ERROR] Invalid params" << std::endl;
		return false;
	}

	// Bind buffer:
	render();

	// If used, delete it first, then create and bind it:
	if (glRenderBufferId[renderBuffer])
		glDeleteRenderbuffers(1, &glRenderBufferId[renderBuffer]);
	glGenRenderbuffers(1, &glRenderBufferId[renderBuffer]);
	glBindRenderbuffer(GL_RENDERBUFFER, glRenderBufferId[renderBuffer]);

	// Perform operation:
	switch (operation)
	{

	case BIND_DEPTHBUFFER:
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, sizeX, sizeY);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, glRenderBufferId[renderBuffer]);
		break;

	default:
		std::cout << "[ERROR] Invalid operation" << std::endl;
		return false;
	}

	// Done:   
	this->sizeX = sizeX;
	this->sizeY = sizeY;
	return updateMrtCache();
}


/**
 * @brief Updates the multiple render targets (MRT) cache.
 *
 * Rebuilds the MRT list based on the current drawBuffer array.
 *
 * @return True on success.
 */
bool ENG_API Eng::Fbo::updateMrtCache()
{
	// Delete previous buffer:
	if (mrt)
		delete[] mrt;

	// Count targets:   
	nrOfMrts = 0;
	for (unsigned int c = 0; c < Fbo::MAX_ATTACHMENTS; c++)
		if (drawBuffer[c] != -1)
			nrOfMrts++;

	// Refresh buffer:
	if (nrOfMrts)
	{
		mrt = new GLenum[nrOfMrts];
		int bufferPosition = 0;
		for (int c = 0; c < Fbo::MAX_ATTACHMENTS; c++)
			if (drawBuffer[c] != -1)
			{
				mrt[bufferPosition] = GL_COLOR_ATTACHMENT0 + drawBuffer[c];
				bufferPosition++;
			}
	}

	// Done: 
	return true;
}

/**
 * @brief Reverts rendering to the default framebuffer.
 *
 * Binds the context framebuffer and resets read/draw buffers.
 */
void ENG_API Eng::Fbo::disable()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDrawBuffer(GL_BACK);
	glReadBuffer(GL_BACK);
}

/**
 * @brief Sets whether rendering uses depth-only mode.
 *
 * @param value True to disable color output; false to enable color.
 */
void ENG_API Eng::Fbo::setDepthOnly(bool value)
{
	depthOnly = value;
}


/**
 * @brief Binds and configures the framebuffer for rendering.
 *
 * Binds the FBO, sets draw buffers and viewport dimensions,
 * and applies depth-only mode if enabled.
 *
 * @param data Optional user data pointer (unused).
 * @return True if binding succeeded.
 */
bool ENG_API Eng::Fbo::render(void* data)
{
	// Bind buffers:
	glBindFramebuffer(GL_FRAMEBUFFER, glId);
	if (nrOfMrts)
	{
		glDrawBuffers(nrOfMrts, mrt);
		glViewport(0, 0, sizeX, sizeY);
	}

	if (depthOnly) {
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
	}

	// Done:   
	return true;
}
