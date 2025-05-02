#include "engine.h"

// GLEW
#include <GL/glew.h>

#include <GL/freeglut.h>

/**
 * @brief Default constructor for the List class.
 *
 * Initializes the render list and assigns a default name.
 */
Eng::List::List() : Object(), cullingSphere(std::make_unique<Eng::List::CullingSphere>()) {
   name = "RenderList";
}
Eng::List::~List() = default;

struct Eng::List::CullingSphere {
    glm::vec3 center;
    float radius;

    /**
    * Constructor.
    */
    CullingSphere() : center{ glm::vec3(0.0f) }, radius{ 1.0f } {
    }
    ~CullingSphere() { }
};

std::shared_ptr<Eng::BoundingBox> Eng::List::getSceneBoundingBox() {
    // The scene bounding box is computed only once at first call
	if (!sceneBoundingBox) {
		sceneBoundingBox = std::make_shared<Eng::BoundingBox>();
        std::cout << "[List] Computing Scene Bounding Box" << std::endl;
        for (int i = lightsCount; i < elements.size(); ++i) {

            if (const auto& mesh = dynamic_cast<Eng::Mesh*>(elements[i]->getNode().get())) {
                for (auto& vertex : mesh->getVertices()) {
                    sceneBoundingBox->update(glm::vec3(mesh->getFinalMatrix() * glm::vec4(vertex.getPosition(), 1.0f)));
                }
            }
        }
        for (auto& vertex : sceneBoundingBox->getVertices()) {
            std::cout << "  (" << vertex.x << ", " << vertex.y << ", " << vertex.z << ")" << std::endl;
        }
	}
	return sceneBoundingBox;
}

/**
 * @brief Computes the frustum corners based on the current view and projection matrices.
 *
 * The frustum corners are computed in world coordinates.
 * If they are already computed, they are returned from the cache.
 *
 * @param projectionMatrix The projection matrix.
 * @param viewMatrix The view matrix.
 * @return A vector of 8 corners representing the frustum.
 */
std::vector<glm::vec3> Eng::List::getEyeFrustumCorners() {
	// The frustum corners are cached once computed
	if (!currentFrustumCorners) {
		currentFrustumCorners = std::make_unique<std::vector<glm::vec3>>(computeFrustumCorners(eyeProjectionMatrix, eyeViewMatrix));
	}
	return *currentFrustumCorners;
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
 * It also clears the caches stored during rendering.
 *
 */
void Eng::List::clear() {
   elements.clear();
   lightsCount = 0;
   currentFrustumCorners = nullptr;
}

bool Eng::List::isWithinCullingSphere(Eng::Mesh* mesh) {
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
    glm::vec3 diff = eyeCenter - cullingSphere->center;
    float distSq = glm::dot(diff, diff);
    float sumRadii = effectiveRadius + cullingSphere->radius;

    // If the object's sphere is completely outside, skip rendering it.
    if (distSq > (sumRadii * sumRadii))
        return false;
    else
        return true;
}

/**
 * @brief Renders all nodes in the render list.
 *
 * This method iterates through all nodes, computes their model-view matrices,
 * and invokes their render methods.
 *
 */
void Eng::List::render() {
    // Virtual Environment
   // Here we do the sphere culling for the virtual environment as it is simpler to work in eye coordinates rather than world coordinates
   // The previous implementation was on the engine traverseAndAdd but it was wrong as we were mixing eye coordinates with world coordinates

    // Virtual Environment culling setup
	Eng::BoundingBox viewBoundingBox = Eng::BoundingBox();
	// Compute the view bounding box based on the frustum corners
	//std::cout << "Frustum corners:" << std::endl;
	for (const auto& corner : getEyeFrustumCorners()) {
		// Print the corner coordinates for debugging
		//std::cout << "Corner: (" << corner.x << ", " << corner.y << ", " << corner.z << ")" << std::endl;
		viewBoundingBox.update(corner);
	}
	cullingSphere->center = viewBoundingBox.getCenter();
	cullingSphere->radius = glm::length(viewBoundingBox.getSize()) * 0.5f;
	//std::cout << "Culling sphere center: (" << cullingSphere->center.x << ", " << cullingSphere->center.y << ", " << cullingSphere->center.z << ")" << std::endl;
	//std::cout << "Culling sphere radius: " << cullingSphere->radius << std::endl;

}



/**
 * @brief Computes the light projection matrix based on the light view matrix and the scene bounding box corners.
 *
 * This method transforms the bounding box corners to light space and computes the
 * orthographic projection matrix based on the light space bounding box.
 *
 * @param lightViewMatrix The view matrix of the light source.
 * @param boundingBoxVertices The corners of the scene bounding box in world coordinates.
 * @return The computed orthographic projection matrix for the light source.
 */
glm::mat4 Eng::List::computeLightProjectionMatrix(const glm::mat4& lightViewMatrix, const std::vector<glm::vec3>& boundingBoxVertices) {
    std::vector<glm::vec3> lightSpaceVertices;
    for (const auto& vertex : boundingBoxVertices) {
        glm::vec4 transformed = lightViewMatrix * glm::vec4(vertex, 1.0f);
        lightSpaceVertices.push_back(glm::vec3(transformed));
    }

    glm::vec3 minLS(FLT_MAX);
    glm::vec3 maxLS(-FLT_MAX);

    for (const auto& v : lightSpaceVertices) {
        minLS = glm::min(minLS, v);
        maxLS = glm::max(maxLS, v);
    }

	// Build the ortho projection matrix based on the light space bounding box
    glm::mat4 lightProjection = glm::ortho(
        minLS.x, maxLS.x,
        minLS.y, maxLS.y,
        -maxLS.z, -minLS.z // OpenGL NDC depth goes in the opposite direction -1 to 1
    );

    return lightProjection;
}

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
* @brief Set new view matrix in the render list.
* @param glm::mat4 View matrix.
*/
void Eng::List::setEyeViewMatrix(glm::mat4& viewMatrix) {
    this->eyeViewMatrix = viewMatrix;
}

/**
* @brief Set new eye projection matrix in the render list.
* @param glm::mat4 Eye projection matrix.
*/
void Eng::List::setEyeProjectionMatrix(glm::mat4& eyeProjectionMatrix) {
	this->eyeProjectionMatrix = eyeProjectionMatrix;
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
