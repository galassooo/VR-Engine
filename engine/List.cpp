#include "engine.h"

// GLEW
#include <GL/glew.h>

#include <GL/freeglut.h>

/**
 * @brief Constructs a new render List with default settings.
 *
 * Initializes the base Object, creates a culling sphere,
 * and sets the default list name.
 */
Eng::List::List() : Object() {
   name = "RenderList";
}

/**
 * @brief Default destructor for List.
 */
Eng::List::~List() = default;


/**
 * @brief Constructs a culling sphere helper struct.
 *
 * Initializes center at origin and a unit radius.
 */
struct Eng::List::CullingSphere {
    glm::vec3 center;
    float radius;

    /**
    * Constructor.
    */
    CullingSphere() : center{ glm::vec3(0.0f) }, radius{ 1.0f } {
    }

    /**
     * @brief Destructor for CullingSphere.
     */
    ~CullingSphere() { }
};


/**
 * @brief Computes and returns the scene's axis-aligned bounding box.
 *
 * On first invocation, iterates over all mesh elements (excluding lights),
 * updates a BoundingBox, and logs corner coordinates.
 *
 * @return Shared pointer to the scene's BoundingBox.
 */
std::shared_ptr<Eng::BoundingBox> Eng::List::getSceneBoundingBox() {
    // The scene bounding box is computed only once at first call
	if (!sceneBoundingBox) {
		sceneBoundingBox = std::make_shared<Eng::BoundingBox>();
        std::cout << "[List] Computing Scene Bounding Box" << std::endl;
        for (int i = lightsCount; i < elements.size(); ++i) {

            if (const auto& mesh = dynamic_cast<Eng::Mesh*>(elements[i]->getNode().get())) {
                sceneBoundingBox->update(glm::vec3(mesh->getFinalMatrix() * glm::vec4(mesh->getBoundingBoxMin(), 1.0f)));
                sceneBoundingBox->update(glm::vec3(mesh->getFinalMatrix() * glm::vec4(mesh->getBoundingBoxMax(), 1.0f)));
            }
        }
        for (auto& vertex : sceneBoundingBox->getVertices()) {
            std::cout << "  (" << vertex.x << ", " << vertex.y << ", " << vertex.z << ")" << std::endl;
        }
	}
	return sceneBoundingBox;
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

   // Keep lights count to use it as an index later
   if (element->getLayer() == RenderLayer::Lights)
       lightsCount++;

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
   lightsCount = 0;
   frustumCornersCached = nullptr;
   cullingSphereCached = nullptr;
}

/**
 * @brief Checks if a mesh is within the culling sphere.
 *
 * This method computes the distance between the mesh's bounding sphere and the
 * culling sphere. If the mesh is outside the culling sphere, it will not be rendered.
 *
 * @param mesh A shared pointer to the mesh being checked.
 * @return true if the mesh is within the culling sphere, false otherwise.
 */
bool Eng::List::isWithinCullingSphere(const std::shared_ptr<Eng::Mesh>& mesh) {
    if (!cullingSphereCached) {
        computeCullingSphere();
    }
    // The mesh stores its bounding sphere (in local space).
    glm::vec3 localCenter = mesh->getBoundingSphereCenter();
    float localRadius = mesh->getBoundingSphereRadius();

    // Transform the bounding sphere center to eye space.
    glm::mat4 modelViewMatrix = eyeViewMatrix * mesh->getFinalMatrix();
    glm::vec3 eyeCenter = glm::vec3(modelViewMatrix * glm::vec4(localCenter, 1.0f));

    // For non-uniform scaling, extract an approximate uniform scale:
    float scale = glm::length(glm::vec3(modelViewMatrix[0]));
    float effectiveRadius = localRadius * scale;

    // Compute the squared distance from the transformed center to the culling sphere center.
    glm::vec3 diff = eyeCenter - cullingSphereCached->center;
    float distSq = glm::dot(diff, diff);
    float sumRadii = effectiveRadius + cullingSphereCached->radius;

    // If the object's sphere is completely outside, skip rendering it.
    if (distSq > (sumRadii * sumRadii))
        return false;
    else
        return true;
}

/**
 * @brief Recomputes the culling sphere from the view frustum corners.
 *
 * Builds a BoundingBox in eye-space from frustum corners, then sets
 * the sphere center and radius to tightly enclose that box.
 */
void Eng::List::computeCullingSphere() {
    // Virtual Environment
   // Here we do the sphere culling for the virtual environment as it is simpler to work in eye coordinates rather than world coordinates
   // The previous implementation was on the engine traverseAndAdd but it was wrong as we were mixing eye coordinates with world coordinates
    // Culling setup
    cullingSphereCached = std::make_unique<Eng::List::CullingSphere>();
    Eng::BoundingBox viewBoundingBox = Eng::BoundingBox();
    // Compute the view bounding box based on the frustum corners
    for (const auto& corner : getEyeFrustumCorners()) {
        viewBoundingBox.update(corner);
    }
    cullingSphereCached->center = viewBoundingBox.getCenter();
    cullingSphereCached->radius = glm::length(viewBoundingBox.getSize()) * 0.5f;
}

/**
 * @brief Performs the full multi-pass rendering of this list.
 *
 * Renders opaque geometry, then for each light renders its pass including
 * additive blending or shadow passes for directional lights.
 */
void Eng::List::render() {
}


/**
 * @brief Computes the frustum corners based on the current view and projection matrices.
 *
 * The frustum corners are computed in world coordinates.
 * If they are already computed, they are returned from the cache.
 *
 * @param projectionMatrix The projection matrix.
 * @param viewMatrix The view matrix.
 * @return A copy of the cached vector of 8 corners representing the frustum.
 */
std::vector<glm::vec3> Eng::List::getEyeFrustumCorners() {
    // The frustum corners are cached once computed
    if (!frustumCornersCached) {
        frustumCornersCached = std::make_unique<std::vector<glm::vec3>>(computeFrustumCorners(eyeProjectionMatrix, eyeViewMatrix));
    }
    return *frustumCornersCached;
}

/**
 * @brief Computes world-space view frustum corners from projection and view matrices.
 *
 * Maps NDC corners back through the inverse view-projection transform.
 *
 * @param projectionMatrix Current projection matrix.
 * @param viewMatrix       Current view matrix.
 * @return Vector of 8 world-space frustum corner positions.
 */
std::vector<glm::vec3> Eng::List::computeFrustumCorners(glm::mat4 projectionMatrix, glm::mat4 viewMatrix) {
    std::vector<glm::vec3> corners = {
        glm::vec3(-1, -1, -1),
        glm::vec3(1, -1, -1),
        glm::vec3(1,  1, -1),
        glm::vec3(-1,  1, -1),
        glm::vec3(-1, -1,  1),
        glm::vec3(1, -1,  1),
        glm::vec3(1,  1,  1),
        glm::vec3(-1,  1,  1)
    };

    glm::mat4 invViewProj = glm::inverse(projectionMatrix * viewMatrix);

    for (auto& corner : corners) {
        glm::vec4 corner4(corner, 1.0f);
        corner4 = invViewProj * corner4;
        corner = glm::vec3(corner4 / corner4.w); // normalize the omogeneous coordinates
    }

    return corners;
}

/**
 * @brief Retrieves all elements in the list.
 * @return A vector of shared pointers to the list elements in the list.
 */
std::vector<std::shared_ptr<Eng::ListElement> > Eng::List::getElements() const {
   return elements;
}

/**
 * @brief Set new eye view matrix in the render list.
 * @param glm::mat4 Eye view matrix.
 */
void Eng::List::setEyeViewMatrix(glm::mat4& viewMatrix) {
    this->eyeViewMatrix = viewMatrix;
	cullingSphereCached = nullptr; // Reset the culling sphere cache
	frustumCornersCached = nullptr; // Reset the frustum corners cache
}

/**
* @brief Set new eye projection matrix in the render list.
* @param glm::mat4 Eye projection matrix.
*/
void Eng::List::setEyeProjectionMatrix(glm::mat4& eyeProjectionMatrix) {
	this->eyeProjectionMatrix = eyeProjectionMatrix;
    cullingSphereCached = nullptr; // Reset the culling sphere cache
	frustumCornersCached = nullptr; // Reset the frustum corners cache
}

/**
 * @brief Sets the global light color for the render list.
 *
 * This method updates the global light color used in the rendering process.
 *
 * @param globalColor The new global light color as a glm::vec3.
 */
void Eng::List::setGlobalLightColor(const glm::vec3& globalColor) {
    this->globalLightColor = globalColor;
}

/**
 * @brief Retrieves the iterator for a specific render layer.
 *
 * This method returns an iterator for the specified render layer.
 * If the iterator is not already cached, it creates a new one and caches it.
 *
 * @param layer The render layer for which to get the iterator.
 * @return A reference to the ListIterator for the specified layer.
 */
Eng::ListIterator Eng::List::getLayerIterator(const Eng::RenderLayer& layer) {
    Eng::ListIterator* iterator = nullptr;
    switch (layer) {
    case Eng::RenderLayer::Lights:
        if (!lightsIteratorCached) {
            lightsIteratorCached = std::make_unique<Eng::ListIterator>(*getElements(layer));
        }
        iterator = lightsIteratorCached.get();
        break;
    case Eng::RenderLayer::Opaque:
        if (!opaqueIteratorCached) {
            opaqueIteratorCached = std::make_unique<Eng::ListIterator>(*getElements(layer));
        }
        iterator = opaqueIteratorCached.get();
        break;
    case Eng::RenderLayer::Transparent:
        if (!transparentIteratorCached) {
            transparentIteratorCached = std::make_unique<Eng::ListIterator>(*getElements(layer));
        }
        iterator = transparentIteratorCached.get();
        break;
    }
    return *iterator;
}

/**
 * @brief Retrieves all elements in the list for a specific render layer.
 *
 * This method returns a shared pointer to a vector of ListElement objects
 * that belong to the specified render layer.
 *
 * @param layer The render layer for which to get the elements.
 * @return A shared pointer to a vector of ListElement objects.
 */
std::shared_ptr<std::vector<std::shared_ptr<Eng::ListElement>>> Eng::List::getElements(const Eng::RenderLayer& layer) {
    std::shared_ptr<std::vector<std::shared_ptr<Eng::ListElement>>> layerElements = std::make_shared<std::vector<std::shared_ptr<Eng::ListElement>>>();
    for (const auto& element : elements) {
        if (element->getLayer() == layer) {
            layerElements->push_back(element);
        }
    }
    return layerElements;
}
