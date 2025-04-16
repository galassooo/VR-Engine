#include "engine.h"
#include <GL/freeglut.h>

/**
 * @brief Default constructor for the List class.
 *
 * Initializes the render list and assigns a default name.
 */
Eng::List::List() : Object() {
   name = "RenderList";
}

/**
 * @brief Adds a node to the render list.
 *
 * Nodes are categorized based on their type:
 * - Light nodes are added to the front of the list.
 * - Other nodes are added to the back of the list.
 *
 * @param node A shared pointer to the node being added.
 * @param finalMatrix The transformation matrix in world space for the node.
 */
void Eng::List::addNode(const std::shared_ptr<Eng::Node> &node, const glm::mat4 &finalMatrix) {
   // Create the ListElement and then sort it
   auto element = std::make_shared<Eng::ListElement>(node, finalMatrix);

   const auto it = std::ranges::find_if(elements, [&element](const auto &e) {
      return element->getLayer() < e->getLayer();
   });

   elements.insert(it, element);
}

/**
 * @brief Clears the render list.
 *
 * Removes all nodes from the list, resetting it for the next frame.
 */
void Eng::List::clear() {
   elements.clear();
}

/**
 * @brief Renders all nodes in the render list.
 *
 * This method iterates through all nodes, computes their model-view matrices,
 * and invokes their render methods.
 *
 */
void Eng::List:: render() {
   const int size = elements.size();

   // Virtual Environment
   // Here we do the sphere culling for the virtual environment as it is simpler to work in eye coordinates rather than world coordinates
   // The previous implementation was on the engine traverseAndAdd but it was wrong as we were mixing eye coordinates with world coordinates

   // Determine if we are in stereoscopic mode
   bool stereo = Eng::Base::engIsEnabled(ENG_STEREO_RENDERING);


   // Set up the near and far clip based on the engine's mode
   float nearClip, farClip;
   if (stereo) {
       nearClip = STEREO_NEAR_CLIP;
       farClip = STEREO_FAR_CLIP;
   }
   else
   {
       // Retrieve the active camera values
       auto cam = Eng::Base::getInstance().getActiveCamera();
       if (cam) {
           if (const Eng::PerspectiveCamera* persp = dynamic_cast<const Eng::PerspectiveCamera*>(cam.get())) {
               nearClip = persp->getNearClip();
               farClip = persp->getFarClip();
           }
           else {
               nearClip = 0.01f;
               farClip = 1000.0f;
           }
       }
       else {
           nearClip = 0.01f;
           farClip = 1000.0f;
       }
   }

   // Compute the mid-distance and culling sphere radius
   float midDistance = (nearClip + farClip) * 0.5f;
   float cullingRadius = (farClip - nearClip) * 0.5f;

   // In eye space, the camera (or head) is at (0,0,0)
   glm::vec3 cullingCenter(0.0f, 0.0f, -midDistance);

   for (int i = 0; i < size; ++i) {
      // Generate modelView matrix
      glm::mat4 modelViewMatrix = viewMatrix * elements[i]->getWorldCoordinates();

      // glLoadMatrixf(glm::value_ptr(modelViewMatrix));    unsupported 4.4

      // If this element's node is a Mesh, perfrom sphere culling
      Eng::Mesh* mesh = dynamic_cast<Eng::Mesh*>(elements[i]->getNode().get());
      if (mesh) {
          // The mesh stores its bounding sphere (in local space).
          glm::vec3 localCenter = mesh->getBoundingSphereCenter();
          float localRadius = mesh->getBoundingSphereRadius();

          // Transform the bounding sphere center to eye space.
          glm::vec3 eyeCenter = glm::vec3(modelViewMatrix * glm::vec4(localCenter, 1.0f));

          // For non-uniform scaling, extract an approximate uniform scale:
          float scale = glm::length(glm::vec3(modelViewMatrix[0]));
          float effectiveRadius = localRadius * scale;

          // Compute the squared distance from the transformed center to the culling sphere center.
          glm::vec3 diff = eyeCenter - cullingCenter;
          float distSq = glm::dot(diff, diff);
          float sumRadii = effectiveRadius + cullingRadius;

          // If the object's sphere is completely outside, skip rendering it.
          if (distSq > (sumRadii * sumRadii))
              continue;
      }

      // Send 4x4 modelview matrix
      ShaderManager::getInstance().setModelViewMatrix(modelViewMatrix);

      // Send 3x3 inverse-transpose for normals
      glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(modelViewMatrix));
      ShaderManager::getInstance().setNormalMatrix(normalMat);

      elements[i]->getNode()->render();
   }
}

/**
 * @brief Retrieves all elements in the list.
 * @return A vector of shared pointers to the list elements in the list.
 */
std::vector<std::shared_ptr<Eng::ListElement> > Eng::List::getElements() const {
   return elements;
}

/**
* @brief Set new view matrix in the render list.
* @param glm::mat4 View matrix.
*/
void Eng::List::setViewMatrix(glm::mat4& viewMatrix) {
    this->viewMatrix = viewMatrix;
}
