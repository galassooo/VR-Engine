/**
 * @file		engine.h
 * @brief	Graphics engine main include file
 *
 * @author	K. Quarenghi, M. Galasso
 */
#pragma once


//////////////
// #INCLUDE //
//////////////
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <stack>
#include <unordered_map>
#include <cassert>
#include <functional>
#include <cstdio>
#define GLM_ENABLE_EXPERIMENTAL


#ifdef _WINDOWS
    #include "glm/glm.hpp"
    #include <glm/gtc/type_ptr.hpp>
    #include <glm/gtc/matrix_transform.hpp>
    #include <glm/gtx/string_cast.hpp>
#else  // sia Mac che Linux
#include "../dependencies/glm/include/glm/glm.hpp"
#include "../dependencies/glm/include/glm/gtc/type_ptr.hpp"
#include "../dependencies/glm/include/glm/gtc/matrix_transform.hpp"
#include "../dependencies/glm/include/glm/gtx/string_cast.hpp"
#endif


// Generic info:
#ifdef _DEBUG
   #define LIB_NAME      "Kevin Quarenghi & Martina Galasso's Graphics Engine v1.0"   ///< Library credits
#else
#define LIB_NAME      "Kevin Quarenghi & Martina Galasso's Graphics Engine v1.0"   ///< Library credits
#endif
#define LIB_VERSION   10                           ///< Library version (divide by 10)

// Export API:
#ifdef _WINDOWS
#ifdef ENGINE_EXPORTS
      #define ENG_API __declspec(dllexport)
#else
      #define ENG_API __declspec(dllimport)
#endif
   #pragma warning(disable : 4251)
#else
#define ENG_API
#endif


// Engine capability flags
#define ENG_RENDER_NORMAL   0x0001

/**
 * @namespace Eng
 * @brief Main namespace containing all engine components
 */

namespace Eng {
    /////////////////
    // SceneGraph //
    ///////////////
#include "Object.h"
#include "Node.h"
#include "Camera.h"
#include "OrthographicCamera.h"
#include "PerspectiveCamera.h"
#include "Light.h"
#include "PointLight.h"
#include "SpotLight.h"
#include "DirectionalLight.h"
#include "Texture.h"
#include "Material.h"
#include "Vertex.h"
#include "Mesh.h"
#include "RenderLayer.h"
#include "ListElement.h"
#include "List.h"
#include "OvoReader.h"
#include "CallbackManager.h"

    ///////////////
    //// Tests ///
    /////////////

#include "Tests/Test_Camera.h"
#include "Tests/Test_Light.h"
#include "Tests/Test_Node.h"
#include "Tests/Test_List.h"
#include "Tests/Test_Mesh.h"
#include "Tests/Test_CallManager.h" 

   /**
    * @class Base
    * @brief Core engine class implementing the singleton pattern
    *
    * The Base class is the main entry point of the graphics engine and manages:
    * - Engine initialization and cleanup
    * - Scene graph management
    * - Camera system
    * - Rendering pipeline
    * - Window management
    *
    * As a singleton, only one instance can exist at any time, accessed via getInstance().
    */
   ///> Eng state
   static unsigned int engineState = 0;
   class ENG_API Base final {
   public:
      static Base &getInstance();
      Base(Base const &) = delete;
      void operator=(Base const &) = delete;

      static void engEnable(unsigned int cap);
      static void engDisable(unsigned int cap);
      static bool engIsEnabled(unsigned int cap);

      static void run();
      bool init();
      bool free();

      void renderScene();
      void loadScene(const std::string &fileName);
      std::shared_ptr<Node> getRootNode();

      void SetActiveCamera(std::shared_ptr<Camera> camera);
      std::shared_ptr<Camera> getActiveCamera() const;
      float getWindowAspectRatio() ;

   private:
      /** @brief Reserved implementation details */
      struct Reserved;
      std::unique_ptr<Reserved> reserved;

      Base();
      ~Base();

      bool initOpenGL();
      void freeOpenGL();

      void traverseAndAddToRenderList(const std::shared_ptr<Node> &node);

      ///> Root node of the scene graph
      std::shared_ptr<Node> rootNode;
      ///> Currently active camera for rendering
      std::shared_ptr<Camera> activeCamera;
      ///> List of objects to be rendered
      List renderList;
      ///>  FreeGLUT window identifier
      int windowId;
   };
}; // end of namespace Eng::
