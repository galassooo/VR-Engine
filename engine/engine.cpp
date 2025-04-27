/**
 * @file		engine.cpp
 * @brief	Graphics engine main file
 *
 * @author	Kevin Alexander Quarenghi Escobar, Martina Galasso
 */

// Main include: f
#include "engine.h"
#include <algorithm>
// C/C++:
#include <iostream>
#include <source_location>
#include <utility>

// GLEW
#include <GL/glew.h>

// FreeGlut
#include <GL/freeglut.h>

// Freeimage
#include <FreeImage.h>

// OpenVR:
#include "ovr.h"

#ifndef _WIN32
#define __stdcall // Just defined as an empty macro under Linux
#endif

/**
 * Debug message callback for OpenGL. See https://www.opengl.org/wiki/Debug_Output
 */
void __stdcall DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam)
{
    std::cout << "OpenGL says: \"" << std::string(message) << "\"" << std::endl;
}


/**
 * @brief Base class reserved structure (using PIMPL/Bridge design pattern https://en.wikipedia.org/wiki/Opaque_pointer).
 */
struct Eng::Base::Reserved {
   // Flags:
   bool initFlag;
   bool ovrReady;

   // OpenVR interface:
   OvVR* ovr;
   int fboSizeX;
   int fboSizeY;

   /**
    * Constructor.
    */
   Reserved() : initFlag{ false }, ovrReady{ false }, ovr { nullptr }, fboSizeX{ 0 }, fboSizeY{ 0 } {
   }
   ~Reserved() {
#ifdef _DEBUG
       std::cout << "[-] " << std::source_location::current().function_name() << " invoked" << std::endl;
#endif
       if (ovr != nullptr && ovrReady) {
           ovr->free();
           delete ovr;
       }
   }
};


/**
 * Constructor.
 */
ENG_API Eng::Base::Base() : reserved(std::make_unique<Eng::Base::Reserved>()), windowId{ 0 },
leftEyeFbo(nullptr), rightEyeFbo(nullptr),
leftEyeTexture(0), rightEyeTexture(0), eyeDistance(0.065f) {
#ifdef _DEBUG
    std::cout << "[+] " << std::source_location::current().function_name() << " invoked" << std::endl;
#endif
}


/**
 * Destructor.
 */
ENG_API Eng::Base::~Base() {
#ifdef _DEBUG
    std::cout << "[-] " << std::source_location::current().function_name() << " invoked" << std::endl;
#endif

    if (leftEyeTexture != 0) {
        glDeleteTextures(1, &leftEyeTexture);
    }
    if (rightEyeTexture != 0) {
        glDeleteTextures(1, &rightEyeTexture);
    }
}


/**
 * Gets a reference to the (unique) singleton instance.
 * @return reference to singleton instance
 */
Eng::Base ENG_API &Eng::Base::getInstance() {
   static Base instance;
   return instance;
}


/**
 * @brief Initializes the graphics engine components
 *
 * This method performs the following initialization steps:
 * - Checks if engine is already initialized
 * - Sets up OpenGL context and configuration
 * - Initializes the callback manager for input handling
 * - Initializes FreeImage library for texture loading
 *
 * @return true if initialization successful, false if engine already initialized
 *         or if any initialization step fails
 *
 * @note Must be called before any other engine operations
 */
bool ENG_API Eng::Base::init() {
   // Already initialized?
   if (reserved->initFlag) {
      std::cout << "ERROR: engine already initialized" << std::endl;
      return false;
   }

   // OpenGL
   if (!initOpenGL()) {
      std::cerr << "ERROR: Failed to initialize OpenGL" << std::endl;
      return false;
   }

   auto &callbackManager = CallbackManager::getInstance();
   if (callbackManager.initialize())
    std::cout << "   CallbackManager initialized successfully!" << std::endl;

   // Init freeimage
   FreeImage_Initialise();
   std::cout << "   FreeImage initialized successfully!" << std::endl;

   // Done:
   reserved->initFlag = true;
   std::cout << "[>] " << LIB_NAME << " initialized" << std::endl;
   return true;
}


/**
 * @brief Releases and cleans up all engine components
 *
 * This method performs the following cleanup steps:
 * - Verifies engine is initialized
 * - Releases OpenGL resources
 * - Shuts down FreeImage library
 * - Resets initialization flag
 *
 * @return true if cleanup successful, false if engine was not initialized
 */
bool ENG_API Eng::Base::free() {
   // Not initialized?
   if (!reserved->initFlag) {
      std::cout << "ERROR: engine not initialized" << std::endl;
      return false;
   }

   freeOpenGL();

   FreeImage_DeInitialise();

   // Done:
   reserved->initFlag = false;
   std::cout << "[<] " << LIB_NAME << " deinitialized" << std::endl;
   return true;
}

/**
 * @brief Initializes OpenVR which links to SteamVR
 *
 * More info in OvVR implementation
 * Also logs tracking device information
 *
 * @return true if OpenVR initialization succeeds or already initialized, false if initialization fails
 */
bool ENG_API Eng::Base::initOpenVR() {
    // Do nothing if already initialized
    if (reserved->ovrReady)
        return true;

    if(!reserved->ovr)
        reserved->ovr = new OvVR();
    if (reserved->ovr->init() == false)
    {
        std::cout << "[ERROR] Unable to init OpenVR" << std::endl;
        delete reserved->ovr;
        return false;
    }

    // Read and store OpenVR ideal FBO sizes
    reserved->fboSizeX = reserved->ovr->getHmdIdealHorizRes();
    reserved->fboSizeY = reserved->ovr->getHmdIdealVertRes();

    // OpenVR device info:
    std::cout << "   Manufacturer . . :  " << reserved->ovr->getManufacturerName() << std::endl;
    std::cout << "   Tracking system  :  " << reserved->ovr->getTrackingSysName() << std::endl;
    std::cout << "   Model number . . :  " << reserved->ovr->getModelNumber() << std::endl;
    std::cout << "   Ideal resolution :  " << reserved->fboSizeX << "x" << reserved->fboSizeY << std::endl;

    return true;
}

/**
 * @brief Initializes OpenGL context and graphics settings
 *
 * Sets up the OpenGL rendering environment with:
 * - Double buffering, RGBA color, and depth buffer
 * - Creates a window for rendering
 * - Enables depth testing, lighting, and normal normalization
 * - Configures light model and background color
 * - Logs OpenGL version information
 *
 * @return true if OpenGL context created successfully, false if context creation fails
 */
bool ENG_API Eng::Base::initOpenGL() {
    int argc = 1;
    char* argv[] = { (char*)"engine" };

    //Glut init
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    // 4.4 context flags
    glutInitContextVersion(4, 4);
    glutInitContextProfile(GLUT_CORE_PROFILE);
#ifdef _DEBUG
    glutInitContextFlags(GLUT_DEBUG); // <-- Debug flag required by the OpenGL debug callback
#endif

    // Set initial window size
    glutInitWindowSize(APP_WINDOWSIZEX, APP_WINDOWSIZEY);
    glutInitWindowPosition(100, 100);

    windowId = glutCreateWindow("Graphics Engine");

    if (!glutGetWindow()) {
        std::cerr << "ERROR: Failed to create OpenGL context" << std::endl;
        return false;
    }

   GLenum err = glewInit();
   if (err != GLEW_OK)
   {
       std::cerr << "[ERROR] " << glewGetErrorString(err) << std::endl;
       return false;
   }
   else
       if (GLEW_VERSION_4_4)
           std::cout << "Driver supports OpenGL 4.4\n" << std::endl;
       else
       {
           std::cout << "[ERROR] OpenGL 4.4 not supported\n" << std::endl;
           return false;
       }

   // Register OpenGL debug callback:
#ifdef _DEBUG
   glDebugMessageCallback((GLDEBUGPROC)DebugCallback, nullptr);
   glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // enable debug notifications
#endif

   //glLightModelf(GL_LIGHT_MODEL_LOCAL_VIEWER, 1.0f);      // 4.4 unsupported
   glm::vec4 ambient = glm::vec4(.6f, .6f, .6f, 1.0f);
   //glLightModelfv(GL_LIGHT_MODEL_AMBIENT, glm::value_ptr(ambient));   // 4.4 unsupported
   glEnable(GL_DEPTH_TEST);     // Enable depth testing
   glDepthFunc(GL_LESS);
   //glEnable(GL_LIGHTING);       // Enable lighting        // 4.4 unsupported
   //glEnable(GL_NORMALIZE);      // Enable normal normalization        // 4.4 unsupported - unnecessary with shaders

   // Enable smooth shading
   //glShadeModel(GL_SMOOTH);     // Not necessary with pixel shading

   // Set the background color for the rendering context
   glClearColor(0.0f, 1.0f, 0.0f, 1.0f); // Light background

   // Add back-face culling
   glEnable(GL_CULL_FACE);  // Enable face culling
   glCullFace(GL_BACK);     // Cull back faces
   glFrontFace(GL_CCW);     // Counter-clockwise front faces

   std::cout << "OpenGL context initialized successfully" << std::endl;

   // Check OpenGL version:
   std::cout << "OpenGL context" << std::endl;
   std::cout << "   version  . . : " << glGetString(GL_VERSION) << std::endl;
   std::cout << "   vendor . . . : " << glGetString(GL_VENDOR) << std::endl;
   std::cout << "   renderer . . : " << glGetString(GL_RENDERER) << std::endl;

   int oglVersion[2];
   glGetIntegerv(GL_MAJOR_VERSION, &oglVersion[0]);
   glGetIntegerv(GL_MINOR_VERSION, &oglVersion[1]);
   std::cout << "   Version  . . :  " << glGetString(GL_VERSION) << " [" << oglVersion[0] << "." << oglVersion[1] << "]" << std::endl;

   int oglContextProfile;
   glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &oglContextProfile);
   if (oglContextProfile & GL_CONTEXT_CORE_PROFILE_BIT)
       std::cout << "                :  " << "Core profile" << std::endl;
   if (oglContextProfile & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT)
       std::cout << "                :  " << "Compatibility profile" << std::endl;

   int oglContextFlags;
   glGetIntegerv(GL_CONTEXT_FLAGS, &oglContextFlags);
   if (oglContextFlags & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT)
       std::cout << "                :  " << "Forward compatible" << std::endl;
   if (oglContextFlags & GL_CONTEXT_FLAG_DEBUG_BIT)
       std::cout << "                :  " << "Debug flag" << std::endl;
   if (oglContextFlags & GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT)
       std::cout << "                :  " << "Robust access flag" << std::endl;
   if (oglContextFlags & GL_CONTEXT_FLAG_NO_ERROR_BIT)
       std::cout << "                :  " << "No error flag" << std::endl;

   std::cout << "   GLSL . . . . :  " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
   std::cout << std::endl;

   return true;
}


/**
 * @brief Cleans up and destroys the OpenGL context
 *
 * Destroys the current FreeGLUT window and releases any associated resources.
 */
void ENG_API Eng::Base::freeOpenGL() {
   glutDestroyWindow(glutGetWindow());
   std::cout << "OpenGL context destroyed" << std::endl;
}


/**
 * @brief Sets the active camera for the engine.
 *
 * The active camera determines the view and projection matrices for rendering.
 *
 * @param camera A shared pointer to the camera to be set as active.
 */
void ENG_API Eng::Base::SetActiveCamera(std::shared_ptr<Eng::Camera> camera) {
   activeCamera = std::move(camera);
}

/**
 * @brief Retrieves the currently active camera.
 *
 * Returns the camera currently in use for rendering, if one has been set.
 *
 * @return std::shared_ptr<Eng::Camera> A shared pointer to the active camera, or nullptr if no camera is set.
 */
std::shared_ptr<Eng::Camera> ENG_API Eng::Base::getActiveCamera() const {
   return activeCamera;
}

/**
 * @brief Renders the entire scene.
 *
 * This method clears the screen, sets up the camera's view and projection matrices,
 * applies lighting, and renders all nodes in the scene graph.
 */

void ENG_API Eng::Base::renderScene() {
    if (engIsEnabled(ENG_STEREO_RENDERING)) {
        renderStereoscopic();
        return;
    }

   if (!activeCamera) {
      std::cerr << "ERROR: No active camera set for rendering" << std::endl;
      return;
   }

   // Clear Buffers
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   // Get View Matrix
   glm::mat4 viewMatrix = activeCamera->getFinalMatrix();

   // Get Projection matrix
   glm::mat4 projectionMatrix = activeCamera->getProjectionMatrix();

   glm::mat4 headWorld = glm::inverse(viewMatrix);
   getHeadNode()->setLocalMatrix(headWorld);

   if (skybox) {
       // Remove the translation component by converting to a 3x3 and back to a 4x4.
       glm::mat4 viewNoTrans = glm::mat4(glm::mat3(viewMatrix));
       skybox->render(viewNoTrans, projectionMatrix);
   }

   // Clear list
   renderList.clear();

   // Execute optional render callbacks
   auto& callbackManager = CallbackManager::getInstance();
   callbackManager.executeRenderCallbacks();

   // Traverse the root nodes and add them to the render list
   traverseAndAddToRenderList(rootNode);

   // Render all nodes in the render list
   renderList.setViewMatrix(viewMatrix);
   renderList.setEyeProjectionMatrix(projectionMatrix);
   renderList.render();


   glutSwapBuffers();
}

/**
 * @brief Recursively traverses the scene graph and adds nodes to the render list.
 *
 * Computes the final transformation matrix for each node and adds it to the render list.
 *
 * @param node The current node being traversed.
 */
void ENG_API Eng::Base::traverseAndAddToRenderList(const std::shared_ptr<Eng::Node>& node) {
    // Compute the final transformation matrix
    const glm::mat4 worldMatrix = node->getFinalMatrix();

    // If the node passed culling (or is not a Mesh), add it.
    renderList.addNode(node, worldMatrix);

    // Recursively process children.
    for (auto& child : *node->getChildren())
        traverseAndAddToRenderList(child);
}

/**
 * @brief Runs the main rendering loop of the engine.
 *
 * Continuously renders the scene and processes events until an exit request is issued.
 */
void ENG_API Eng::Base::run() {
    if (engIsEnabled(ENG_STEREO_RENDERING)) {
        // OpenVR
        reserved->ovrReady = initOpenVR();
        if (!reserved->ovrReady) {
            std::cout << "Falling back to standard mono rendering" << std::endl;
            engDisable(ENG_STEREO_RENDERING);
        }
        else {
            setupStereoscopicRendering(reserved->fboSizeX, reserved->fboSizeY);
        }
    }
   // Enter FreeGLUT main loop
   glutMainLoop();
}

/**
 * @brief Loads a scene from a file.
 *
 * Parses the specified scene file in `.ovo` format and builds the scene graph.
 *
 * @param fileName The name of the file containing the scene description.
 */
void ENG_API Eng::Base::loadScene(const std::string &fileName) {
   Eng::OvoReader reader;
   rootNode = reader.parseOvoFile(fileName);
   std::cout << "Printing scene " << fileName << std::endl;
   reader.printGraph();
   auto& shaderManager = ShaderManager::getInstance();
   if (shaderManager.initialize())
       std::cout << "   ShaderManager initialized successfully!" << std::endl;
   if(renderList.initShaders())
       std::cout << "   Shaders loaded successfully!" << std::endl;
}

/**
 * @brief Retrieves the root node of the scene graph
 *
 * @return std::shared_ptr<Node> The root node of the scene hierarchy
 */

std::shared_ptr<Eng::Node> Eng::Base::getRootNode() {
   return rootNode;
}

/**
 * @brief Calculates the current window aspect ratio
 *
 * Computes the ratio between window width and height. If height is zero,
 * returns 1.0 to avoid division by zero.
 *
 * @return float The window aspect ratio (width/height)
 */

float Eng::Base::getWindowAspectRatio() {
    const int width = engIsEnabled(ENG_STEREO_RENDERING) ? APP_WINDOWSIZEX / 2 : APP_WINDOWSIZEX;
   int height = APP_WINDOWSIZEY;
   if (height == 0) height = 1; // Avoid division by zero
   return static_cast<float>(width) / height;
}
/**
* @brief Enables a specified engine capability.
* @param cap The capability flag to enable
*/
void Eng::Base::engEnable(unsigned int cap) {
    engineState |= cap;
}
/**
* @brief Disables a specified engine capability.
* @param cap The capability flag to disable
*/
void Eng::Base::engDisable(unsigned int cap) {
   engineState &= ~cap;
}

/**
* @brief Checks if a specified capability is enabled.
* @param cap The capability flag to check
* @return true if enabled, false otherwise
*/
bool Eng::Base::engIsEnabled(unsigned int cap) {
   return (engineState & cap) != 0;
}

/**
 * @brief Configura il rendering stereoscopico creando gli FBO necessari
 * @param width Larghezza degli FBO
 * @param height Altezza degli FBO
 */
bool ENG_API Eng::Base::setupStereoscopicRendering(int width, int height) {
    std::cout << "DEBUG: Setting up stereoscopic rendering, requested size: "
        << width << "x" << height << std::endl;

    if (width <= 0 || height <= 0) {
        // Override with predefined constants for consistency
        width = APP_FBOSIZEX;
        height = APP_FBOSIZEY;
        std::cout << "DEBUG: Using fixed FBO size: " << width << "x" << height << std::endl;
    }

    // Clean up existing FBOs if they exist
    if (leftEyeTexture != 0) {
        glDeleteTextures(1, &leftEyeTexture);
        leftEyeTexture = 0;
    }
    if (rightEyeTexture != 0) {
        glDeleteTextures(1, &rightEyeTexture);
        rightEyeTexture = 0;
    }

    leftEyeFbo = std::make_shared<Fbo>();
    rightEyeFbo = std::make_shared<Fbo>();

    // Create texture for left eye
    glGenTextures(1, &leftEyeTexture);
    glBindTexture(GL_TEXTURE_2D, leftEyeTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    std::cout << "DEBUG: Left eye texture created with ID : " << leftEyeTexture << std::endl;

    // Bind texture to left FBO
    leftEyeFbo->bindTexture(0, Fbo::BIND_COLORTEXTURE, leftEyeTexture);
    leftEyeFbo->bindRenderBuffer(1, Fbo::BIND_DEPTHBUFFER, width, height);

    if (!leftEyeFbo->isOk()) {
        std::cerr << "ERROR: Left eye FBO setup failed" << std::endl;
        return false;
    }
    std::cout << "DEBUG: Left eye FBO setup successful, handle: " << leftEyeFbo->getHandle() << ", size " << leftEyeFbo->getSizeX() << "x" << leftEyeFbo->getSizeY() << std::endl;

    // Create texture for right eye
    glGenTextures(1, &rightEyeTexture);
    glBindTexture(GL_TEXTURE_2D, rightEyeTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    std::cout << "DEBUG: Right eye texture created with ID: " << rightEyeTexture << std::endl;

    // Bind texture to right FBO
    rightEyeFbo->bindTexture(0, Fbo::BIND_COLORTEXTURE, rightEyeTexture);
    rightEyeFbo->bindRenderBuffer(1, Fbo::BIND_DEPTHBUFFER, width, height);

    if (!rightEyeFbo->isOk()) {
        std::cerr << "ERROR: Right eye FBO setup failed" << std::endl;
        return false;
    }
    std::cout << "DEBUG: Right eye FBO setup successful, handle: " << rightEyeFbo->getHandle() << ", size " << rightEyeFbo->getSizeX() << "x" << rightEyeFbo->getSizeY() << std::endl;

    // Set default eye distance
    eyeDistance = 0.065f; // 6.5 cm is the average human interpupillary distance
    std::cout << "DEBUG: Eye distance set to " << eyeDistance << std::endl;

    // Reset viewport to default
    Fbo::disable();

    return true;
}
glm::mat4 Eng::Base::computeEyeViewMatrix(const glm::mat4& cameraWorldMatrix, float eyeOffset) {
    //ITA: serve per estrarre la posizione della camera selezionata per poterci applicare l'offset per ogni occhio
    //ITA: come funziona: la view matrix � l'inversa della wrold, (in view coordinates la camera � in 0 0 0)
    //ITA: una volta fatta l'inversa (nel metodo chiamante) la posizione vien eestratta insieme agli altri valori
    //ITA: per applicare l'offset prendo la posizone iniziale in x e la traslo per ottenere left e right eye
    //ITA: ricalcolo poi la lookat matrix usando i valori della camera vecchia estratti ma con l'offset su x
    glm::vec3 cameraPos = glm::vec3(cameraWorldMatrix[3]);
    glm::vec3 cameraRight = glm::vec3(cameraWorldMatrix[0]); // X-axis
    glm::vec3 up = glm::vec3(cameraWorldMatrix[1]);
    glm::vec3 forward = -glm::vec3(cameraWorldMatrix[2]);

    glm::vec3 eyePos = cameraPos + (cameraRight * eyeOffset);
    glm::vec3 target = eyePos + forward;

    return glm::lookAt(eyePos, target, up);
}

void Eng::Base::renderEye(Fbo* eyeFbo, glm::mat4& viewMatrix, glm::mat4& projectionMatrix) {

    eyeFbo->render();
    glViewport(0, 0, eyeFbo->getSizeX(), eyeFbo->getSizeY());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    if (skybox) {
        // Remove the translation component by converting to a 3x3 and back to a 4x4.
        glm::mat4 viewNoTrans = glm::mat4(glm::mat3(viewMatrix));
        skybox->render(viewNoTrans, projectionMatrix);
    }


    renderList.clear();
    traverseAndAddToRenderList(rootNode);
    renderList.setViewMatrix(viewMatrix);
	renderList.setEyeProjectionMatrix(projectionMatrix);
    renderList.render();
}


void ENG_API Eng::Base::renderStereoscopic() {
    if (!leftEyeFbo || !rightEyeFbo) {
        std::cerr << "ERROR: FBOs non inizializzati per il rendering stereoscopico." << std::endl;
        renderScene();
        return;
    }

    int windowWidth = glutGet(GLUT_WINDOW_WIDTH);
    int windowHeight = glutGet(GLUT_WINDOW_HEIGHT);
    OvVR::OvEye leftEye = OvVR::OvEye::EYE_LEFT;    // Use of more simple variable names for readability
    OvVR::OvEye rightEye = OvVR::OvEye::EYE_RIGHT;

    // Update current user position
    reserved->ovr->update();

    // Compute modelViewMatrix based on user VR position
    glm::mat4 headPositionMatrix = reserved->ovr->getModelviewMatrix();
    glm::mat4 modelViewMatrix = glm::inverse(headPositionMatrix);

    // Update the head node with the VR head position
    getHeadNode()->setLocalMatrix(headPositionMatrix);

    // Skybox
    glm::mat4 skyboxView = glm::mat4(glm::mat3(modelViewMatrix));

    glm::mat4 tmpProjMat, eye2Head;

    // CallBackManager for the LeapMotion
    auto& callbackManager = CallbackManager::getInstance();
    callbackManager.executeRenderCallbacks();


    // ---- Left Eye Rendering ----
    {
        // Retrieve the left-eye projection matrix from OpenVR.
        tmpProjMat = reserved->ovr->getProjMatrix(leftEye, STEREO_NEAR_CLIP, STEREO_FAR_CLIP);
        eye2Head = reserved->ovr->getEye2HeadMatrix(leftEye);

        // Computing Left Eye matrix
        glm::mat4 leftEyeProjMat = tmpProjMat * glm::inverse(eye2Head);

        // Render the skybox for the left eye:
        if (skybox) {
            skybox->render(skyboxView, leftEyeProjMat);
        }

        // Render the scene for the left eye.
        renderEye(leftEyeFbo.get(), modelViewMatrix, leftEyeProjMat);
        // Pass the rendered FBO texture to OpenVR.
        reserved->ovr->pass(leftEye, leftEyeTexture);
    }

    // ---- Right Eye Rendering ----
    {
        // Retrieve the right-eye projection matrix from OpenVR.
        tmpProjMat = reserved->ovr->getProjMatrix(rightEye, STEREO_NEAR_CLIP, STEREO_FAR_CLIP);
        eye2Head = reserved->ovr->getEye2HeadMatrix(rightEye);

        // Computing Right Eye matrix
        glm::mat4 rightEyeProjMat = tmpProjMat * glm::inverse(eye2Head);

        // Render the skybox for the right eye:
        if (skybox) {
            skybox->render(skyboxView, rightEyeProjMat);
        }

        // Render the scene for the right eye.
        renderEye(rightEyeFbo.get(), modelViewMatrix, rightEyeProjMat);
        // Pass the rendered FBO texture to OpenVR.
        reserved->ovr->pass(rightEye, rightEyeTexture);
    }

    // Update internal OpenVR settings (sends data to VR System):
    reserved->ovr->render();

    // Reset to standard render output
    Fbo::disable();
    glViewport(0, 0, windowWidth, windowHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Display Left and Right Eye images on standard screen as split screen
    glBindFramebuffer(GL_READ_FRAMEBUFFER, leftEyeFbo->getHandle());
    glBlitFramebuffer(
        0, 0, leftEyeFbo->getSizeX(), leftEyeFbo->getSizeY(),
        0, 0, APP_FBOSIZEX, APP_FBOSIZEY,
        GL_COLOR_BUFFER_BIT, GL_LINEAR
    );

    glBindFramebuffer(GL_READ_FRAMEBUFFER, rightEyeFbo->getHandle());
    glBlitFramebuffer(
        0, 0, rightEyeFbo->getSizeX(), rightEyeFbo->getSizeY(),
        APP_FBOSIZEX, 0, APP_WINDOWSIZEX, APP_FBOSIZEY,
        GL_COLOR_BUFFER_BIT, GL_LINEAR
    );

    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    CallbackManager::getInstance().executeRenderCallbacks();
    glutSwapBuffers();
}

// Skybox
std::shared_ptr <Eng::Skybox> Eng::Base::getSkybox() const {
    return skybox;
}

void Eng::Base::registerSkybox(const std::vector<std::string>& faces) {
    // Create a new Skybox instance if one doesn’t already exist,
    skybox = std::make_shared<Skybox>(faces);

    if (!skybox->init()) {
        std::cerr << "[Base] Skybox initialization failed." << std::endl;
        skybox.reset();
    }
}


// HeadNode for LeapMotion
std::shared_ptr<Eng::Node> Eng::Base::getHeadNode() {
    if (!headNode) {
        // lazily create it and parent under the scene root
        headNode = std::make_shared<Node>();
        headNode->setName("Head");
        rootNode->addChild(headNode);
        headNode->setParent(rootNode.get());
    }
    return headNode;
}
