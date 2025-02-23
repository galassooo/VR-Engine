#pragma once

/**
 * @struct KeyInfo
 * @brief Stores information about a keyboard shortcut and its associated functionality
 */
struct KeyInfo {
   char key; ///< The keyboard key that triggers the callback
   std::string desc; ///< Human-readable description of the key's function
   std::function<void(unsigned char, int, int)> func; ///< Callback function executed when key is pressed
};

/**
 * @class CallbackManager
 * @brief Manages FreeGLUT callbacks and keyboard bindings for the graphics engine
 *
 * The CallbackManager implements the Singleton pattern to provide centralized management
 * of all input and rendering callbacks. It handles:
 * - Registration and execution of display, reshape, keyboard, and special key callbacks
 * - Management of custom render callbacks that can be dynamically added/removed
 * - Maintenance of a help menu system showing available keyboard shortcuts
 * - Coordination of core engine functionality like FPS display and coordinate axes
 */
class ENG_API CallbackManager {
public:
   /** @brief Function type for display/render callbacks */
   using DisplayFunc = std::function<void()>;
   /** @brief Function type for window reshape callbacks */
   using ReshapeFunc = std::function<void(int, int)>;
   /** @brief Function type for keyboard input callbacks */
   using KeyboardFunc = std::function<void(unsigned char, int, int)>;
   /** @brief Function type for special key callbacks */
   using SpecialFunc = std::function<void(int, int, int)>;
   /** @brief Function type for application cleanup callbacks */
   using CloseFunc = std::function<void()>;
   /** @brief Function type for optional render callbacks */
   using RenderCallBack = std::function<void()>;

   static CallbackManager &getInstance();
   bool initialize();
   CallbackManager(const CallbackManager &) = delete;
   CallbackManager &operator=(const CallbackManager &) = delete;

   void setDisplayCallback(DisplayFunc func);
   void setReshapeCallback(ReshapeFunc func);
   void registerKeyBinding(char key, const std::string &description, KeyboardFunc func);
   void setSpecialCallback(SpecialFunc func);
   void setCloseCallback(CloseFunc func);

   void registerRenderCallback(std::string id, const RenderCallBack &func);
   void deregisterCallback(const std::string &id);
   void executeRenderCallbacks() const;

   bool isHelpMenuVisible() const { return showHelpMenu; }
   const std::unordered_map<char, KeyInfo> &getKeyBindings() const { return keyBindings; }

private:
   /** @brief Private constructor to enforce singleton pattern */
   CallbackManager() = default;

   static void renderCoordinateAxes();
   static void calculateFPS();
   static void renderInfoBar();
   void setDefaultCallbacks();
   void renderHelpMenu();

   ///< Primary display callback
   DisplayFunc displayCallback;
   ///< Window reshape callback
   ReshapeFunc reshapeCallback;
   ///< Standard keyboard callbacks
   std::vector<KeyboardFunc> keyboardCallbacks;
   ///< Special key callback
   SpecialFunc specialCallback;
   ///< Cleanup callback
   CloseFunc closeCallback;
   ///< Help menu visibility flag
   bool showHelpMenu = false;
   ///< Registered keyboard shortcuts
   std::unordered_map<char, KeyInfo> keyBindings;
   ///< Optional render callbacks
   std::unordered_map<std::string, RenderCallBack> optionalRenderCallbacks;
   ///< Initialization state flag
   bool initialized = false;
};
