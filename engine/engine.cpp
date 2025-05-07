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
#include <chrono>
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
#ifdef _DEBUG
    //std::cout << "OpenGL says: \"" << std::string(message) << "\"" << std::endl;
#endif
}


// ------------------ Base::Reserved ------------------
/**
 * @brief PIMPL reserved data for Base.
 *
 * Holds initialization flags, OpenVR interface, and FBO sizes.
 */
struct Eng::Base::Reserved {
    // Flags:
    bool initFlag;          ///< Engine initialized flag
    bool ovrReady;          ///< OpenVR initialized flag

    // OpenVR interface:
    OvVR* ovr;              ///< OpenVR interface pointer
    int fboSizeX;           ///< VR framebuffer width
    int fboSizeY;           ///< VR framebuffer height

    /**
    * @brief Constructs Reserved with default flags.
    */
    Reserved() : initFlag{ false }, ovrReady{ false }, ovr{ nullptr }, fboSizeX{ 0 }, fboSizeY{ 0 } {
    }

    /**
     * @brief Destructs Reserved, freeing OpenVR.
     */
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


// ------------------ Base ------------------
/**
 * @brief Constructs the graphics engine Base.
 *
 * Initializes internal PIMPL and default members.
 */
ENG_API Eng::Base::Base() : reserved(std::make_unique<Eng::Base::Reserved>()), windowId{ 0 },
leftEyeFbo(nullptr), rightEyeFbo(nullptr),
leftEyeTexture(0), rightEyeTexture(0), eyeDistance(0.065f) {
#ifdef _DEBUG
    std::cout << "[+] " << std::source_location::current().function_name() << " invoked" << std::endl;
#endif
}


/**
 * @brief Destroys the graphics engine Base.
 *
 * Cleans up textures and associated resources.
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
Eng::Base ENG_API& Eng::Base::getInstance() {
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

    auto& callbackManager = CallbackManager::getInstance();
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

    if (!reserved->ovr)
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

    glm::vec4 ambient = glm::vec4(.6f, .6f, .6f, 1.0f);
    glEnable(GL_DEPTH_TEST);     // Enable depth testing
    glDepthFunc(GL_LESS);


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
 * @brief Renders the entire scene, with optional stereoscopic or post-processing.
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

    // Se ci sono post-processor attivi, dobbiamo renderizzare su un FBO
    bool usePostProcessing = isPostProcessingEnabled() &&
        PostProcessorManager::getInstance().getPostProcessorCount() > 0;

    GLuint sceneTexture = 0;
    GLuint outputTexture = 0;
    GLuint sceneFBO = 0;
    GLuint depthRBO = 0;

    if (usePostProcessing) {
        // Crea texture e FBO per la scena
        glGenTextures(1, &sceneTexture);
        glBindTexture(GL_TEXTURE_2D, sceneTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, APP_WINDOWSIZEX, APP_WINDOWSIZEY, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glGenTextures(1, &outputTexture);
        glBindTexture(GL_TEXTURE_2D, outputTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, APP_WINDOWSIZEX, APP_WINDOWSIZEY, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glGenFramebuffers(1, &sceneFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneTexture, 0);

        // Attach depth buffer
        glGenRenderbuffers(1, &depthRBO);
        glBindRenderbuffer(GL_RENDERBUFFER, depthRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, APP_WINDOWSIZEX, APP_WINDOWSIZEY);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRBO);

        // Render to FBO
        glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
    }

    // Clear the buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Get matrices
    glm::mat4 viewMatrix = activeCamera->getFinalMatrix();
    glm::mat4 projectionMatrix = activeCamera->getProjectionMatrix();

    // Set head node transform
    glm::mat4 headWorld = glm::inverse(viewMatrix);
    getHeadNode()->setLocalMatrix(headWorld);

    // Render skybox if present
    if (skybox) {
        glm::mat4 viewNoTrans = glm::mat4(glm::mat3(viewMatrix));
        skybox->render(viewNoTrans, projectionMatrix);
    }

    // Clear and prepare render list
    renderList.clear();
    auto& callbackManager = CallbackManager::getInstance();
    callbackManager.executeRenderCallbacks();

    // Build scene
    traverseAndAddToRenderList(rootNode);
    if (!sceneBoundingBox) {
        sceneBoundingBox = renderList.getSceneBoundingBox();
        stereoFarClip = glm::length(sceneBoundingBox->getSize()) * 2;
    }

    // Render scene
    renderList.setEyeViewMatrix(viewMatrix);
    renderList.setEyeProjectionMatrix(projectionMatrix);
    renderPipeline.runOn(&renderList);

    // Apply post-processing if enabled
    if (usePostProcessing) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        PostProcessorManager::getInstance().applyPostProcessing(sceneTexture, outputTexture, APP_WINDOWSIZEX, APP_WINDOWSIZEY);

        // Render the final result to the screen
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Simple fullscreen quad rendering
        static GLuint quadVAO = 0;
        if (quadVAO == 0) {
            // Create a full-screen quad
            float quadVertices[] = {
                // positions        // texture coords
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
                 1.0f,  1.0f, 0.0f, 1.0f, 1.0f
            };

            GLuint quadVBO;
            glGenVertexArrays(1, &quadVAO);
            glGenBuffers(1, &quadVBO);
            glBindVertexArray(quadVAO);
            glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        }

        // Use a simple shader to display the final texture
        static std::shared_ptr<Program> displayProgram = nullptr;
        if (!displayProgram) {
            const char* vsCode = R"(
            #version 440 core
            layout (location = 0) in vec3 aPos;
            layout (location = 1) in vec2 aTexCoords;
            out vec2 TexCoords;
            void main() {
                TexCoords = aTexCoords;
                gl_Position = vec4(aPos, 1.0);
            }
            )";

            const char* fsCode = R"(
            #version 440 core
            out vec4 FragColor;
            in vec2 TexCoords;
            uniform sampler2D screenTexture;
            void main() {
                FragColor = texture(screenTexture, TexCoords);
            }
            )";

            std::shared_ptr<VertexShader> vs = std::make_shared<VertexShader>();
            vs->load(vsCode);

            std::shared_ptr<FragmentShader> fs = std::make_shared<FragmentShader>();
            fs->load(fsCode);

            displayProgram = std::make_shared<Program>();
            displayProgram->bindAttribute(0, "aPos");
            displayProgram->bindAttribute(1, "aTexCoords");
            displayProgram->bindSampler(0, "screenTexture");
            displayProgram->addShader(vs).addShader(fs).build();
        }

        displayProgram->render();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, outputTexture);
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Cleanup
        glDeleteTextures(1, &sceneTexture);
        glDeleteTextures(1, &outputTexture);
        glDeleteFramebuffers(1, &sceneFBO);
        glDeleteRenderbuffers(1, &depthRBO);
    }

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
    // Base::run()
    if (engIsEnabled(ENG_STEREO_RENDERING))
    {
        reserved->ovrReady = initOpenVR();
        if (!reserved->ovrReady)
        {
            engDisable(ENG_STEREO_RENDERING);
            PostProcessorManager::getInstance()
                .initializeAll(APP_WINDOWSIZEX, APP_WINDOWSIZEY);
        }
        else
        {
            setupStereoscopicRendering(reserved->fboSizeX, reserved->fboSizeY);
            PostProcessorManager::getInstance()
                .initializeAll(stereoRenderWidth, stereoRenderHeight);
        }
    }
    else
    {
        PostProcessorManager::getInstance()
            .initializeAll(APP_WINDOWSIZEX, APP_WINDOWSIZEY);
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
void ENG_API Eng::Base::loadScene(const std::string& fileName) {
    Eng::OvoReader reader;
    rootNode = reader.parseOvoFile(fileName);
    std::cout << "Printing scene " << fileName << std::endl;
    reader.printGraph();
    auto& shaderManager = ShaderManager::getInstance();
    if (shaderManager.initialize())
        std::cout << "   ShaderManager initialized successfully!" << std::endl;
    if (renderPipeline.init())
        std::cout << "   Render pipeline and shaders loaded successfully!" << std::endl;
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
 * @brief Configures stereoscopic rendering by creating and binding the required FBOs.
 *
 * @param width  Width of the FBOs.
 * @param height Height of the FBOs.
 * @return True if both left and right eye FBOs were set up successfully; false on error.
 */
bool ENG_API Eng::Base::setupStereoscopicRendering(int width, int height) {

    if (width <= 0 || height <= 0) {
        // Override with predefined constants for consistency
        width = APP_FBOSIZEX;
        height = APP_FBOSIZEY;
    }

    // Store these dimensions for later use
    stereoRenderWidth = width;
    stereoRenderHeight = height;

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

    // Create texture for left eye with HDR format
    glGenTextures(1, &leftEyeTexture);
    glBindTexture(GL_TEXTURE_2D, leftEyeTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


    // Bind texture to left FBO
    leftEyeFbo->bindTexture(0, Fbo::BIND_COLORTEXTURE, leftEyeTexture);
    leftEyeFbo->bindRenderBuffer(1, Fbo::BIND_DEPTHBUFFER, width, height);

    if (!leftEyeFbo->isOk()) {
        std::cerr << "ERROR: Left eye FBO setup failed" << std::endl;
        return false;
    }

    // Create texture for right eye with HDR format
    glGenTextures(1, &rightEyeTexture);
    glBindTexture(GL_TEXTURE_2D, rightEyeTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


    // Bind texture to right FBO
    rightEyeFbo->bindTexture(0, Fbo::BIND_COLORTEXTURE, rightEyeTexture);
    rightEyeFbo->bindRenderBuffer(1, Fbo::BIND_DEPTHBUFFER, width, height);

    if (!rightEyeFbo->isOk()) {
        std::cerr << "ERROR: Right eye FBO setup failed" << std::endl;
        return false;
    }

    // Set default eye distance
    eyeDistance = 0.065f; // 6.5 cm is the average human interpupillary distance

    // Reset viewport to default
    Fbo::disable();

    // Initialize post-processors for stereoscopic rendering
    // Important: Use the actual FBO dimensions, not window dimensions
    PostProcessorManager::getInstance().initializeAll(width, height);

    return true;
}

/**
 * @brief Computes an eye-offset view matrix for stereoscopic rendering.
 *
 * Takes the camera’s world-space transform, applies a lateral eyeOffset,
 * and builds a corresponding lookAt matrix.
 *
 * @param cameraWorldMatrix  The full camera world-space matrix.
 * @param eyeOffset          Horizontal offset for this eye (±IPD/2).
 * @return A view matrix for that eye.
 */
glm::mat4 Eng::Base::computeEyeViewMatrix(const glm::mat4& cameraWorldMatrix, float eyeOffset) {
    // Extract camera properties from the world matrix
    glm::vec3 cameraPos = glm::vec3(cameraWorldMatrix[3]);
    glm::vec3 cameraRight = glm::vec3(cameraWorldMatrix[0]); // X-axis
    glm::vec3 up = glm::vec3(cameraWorldMatrix[1]);
    glm::vec3 forward = -glm::vec3(cameraWorldMatrix[2]);

    // Apply eye offset along the right vector
    glm::vec3 eyePos = cameraPos + (cameraRight * eyeOffset);
    glm::vec3 target = eyePos + forward;

    // Create a lookAt matrix for the eye
    return glm::lookAt(eyePos, target, up);
}

/**
 * @brief Renders the scene into one eye’s FBO.
 *
 * Binds the given FBO, clears it, sets up the render list with the
 * provided view/projection matrices, and invokes the deferred renderer.
 *
 * @param eyeFbo         Pointer to the eye’s FBO.
 * @param viewMatrix     Eye-space view matrix.
 * @param projectionMatrix  Eye-space projection matrix.
 */
void Eng::Base::renderEye(Fbo* eyeFbo, glm::mat4& viewMatrix, glm::mat4& projectionMatrix) {
    eyeFbo->render();
    static const GLenum drawBufs[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBufs);
    glViewport(0, 0, eyeFbo->getSizeX(), eyeFbo->getSizeY());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set up the render list with view matrix and projection matrix
    renderList.clear();
    traverseAndAddToRenderList(rootNode);
    if (!sceneBoundingBox) {
        sceneBoundingBox = renderList.getSceneBoundingBox();
        stereoFarClip = glm::length(sceneBoundingBox->getSize()) * 2;
    }
    renderList.setEyeViewMatrix(viewMatrix);
    renderList.setEyeProjectionMatrix(projectionMatrix);

    // Pass the current FBO to the render list
    renderList.setCurrentFBO(eyeFbo);

	// Run render pipeline on the render list
    renderPipeline.runOn(&renderList);
}

/**
 * @brief Sets the initial body/world transform for stereoscopic rendering.
 *
 * @param position  4×4 transform matrix of the user’s body in world space.
 */
void ENG_API Eng::Base::setBodyPosition(const glm::mat4& position) {

    stereoInitialTransform = position;
}

/**
 * @brief Retrieves the initial body/world transform.
 *
 * @return The 4×4 matrix last set by setBodyPosition().
 */
glm::mat4 ENG_API Eng::Base::getBodyPosition() const {
    return stereoInitialTransform;
}

/**
 * @brief Renders the scene stereoscopically to both eyes (VR headset).
 *
 * Updates VR tracking, computes per-eye view/projection, renders each eye’s FBO,
 * and then either applies post-processing or submits directly to the HMD. Finally
 * blits a mirror view to the desktop window.
 */
void Eng::Base::renderStereoscopic() {
    // Check FBOs initialization
    if (!leftEyeFbo || !rightEyeFbo) {
        std::cerr << "ERROR: FBOs not initialized for stereoscopic rendering." << std::endl;
        renderScene();
        return;
    }

    // Save current viewport
    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    const int windowWidth = glutGet(GLUT_WINDOW_WIDTH);
    const int windowHeight = glutGet(GLUT_WINDOW_HEIGHT);

    OvVR::OvEye eyeLeft = OvVR::OvEye::EYE_LEFT;
    OvVR::OvEye eyeRight = OvVR::OvEye::EYE_RIGHT;

    // Update VR tracking
    reserved->ovr->update();

    // Calculate head and view transformation
    glm::mat4 headPos = reserved->ovr->getModelviewMatrix();
    glm::mat4 initT = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, stereoEyeHeight, 0.0f)) * stereoInitialTransform;
    glm::mat4 finalHead = initT * headPos;
    glm::mat4 modelView = glm::inverse(finalHead);

    if (auto hn = getHeadNode()) {
        hn->setLocalMatrix(finalHead);
    }

    auto& cbMgr = CallbackManager::getInstance();

    // Get actual FBO dimensions from the FBO objects
    const int actualFboWidth = leftEyeFbo->getSizeX();
    const int actualFboHeight = leftEyeFbo->getSizeY();

    if (actualFboWidth != stereoRenderWidth || actualFboHeight != stereoRenderHeight) {
        std::cout << "WARNING: FBO dimensions mismatch. Stored: " << stereoRenderWidth << "x" << stereoRenderHeight
            << ", Actual: " << actualFboWidth << "x" << actualFboHeight << std::endl;
        // Update stored dimensions
        stereoRenderWidth = actualFboWidth;
        stereoRenderHeight = actualFboHeight;
    }

    // Create textures for post-processing if needed
    auto ensureTexture = [](GLuint& tex, int width, int height) {
        if (tex == 0) {
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
        return tex;
        };

    static GLuint leftEyePostTex = 0;
    static GLuint rightEyePostTex = 0;

    // Ensure post-processing textures exist with correct dimensions
    leftEyePostTex = ensureTexture(leftEyePostTex, stereoRenderWidth, stereoRenderHeight);
    rightEyePostTex = ensureTexture(rightEyePostTex, stereoRenderWidth, stereoRenderHeight);

    // Render a single eye
    auto renderEye = [&](OvVR::OvEye eye,
        std::shared_ptr<Fbo>& eyeFbo,
        GLuint eyeTexture,
        GLuint postTexture)
        {
            // Bind FBO and clear
            eyeFbo->render();
            glViewport(0, 0, stereoRenderWidth, stereoRenderHeight);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Eye-specific projection and view matrices
			glm::mat4 projEye = reserved->ovr->getProjMatrix(eye, stereoNearClip, stereoFarClip); // This is the projection without the eye offset, correct for the skybox
            glm::mat4 eye2Head = reserved->ovr->getEye2HeadMatrix(eye);
            glm::mat4 viewEye = modelView;
			glm::mat4 projEyeFix = projEye * glm::inverse(eye2Head); // This is the projection with the specific eye offset

            // Render skybox
            if (skybox) {
                glm::mat4 skyV = glm::mat4(glm::mat3(viewEye));
				skybox->render(skyV, projEye); // Using projEye for the skybox, so it looks infinitively far away
            }

            // Execute registered rendering callbacks
            cbMgr.executeRenderCallbacks();

            // Build and render scene list
            renderList.clear();
            traverseAndAddToRenderList(rootNode);

            if (!sceneBoundingBox) {
                sceneBoundingBox = renderList.getSceneBoundingBox();
                stereoFarClip = glm::length(sceneBoundingBox->getSize()) * 2;
            }

            renderList.setEyeViewMatrix(viewEye);
            renderList.setEyeProjectionMatrix(projEyeFix);
            renderList.setCurrentFBO(eyeFbo.get());
            renderPipeline.runOn(&renderList);


            // Apply post-processing if enabled
            if (PostProcessorManager::getInstance().isPostProcessingEnabled() &&
                PostProcessorManager::getInstance().getPostProcessorCount() > 0) {

                // Pass the ACTUAL FBO dimensions
                PostProcessorManager::getInstance().applyPostProcessing(eyeTexture, postTexture,
                    stereoRenderWidth, stereoRenderHeight);

                // Submit post-processed frame to VR headset
                reserved->ovr->pass(eye, postTexture);
            }
            else {

                // Submit unprocessed frame to VR headset
                reserved->ovr->pass(eye, eyeTexture);
            }
        };

    // Render left and right eye
    renderEye(eyeLeft, leftEyeFbo, leftEyeTexture, leftEyePostTex);
    renderEye(eyeRight, rightEyeFbo, rightEyeTexture, rightEyePostTex);

    // Submit frames to VR runtime
    reserved->ovr->render();
    Fbo::disable();

    // Render mirror view to monitor
    glViewport(0, 0, windowWidth, windowHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    static GLuint mirrorFbo = 0;
    if (mirrorFbo == 0) glGenFramebuffers(1, &mirrorFbo);

    // Select the appropriate texture based on post-processing
    GLuint leftDisplayTex = PostProcessorManager::getInstance().isPostProcessingEnabled() ? leftEyePostTex : leftEyeTexture;
    GLuint rightDisplayTex = PostProcessorManager::getInstance().isPostProcessingEnabled() ? rightEyePostTex : rightEyeTexture;


    auto blitToScreen = [&](GLuint tex, int x0, int x1) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFbo);
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, tex, 0);
        glReadBuffer(GL_COLOR_ATTACHMENT0);

        glBlitFramebuffer(0, 0, stereoRenderWidth, stereoRenderHeight,
            x0, 0, x1, APP_FBOSIZEY,
            GL_COLOR_BUFFER_BIT, GL_LINEAR);
        };

    blitToScreen(leftDisplayTex, 0, APP_FBOSIZEX);
    blitToScreen(rightDisplayTex, APP_FBOSIZEX, APP_WINDOWSIZEX);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glutSwapBuffers();

    // Restore previous viewport
    glViewport(prevViewport[0], prevViewport[1],
        prevViewport[2], prevViewport[3]);
}

/**
 * @brief Retrieves the currently registered skybox.
 *
 * @return Shared pointer to the Skybox, or nullptr if none is registered.
 */
std::shared_ptr <Eng::Skybox> Eng::Base::getSkybox() const {
    return skybox;
}


/**
 * @brief Creates and registers a skybox from 6 face textures.
 *
 * @param faces  Filenames (in order) for the 6 cubemap faces.
 */
void Eng::Base::registerSkybox(const std::vector<std::string>& faces) {
    // Create a new Skybox instance if one doesn’t already exist,
    skybox = std::make_shared<Skybox>(faces);

    if (!skybox->init()) {
        std::cerr << "[Base] Skybox initialization failed." << std::endl;
        skybox.reset();
    }

    renderList.setGlobalLightColor(skybox->getGlobalColor());
}

/**
 * @brief Adds a post-processor to the pipeline.
 *
 * @param postProcessor  Shared pointer to a PostProcessor instance.
 * @return True if successfully added; false on name collision.
 */
bool ENG_API Eng::Base::addPostProcessor(std::shared_ptr<PostProcessor> postProcessor) {
    return PostProcessorManager::getInstance().addPostProcessor(postProcessor);
}

/**
 * @brief Removes a named post-processor from the pipeline.
 *
 * @param name  Unique name of the post-processor to remove.
 * @return True if found & removed; false if not present.
 */
bool ENG_API Eng::Base::removePostProcessor(const std::string& name) {
    return PostProcessorManager::getInstance().removePostProcessor(name);
}

/**
 * @brief Retrieves a named post-processor.
 *
 * @param name  Unique name of the post-processor.
 * @return Shared pointer to the PostProcessor, or nullptr if not found.
 */
std::shared_ptr<Eng::PostProcessor> ENG_API Eng::Base::getPostProcessor(const std::string& name) {
    return PostProcessorManager::getInstance().getPostProcessor(name);
}

/**
 * @brief Enables or disables post-processing in the pipeline.
 *
 * @param enabled  True to enable post-processing, false to bypass.
 */
void ENG_API Eng::Base::setPostProcessingEnabled(bool enabled) {
    PostProcessorManager::getInstance().setPostProcessingEnabled(enabled);
}

/**
 * @brief Queries whether post-processing is currently enabled.
 *
 * @return True if enabled, false otherwise.
 */
bool ENG_API Eng::Base::isPostProcessingEnabled() const {
    return PostProcessorManager::getInstance().isPostProcessingEnabled();
}

/**
 * @brief Retrieves or lazily creates the head node in the scene graph.
 *
 * The head node is used as the parent transform for VR/Leap-derived head motion.
 *
 * @return Shared pointer to the head node.
 */
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
