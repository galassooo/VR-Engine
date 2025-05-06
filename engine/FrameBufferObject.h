#pragma once


/**
 * @class Fbo
 * @brief Frame buffer class to deal with OpenGL FBOs.
 *
 * Manages creation and configuration of OpenGL frame buffer objects,
 * including texture and render buffer attachments, binding operations,
 * and rendering control.
 */
class ENG_API Fbo
{
public:

	   // Constants:
	static const unsigned int MAX_ATTACHMENTS = 8; ///< Maximum number of available render buffers or textures per FBO	

	// Enumerations:
	enum : unsigned int ///< Kind of operation
	{
		BIND_DEPTHBUFFER = 0,
		BIND_COLORTEXTURE,
		BIND_DEPTHTEXTURE,
	};

	// Const/dest:	 
	Fbo();
	~Fbo();

	// Get/set:   
	unsigned int getTexture(unsigned int textureNumber);
	inline int getSizeX() { return sizeX; }
	inline int getSizeY() { return sizeY; }
	inline int getSizeZ() { return sizeZ; }
	inline unsigned int getHandle() { return glId; }

	// Management:
	bool isOk();
	bool bindTexture(unsigned int textureNumber, unsigned int operation, unsigned int texture, int param1 = 0, int param2 = 0);
	bool bindRenderBuffer(unsigned int renderBuffer, unsigned int operation, int sizeX, int sizeY);
	void setDepthOnly(bool value);

	// Rendering:     
	bool render(void* data = nullptr);
	static void disable();

private:

		// Generic data:
	int sizeX, sizeY, sizeZ;	         					///< FBO width, height and depth
	unsigned int texture[MAX_ATTACHMENTS];             ///< Attached textures
	int drawBuffer[MAX_ATTACHMENTS];       		      ///< Set color attachment per texture
	bool depthOnly = false;

	// OGL stuff:
	unsigned int glId;                                 ///< OpenGL ID
	unsigned int glRenderBufferId[MAX_ATTACHMENTS];    ///< Render buffer IDs

	// MRT cache:   
	int nrOfMrts;                                      ///< Number of MRTs
	unsigned int* mrt;                                 ///< Cached list of buffers 

	// Cache:
	bool updateMrtCache();
};
