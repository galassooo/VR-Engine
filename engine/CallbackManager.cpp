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
         /* unsupported 4.4
         glMatrixMode(GL_PROJECTION);
         glLoadMatrixf(glm::value_ptr(projection));
         glMatrixMode(GL_MODELVIEW);
         */
         ShaderManager::getInstance().setProjectionMatrix(projection);
      }
   };

   // Register default key bindings
   registerKeyBinding('x', "Toggle coordinate axes", [this](unsigned char key, int x, int y) {
      static bool showAxes = false;
      showAxes = !showAxes;
      if (showAxes) {
         registerRenderCallback("axisRender", [this]() { renderCoordinateAxes(); });
      } else {
         deregisterCallback("axisRender");
      }
   });

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

   registerKeyBinding('i', "Toggle help menu", [this](unsigned char key, int x, int y) {
      showHelpMenu = !showHelpMenu;
      if (showHelpMenu) {
         registerRenderCallback("helpMenu", [this]() { renderHelpMenu(); });
      } else {
         deregisterCallback("helpMenu");
      }
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
   renderInfoBar();

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
 * @brief Renders coordinate axes with arrows in the bottom right corner of the screen
 *
 * Draws a 3D coordinate system indicator showing X (red), Y (green), and Z (blue) axes
 * with directional arrows and labels. The axes follow the camera rotation to maintain
 * proper orientation feedback.
 *
 * The rendering process:
 * 1. Saves and sets up OpenGL state for 2D overlay rendering
 * 2. Positions the axes in bottom right corner with scaling
 * 3. Extracts and applies inverse camera rotation to match view orientation
 * 4. Renders the colored axes lines
 * 5. Adds arrow cones at the end of each axis
 * 6. Adds X, Y, Z labels in white
 * 7. Restores original OpenGL state
 *
 * The function uses glPushAttrib/glPopAttrib to preserve OpenGL state and
 * temporarily disables depth testing and lighting for overlay rendering.
 */

void ENG_API Eng::CallbackManager::renderCoordinateAxes() {
    /* Unsupported 4.4
   glPushAttrib(GL_ALL_ATTRIB_BITS);
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();

   glDisable(GL_DEPTH_TEST);
   glDisable(GL_LIGHTING);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-1, 1, -1, 1, -1, 1);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   //place arrows bottom right
   glTranslatef(0.7f, -0.7f, 0.0f);
   glScalef(0.15f, 0.15f, 0.15f);

   //estrai matrice
   glm::mat4 viewMatrix = Base::getInstance().getActiveCamera()->getViewMatrix();
   glm::mat3 rotationMatrix(viewMatrix);
   glm::mat3 inverseRotation = glm::transpose(rotationMatrix); //inverto rotazione
   glMultMatrixf(glm::value_ptr(glm::mat4(inverseRotation)));

   //draw axis
   glLineWidth(2.0f);
   glBegin(GL_LINES);
   // X - Red
   glColor3f(1.0f, 0.0f, 0.0f);
   glVertex3f(0.0f, 0.0f, 0.0f);
   glVertex3f(1.0f, 0.0f, 0.0f);

   // Y - Green
   glColor3f(0.0f, 1.0f, 0.0f);
   glVertex3f(0.0f, 0.0f, 0.0f);
   glVertex3f(0.0f, 1.0f, 0.0f);

   // Z - Blue
   glColor3f(0.0f, 0.0f, 1.0f);
   glVertex3f(0.0f, 0.0f, 0.0f);
   glVertex3f(0.0f, 0.0f, 1.0f);
   glEnd();

   //DROW ARROWS

   glPushMatrix();
   // Arrow X
   glTranslatef(0.8f, 0.0f, 0.0f); //move to x
   glRotatef(90, 0, 1, 0); //rotate position on x
   glColor3f(1.0f, 0.0f, 0.0f);
   glutSolidCone(0.08f, 0.2f, 16, 16);
   glPopMatrix();

   glPushMatrix();

   //arrow Y
   glTranslatef(0.0f, 0.8f, 0.0f);
   glRotatef(-90, 1, 0, 0); //ruoto x allinearmi con l'asse
   glColor3f(0.0f, 1.0f, 0.0f);
   glutSolidCone(0.08f, 0.2f, 16, 16);
   glPopMatrix();

   glPushMatrix();

   // arrow Z
   glTranslatef(0.0f, 0.0f, 0.8f); //posizione alla fine asse Z
   glColor3f(0.0f, 0.0f, 1.0f); //no rotazione, gia allienato
   glutSolidCone(0.08f, 0.2f, 16, 16);
   glPopMatrix();

   //labels
   glColor3f(1.0f, 1.0f, 1.0f);
   glRasterPos3f(1.2f, 0.0f, 0.0f);
   glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, 'X');
   glRasterPos3f(0.0f, 1.2f, 0.0f);
   glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, 'Y');
   glRasterPos3f(0.0f, 0.0f, 1.2f);
   glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, 'Z');

   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();
   glPopAttrib();
   */
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
   }

   //show fps counter
   /* Unsupported 4.4
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glLoadIdentity();
   glOrtho(0, glutGet(GLUT_WINDOW_WIDTH), 0, glutGet(GLUT_WINDOW_HEIGHT), -1, 1);

   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glLoadIdentity();

   glDisable(GL_LIGHTING);
   glDisable(GL_DEPTH_TEST);

   glColor3f(1.0f, 1.0f, 1.0f);
   glRasterPos2i(10, glutGet(GLUT_WINDOW_HEIGHT) - 20);

   std::string fpsText = "FPS: " + std::to_string(static_cast<int>(fps));
   for (char c: fpsText) {
      glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
   }

   glEnable(GL_LIGHTING);
   glEnable(GL_DEPTH_TEST);

   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();
   */
}

/**
 * @brief Renders an overlay help menu displaying all keyboard controls
 *
 * When enabled, displays a semi-transparent fullscreen overlay showing all registered
 * keyboard shortcuts and their descriptions. The menu includes:
 * - A centered title "Keyboard Controls"
 * - List of all registered key bindings and their functions
 * - Footer text with instructions to close the menu
 *
 * The rendering process:
 * 1. Saves OpenGL state and sets up 2D orthographic projection
 * 2. Disables 3D rendering features (lighting and depth testing)
 * 3. Draws a semi-transparent black background (alpha = 0.8)
 * 4. Renders white text for title, key bindings, and footer
 * 5. Special handling for ESC and ENTER keys in the display
 * 6. Restores original OpenGL state
 *
 * The menu is only rendered if showHelpMenu flag is true. Key bindings are
 * retrieved from the keyBindings map and displayed in a formatted list.
 * Text is rendered using GLUT bitmap fonts (Helvetica 18 for title, 12 for content).
 */

void Eng::CallbackManager::renderHelpMenu() {
   if (!showHelpMenu) return;
   /* Unsupported 4.4
   // Save OpenGL state
   glPushAttrib(GL_ALL_ATTRIB_BITS);
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glLoadIdentity();
   glOrtho(0, glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT), 0, -1, 1);

   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glLoadIdentity();

   // Disable 3D rendering features
   glDisable(GL_LIGHTING);
   glDisable(GL_DEPTH_TEST);
   glDisable(GL_CULL_FACE);

   // Draw semi-transparent background
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glColor4f(0.0f, 0.0f, 0.0f, .8f);
   glBegin(GL_QUADS);
   glVertex2i(0, 0);
   glVertex2i(glutGet(GLUT_WINDOW_WIDTH), 0);
   glVertex2i(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
   glVertex2i(0, glutGet(GLUT_WINDOW_HEIGHT));
   glEnd();
   glDisable(GL_BLEND);

   // Draw help menu content
   glColor3f(1.0f, 1.0f, 1.0f);
   int startY = 50;
   int lineHeight = 20;

   // Draw title
   std::string title = "Keyboard Controls";
   glRasterPos2i(glutGet(GLUT_WINDOW_WIDTH) / 2 - 70, startY);
   for (char c: title) {
      glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
   }

   // Draw key bindings
   startY += lineHeight * 2;
   for (const auto &[key, info]: keyBindings) {
      std::string keyChar = (key == 27) ? "ESC" : (key == 13) ? "ENTER" : std::string(1, key);
      std::string text = keyChar + " - " + info.desc;
      glRasterPos2i(glutGet(GLUT_WINDOW_WIDTH) / 4, startY);
      for (char c: text) {
         glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
      }
      startY += lineHeight;
   }

   // Draw footer
   std::string footer = "Press I again to close this menu";
   glRasterPos2i(glutGet(GLUT_WINDOW_WIDTH) / 2 - 100, startY + lineHeight * 2);
   for (char c: footer) {
      glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
   }

   // Restore OpenGL state
   glEnable(GL_CULL_FACE);
   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();
   glPopAttrib();
   */
}

/**
 * @brief Renders a semi-transparent info bar with centered text.
 *
 * Displays an informational message ("Press I to toggle info menu") at the top
 * of the OpenGL window. The function uses a 2D orthographic projection and
 * temporarily disables 3D rendering features like lighting and depth testing.
 *
 * @note Uses fixed-function pipeline and GLUT bitmap fonts.
 *
 */
void Eng::CallbackManager::renderInfoBar() {
    /* Unsupported 4.4
   glPushAttrib(GL_ALL_ATTRIB_BITS);
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glLoadIdentity();
   glOrtho(0, glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT), 0, -1, 1);

   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glLoadIdentity();

   // Disable 3D rendering features
   glDisable(GL_LIGHTING);
   glDisable(GL_DEPTH_TEST);
   glDisable(GL_CULL_FACE);
   // Draw semi-transparent background
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   const int padding = 0;
   const int textHeight = 20;
   const int windowWidth = glutGet(GLUT_WINDOW_WIDTH);
   const int yPos = glutGet(GLUT_WINDOW_HEIGHT) - padding - textHeight;

   // Calculate text width
   std::string text = "Press I to toggle info menu";
   int textWidth = 0;
   for (char c : text) {
      textWidth += glutBitmapWidth(GLUT_BITMAP_9_BY_15, c);
   }

   // Calculate centered x position
   const int xPos = (windowWidth - textWidth) / 2;

   // Draw black background with alpha
   glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
   glBegin(GL_QUADS);
   glVertex2i(0, yPos); // Estendi il background a tutta la larghezza
   glVertex2i(windowWidth, yPos);
   glVertex2i(windowWidth, yPos + textHeight);
   glVertex2i(0, yPos + textHeight);
   glEnd();

   // Draw text
   glColor3f(1.0f, 1.0f, 1.0f);
   glRasterPos2i(xPos, yPos + 15);
   for (char c : text) {
      glutBitmapCharacter(GLUT_BITMAP_9_BY_15, c);
   }

   // Restore OpenGL state
   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
   glMatrixMode(GL_MODELVIEW);
   glDisable(GL_CULL_FACE);
   glPopMatrix();
   glPopAttrib();
   */
}