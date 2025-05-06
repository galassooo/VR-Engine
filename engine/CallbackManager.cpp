#include "engine.h"
#include <GL/freeglut.h>

/**
 * @brief Retrieves the singleton instance of the CallbackManager.
 * @return Reference to the CallbackManager instance.
 */

Eng::CallbackManager &Eng::CallbackManager::getInstance() {
   static Eng::CallbackManager instance;
   return instance;
}

/**
 * @brief Sets the default callback functions for FreeGLUT.
 *
 * Default callbacks include rendering, resizing, input handling, and cleanup.
 * These are linked to the engine's internal logic to provide basic functionality.
 */

void Eng::CallbackManager::setDefaultCallbacks() {
   // Display callback rimane invariato
   displayCallback = []() {
      auto &engine = Base::getInstance();
      engine.renderScene();
   };

   glutDisplayFunc([]() {
      auto &manager = getInstance();
      if (manager.displayCallback) manager.displayCallback();
   });

   glutIdleFunc([]() {
      glutPostRedisplay();
   });

   // Reshape callback rimane invariato
   reshapeCallback = [](int width, int height) {

      auto &engine = Base::getInstance();
      auto activeCamera = engine.getActiveCamera();

      if (activeCamera) {
         if (auto perspectiveCamera = std::dynamic_pointer_cast<PerspectiveCamera>(activeCamera)) {
            perspectiveCamera->setAspect(engine.getWindowAspectRatio());
         }
         glm::mat4 projection = activeCamera->getProjectionMatrix();
         ShaderManager::getInstance().setProjectionMatrix(projection);
      }
   };


   registerKeyBinding('f', "Toggle FPS display", [this](unsigned char key, int x, int y) {
      static bool showFPS = false;
      showFPS = !showFPS;
      if (showFPS) {
         registerRenderCallback("fpsRender", [this]() { calculateFPS(); });
      } else {
         deregisterCallback("fpsRender");
      }
   });

   registerKeyBinding('z', "Toggle wireframe mode", [](unsigned char key, int x, int y) {
      static bool wireframe = false;
      wireframe = !wireframe;
      glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
   });

   registerKeyBinding('b', "Toggle face culling mode", [](unsigned char key, int x, int y) {
       static bool faceCulling = true;
       faceCulling = !faceCulling;

       faceCulling ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
   });

   registerKeyBinding(27, "Exit application", [](unsigned char key, int x, int y) {
      glutLeaveMainLoop();
   });

   // Set up keyboard callback to handle both registered bindings and dynamic callbacks
   glutKeyboardFunc([](unsigned char key, int x, int y) {
      auto &manager = getInstance();

      // First check registered key bindings
      if (manager.keyBindings.contains(key)) {
         manager.keyBindings[key].func(key, x, y);
      }

      // Then process any additional keyboard callbacks
      for (const auto &callback: manager.keyboardCallbacks) {
         callback(key, x, y);
      }
   });

   glutReshapeFunc([](int w, int h) {
      if (const auto &manager = getInstance(); manager.reshapeCallback) manager.reshapeCallback(w, h);
   });
   registerKeyBinding('n', "Toggle normal vectors", [](unsigned char key, int x, int y) {
       if (Eng::Base::engIsEnabled(ENG_RENDER_NORMAL)) {
           Eng::Base::engDisable(ENG_RENDER_NORMAL);
       } else {
           Eng::Base::engEnable(ENG_RENDER_NORMAL);
       }
   });

   registerRenderCallback("helpText", [this]() {

   reshapeCallback = [](int width, int height) {
       auto& engine = Base::getInstance();

       if (engine.engIsEnabled(ENG_STEREO_RENDERING)) {
           if (width != APP_WINDOWSIZEX / 2 || height != APP_WINDOWSIZEY) {
               glutReshapeWindow(APP_WINDOWSIZEX , APP_WINDOWSIZEY);
               return;
           }
       }
       else {
           // Force window size to be constant for stereoscopic rendering
           if (width != APP_WINDOWSIZEX || height != APP_WINDOWSIZEY) {
               glutReshapeWindow(APP_WINDOWSIZEX, APP_WINDOWSIZEY);
               return;
           }
       }
       

       glViewport(0, 0, width, height);

       auto activeCamera = engine.getActiveCamera();

       if (activeCamera) {
           if (auto perspectiveCamera = std::dynamic_pointer_cast<PerspectiveCamera>(activeCamera)) {
               perspectiveCamera->setAspect(engine.getWindowAspectRatio());
           }
           glm::mat4 projection = activeCamera->getProjectionMatrix();
           ShaderManager::getInstance().setProjectionMatrix(projection);
       }
       };

   // Register the reshape callback
   glutReshapeFunc([](int w, int h) {
       if (const auto& manager = getInstance(); manager.reshapeCallback) manager.reshapeCallback(w, h);
       });
});
}

/**
 * @brief Registers a key binding.
 */
void Eng::CallbackManager::registerKeyBinding(char key, const std::string &description, KeyboardFunc func) {
   KeyInfo info{key, description, func};
   keyBindings[key] = info;
}

/**
 * @brief Registers the current callbacks with FreeGLUT.
 * @return Returns true if initialized correctly, false if already initualized or an error occurred.
 */

bool Eng::CallbackManager::initialize() {
   if (initialized) {
      std::cerr << "CallbackManager::initialize() already called. Skipping reinitialization." << std::endl;
      return false;
   }

   setDefaultCallbacks();
   initialized = true;
   return true;
}

/**
 * @brief Sets a custom display callback.
 * @param func The custom callback function for rendering the scene.
 */

void Eng::CallbackManager::setDisplayCallback(DisplayFunc func) {
   displayCallback = std::move(func);
   glutDisplayFunc([]() {
      if (const auto &manager = getInstance(); manager.displayCallback) manager.displayCallback();
   });
}

/**
 * @brief Registers a render callback with a unique identifier.
 *
 * This method allows registering a rendering callback function associated with a specific identifier.
 * The callback will be executed during the rendering process if registered.
 *
 * @param id A unique string identifier for the callback.
 * @param callback The render callback function to be registered.
 */

void Eng::CallbackManager::registerRenderCallback(std::string id, const RenderCallBack &callback) {
   optionalRenderCallbacks[id] = callback;
}

/**
 * @brief Executes all registered render callbacks.
 *
 * Iterates over all registered render callbacks and executes them in the order of registration.
 * This method is typically called during the rendering loop to execute optional rendering logic.
 */

void Eng::CallbackManager::executeRenderCallbacks() const {
   for (const auto &[id, callback]: optionalRenderCallbacks) {
      callback();
   }
}

/**
 * @brief Deregisters a render callback using its identifier.
 *
 * Removes a previously registered render callback from the internal storage, preventing it
 * from being executed during the rendering process.
 *
 * @param id The unique string identifier of the callback to be deregistered.
 */

void Eng::CallbackManager::deregisterCallback(const std::string &id) {
   optionalRenderCallbacks.erase(id);
}


/**
 * @brief Sets a custom reshape callback.
 * @param func The custom callback function for handling window resizing.
 */

void Eng::CallbackManager::setReshapeCallback(ReshapeFunc func) {
   reshapeCallback = std::move(func);
   glutReshapeFunc([](int w, int h) {
      auto &manager = getInstance();
      if (manager.reshapeCallback) manager.reshapeCallback(w, h);
   });
}

/**
 * @brief Sets a custom special key callback.
 * @param func The custom callback function for special key input.
 */

void Eng::CallbackManager::setSpecialCallback(SpecialFunc func) {
   specialCallback = std::move(func);
   glutSpecialFunc([](int key, int x, int y) {
      auto &manager = getInstance();
      if (manager.specialCallback) manager.specialCallback(key, x, y);
   });
}

/**
 * @brief Sets a custom close callback.
 * @param func The custom callback function for application cleanup.
 */

void Eng::CallbackManager::setCloseCallback(CloseFunc func) {
   closeCallback = std::move(func);
   glutCloseFunc([]() {
      if (const auto &manager = getInstance(); manager.closeCallback) manager.closeCallback();
   });
}


/**
 * @brief Renders the FPS counter on screen.
 *
 * This method handles the rendering of the frames per second counter in the top-left corner
 * of the window. It temporarily modifies the OpenGL state to render 2D text, including:
 * - Saving and restoring projection/modelview matrices
 * - Disabling lighting and depth testing during text rendering
 * - Setting up an orthographic projection for 2D rendering
 * - Drawing the FPS value using GLUT bitmap characters
 *
 * The FPS value is displayed in white text using the HELVETICA_12 font.
 */

void ENG_API Eng::CallbackManager::calculateFPS() {
   // Data for FPS counter
   static int frameCount;
   static float fps;
   static float lastFpsUpdateTime;

   frameCount++;
   const float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

   if (const float elapsed = currentTime - lastFpsUpdateTime; elapsed >= 1.0f) {
       //each second
       fps = static_cast<float>(frameCount) / elapsed;
       frameCount = 0;
       lastFpsUpdateTime = currentTime;
       std::cout << "[FPS] " << fps << std::endl;
   }
}
