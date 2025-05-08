/**
 * @file		Engine.h
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
    #include <glm/gtc/matrix_inverse.hpp>
#else  // sia Mac che Linux
#include "../dependencies/glm/include/glm/glm.hpp"
#include "../dependencies/glm/include/glm/gtc/type_ptr.hpp"
#include "../dependencies/glm/include/glm/gtc/matrix_transform.hpp"
#include "../dependencies/glm/include/glm/gtx/string_cast.hpp"
#include "../dependencies/glm/include/glm/gtc/matrix_inverse.hpp"
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
#define ENG_STEREO_RENDERING  0x0002

// Window and FBO size constants
#define APP_WINDOWSIZEX   1024
#define APP_WINDOWSIZEY   512
#define APP_FBOSIZEX      (APP_WINDOWSIZEX / 2)
#define APP_FBOSIZEY      APP_WINDOWSIZEY

// Stereo camera constants
#define STEREO_NEAR_CLIP    0.1f
#define STEREO_FAR_CLIP     1000000.0f

#define STEREO_EYE_HEIGHT   -0.1f

/**
 * @namespace Eng
 * @brief Main namespace containing all engine components
 */

namespace Eng {
    /////////////////
    // SceneGraph //
    ///////////////
#include "FrameBufferObject.h"
#include "BoundingBox.h"
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
#include "Shader.h"
#include "VertexShader.h"
#include "FragmentShader.h"
#include "Program.h"
#include "RenderLayer.h"
#include "ListElement.h"
#include "ListIterator.h"
#include "List.h"
#include "OvoReader.h"
#include "CallbackManager.h"
#include "PostProcessor.h"
#include "PostProcessorManager.h"
#include "BloomEffect.h"
#include "Builder.h"
#include "ShaderManager.h"
#include "Skybox.h"
#include "HolographicMaterial.h"
#include "RenderPipeline.h"


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

      void run();
      bool init();
      bool free();

      void renderScene();
      void loadScene(const std::string &fileName);
      std::shared_ptr<Node> getRootNode();

      void SetActiveCamera(std::shared_ptr<Camera> camera);
      std::shared_ptr<Camera> getActiveCamera() const;
      float getWindowAspectRatio() ;


      bool setupStereoscopicRendering(int width, int height);
      void renderStereoscopic();


      void setEyeDistance(float distance) { eyeDistance = distance; }
      float getEyeDistance() const { return eyeDistance; }

      // Skybox
      void registerSkybox(const std::vector<std::string>& faces);
      std::shared_ptr<Skybox> getSkybox() const;

	  // HeadNode for LeapMotion
      std::shared_ptr<Node> getHeadNode();

      void setStereoEyeHeight(float height) { stereoEyeHeight = height; }
      float getStereoEyeHeight() const { return stereoEyeHeight; }

      void setBodyPosition(const glm::mat4& position);
      glm::mat4 getBodyPosition() const;

      //post processing
      bool addPostProcessor(std::shared_ptr<PostProcessor> postProcessor);
      bool removePostProcessor(const std::string& name);
      std::shared_ptr<PostProcessor> getPostProcessor(const std::string& name);
      void setPostProcessingEnabled(bool enabled);
      bool isPostProcessingEnabled() const;
      bool initializePostProcessingTextures(int width, int height);
      void cleanupTextures();

   private:
      /** @brief Reserved implementation details */
      struct Reserved;
      std::unique_ptr<Reserved> reserved;

	  std::shared_ptr<Eng::BoundingBox> sceneBoundingBox = nullptr;
      float stereoNearClip = 0.2f;
	  float stereoFarClip = 1000000.0f;

      std::shared_ptr<Fbo> leftEyeFbo;
      std::shared_ptr<Fbo> rightEyeFbo;
      unsigned int leftEyeTexture;
      unsigned int rightEyeTexture;
      glm::mat4 stereoInitialTransform = glm::translate(glm::mat4(1.0f), glm::vec3(-1.4f, -0.1f, -0.6f)) *
          glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(0.0f, 1.0f, 0.0f));

      float eyeDistance;
      glm::mat4 computeEyeViewMatrix(const glm::mat4& cameraWorldMatrix, float eyeOffset);
      void renderEye(Fbo* eyeFbo, glm::mat4& viewMatrix, glm::mat4& projectionMatrix);

      Base();
      ~Base();

      bool initOpenGL();
      bool initOpenVR();
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

      // Skybox
      std::shared_ptr<Skybox> skybox;

	  // Render pipeline
	  RenderPipeline renderPipeline;

      // HeadNode for LeapMotion
	  std::shared_ptr<Node> headNode;
      int stereoRenderWidth = 0;
      int stereoRenderHeight = 0;

      std::shared_ptr<Eng::Fbo> sceneFbo;
      std::shared_ptr<Eng::Fbo> outputFbo;
      unsigned int sceneTexture = 0;
      unsigned int outputTexture = 0;

      // Post-processing textures for stereoscopic rendering
      unsigned int leftEyePostTexture = 0;
      unsigned int rightEyePostTexture = 0;

      float stereoEyeHeight = STEREO_EYE_HEIGHT;
   };
}; // end of namespace Eng::
