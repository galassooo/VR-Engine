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
   for (int i = 0; i < size; ++i) {
      // Generate modelView matrix
      glm::mat4 modelViewMatrix = viewMatrix * elements[i]->getWorldCoordinates();

      glLoadMatrixf(glm::value_ptr(modelViewMatrix));

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
