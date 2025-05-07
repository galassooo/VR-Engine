#include "engine.h"
#include <iostream>
#include <functional>
#include <random>
#include <array>
#include <chrono>
#include <deque>
#include "leap.h"

extern "C" {
    _declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    _declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

/*
* Skybox
*/ 
std::vector<std::string> myCubemapFaces = {
    "../resources/right.hdr", // right
    "../resources/left.hdr", // left
    "../resources/top.hdr", // top
    "../resources/bottom.hdr", // bottom
    "../resources/front.hdr", // front
    "../resources/back.hdr"  // back
};

/*
* Leap Motion
*/
Leap* leap = nullptr;
std::shared_ptr<Eng::Node> handsNode = nullptr;
bool leapVisualizationEnabled = true;
static const int MAX_HANDS = 2;
static const int JOINTS_PER_HAND = 3 + (5 * 4);
static std::vector<std::shared_ptr<Eng::Mesh>> jointMeshes;
static std::shared_ptr<Eng::Mesh> cylinderMesh = nullptr;
static std::vector<std::shared_ptr<Eng::Node>> boneNodes;

// forward declarations
void setupLeapMotion(Eng::Base& eng);
void updateLeapHands();
std::shared_ptr<Eng::Mesh> createSphereMesh(float radius = 5.0f);
std::shared_ptr<Eng::Mesh> createCylinderMesh(float radius = 0.002f, float height = 1.0f);

/*
* Chess pieces
*/
struct SelectablePiece {
    std::shared_ptr<Eng::Node> node;
    std::shared_ptr<Eng::Mesh> mesh;
    glm::vec3 boundingBoxMin;
    glm::vec3 boundingBoxMax;
    std::shared_ptr<Eng::Material> originalMaterial;
    glm::mat4 originalMatrix;
    bool isSelected = false;
};

std::vector<SelectablePiece> selectablePieces;
std::shared_ptr<Eng::Node> selectedPiece = nullptr;
bool isPinching = false;
float pinchThreshold = 0.7f;
glm::vec3 grabOffset;
bool showBoundingBoxes = false;
std::vector<std::shared_ptr<Eng::Mesh>> boundingBoxMeshes;

// forward declarations
void updateChessPieceSelection();
void initChessPieceSelection(Eng::Base& eng);
void findChessPieces(std::shared_ptr<Eng::Node> node);
void updateBoundingBoxes(Eng::Base& eng);
std::shared_ptr<Eng::Mesh> createLineMesh(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color);
void createBoundingBoxLines(Eng::Base& eng, const glm::vec3& min, const glm::vec3& max, const glm::vec3& color);
void updateBoundingBoxes(Eng::Base& eng);
void applyHolographicEffect();
bool isPointInBoundingBox(const glm::vec3& point, const SelectablePiece& piece);



/*
* Positions
*/

std::vector<glm::mat4> predefinedPositions;
int currentPositionIndex = 0;
bool gestureActive = false;
float lastGestureTime = 0.0f;
const float GESTURE_COOLDOWN = 1.0f;

// forward declarations
void setupPositionCycling(Eng::Base& eng);
void initPredefinedPositions();
void setUpCameras(Eng::Base &eng);
bool areHandsTogether();
void updatePositionFromGesture();


/**
 * @brief Entry point of the application
 *
 * Initializes the engine, loads the chess scene, sets up various functionalities
 * including Leap Motion tracking, chess piece interaction, and camera controls.
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, -1 on initialization failure
 */
int main(int argc, char *argv[]) {
   std::cout << "Client application example, K. Quarenghi, M. Galasso, L. Forestieri (C) SUPSI" << std::endl;
   std::cout << std::endl;

   Eng::Base &eng = Eng::Base::getInstance();
   if (!eng.init()) {
      return -1;
   }

   // Set post processing
   auto bloom = std::make_shared<Eng::BloomEffect>();
   Eng::Base::getInstance().addPostProcessor(bloom);
   Eng::Base::getInstance().setPostProcessingEnabled(true);

   // Load scene
   eng.loadScene("..\\resources\\Chess.ovo");

   // Setup Motion and piece control
   setupLeapMotion(eng);
   initChessPieceSelection(eng);
   applyHolographicEffect();
   eng.engEnable(ENG_STEREO_RENDERING);

   // Setup cameras
   setUpCameras(eng);
   setupPositionCycling(eng);

   // Register skybox
   eng.registerSkybox(myCubemapFaces);

   eng.run();

    // Cleanup
   if (leap) {
       leap->free();
       delete leap;
       leap = nullptr;
   }

   eng.free();

   return 0;
}

/**
 * @brief Applies holographic material effects to all chess pieces
 *
 * Creates and assigns holographic materials to chess pieces based on their color
 * (blue for white pieces, red for black pieces).
 */
void applyHolographicEffect() {

    auto holoMaterialWhite = std::make_shared<Eng::HolographicMaterial>(
        glm::vec3(0.2f, 0.3f, 0.7f), //base color
        0.0f, //alpha
        200.0f, //frequency
        1.f //speed
    );
    holoMaterialWhite->setSecondaryColor(glm::vec3(0.5f, 0.7f, 1.0f)); //secondary color

    // Materiale olografico per pezzi neri (rosso tenue)
    auto holoMaterialBlack = std::make_shared<Eng::HolographicMaterial>(
        glm::vec3(5.f, 0.3f, 0.3f), //base color
        0.0f, //alpha
        200.0f, //frequency
        1.f  //speed
    );
    holoMaterialBlack->setSecondaryColor(glm::vec3(2.f, 0.1f, 0.1f)); //secondary color

    for (auto& piece : selectablePieces) {
        if (piece.mesh) {
            std::string name = piece.node->getName();
            if (name.length() >= 2) {
                if (name.substr(0, 2) == "W_") {
                    piece.mesh->setMaterial(holoMaterialWhite);
                    piece.originalMaterial = holoMaterialWhite;
                }
                else if (name.substr(0, 2) == "B_") {
                    piece.mesh->setMaterial(holoMaterialBlack);
                    piece.originalMaterial = holoMaterialBlack;
                }
            }
        }
    }
}

/**
 * @brief Sets up the position cycling system for camera viewpoints
 *
 * Initializes predefined positions and registers callbacks for gesture detection
 * to allow switching between different viewpoints.
 *
 * @param eng Reference to the engine instance
 */
void setupPositionCycling(Eng::Base& eng) {
    // Initialize the predefined positions
    initPredefinedPositions();

    // Set the initial position
    eng.setBodyPosition(predefinedPositions[currentPositionIndex]);

    // Register the gesture detection callback
    auto& callbackManager = Eng::CallbackManager::getInstance();
    callbackManager.registerRenderCallback("positionCycling", []() {
        updatePositionFromGesture();
        });
}


/**
 * @brief Updates the viewing position based on hand gestures
 *
 * Checks for the "hands together" gesture and cycles to the next predefined
 * viewpoint position when detected, with a cooldown period.
 */
void updatePositionFromGesture() {
    if (!leap || !Eng::Base::engIsEnabled(ENG_STEREO_RENDERING)) return;

    bool handsTogetherNow = areHandsTogether();
    float currentTime = std::chrono::duration<float>(std::chrono::steady_clock::now().time_since_epoch()).count();


    // Detect the gesture activation (hands coming together)
    if (handsTogetherNow && !gestureActive && (currentTime - lastGestureTime > GESTURE_COOLDOWN)) {
        // Cycle to next position
        currentPositionIndex = (currentPositionIndex + 1) % predefinedPositions.size();

        // Apply the new position
        auto& eng = Eng::Base::getInstance();
        eng.setBodyPosition(predefinedPositions[currentPositionIndex]);

        // Update state and cooldown time
        gestureActive = true;
        lastGestureTime = currentTime;
    }
    else if (!handsTogetherNow && gestureActive) {
        // Reset gesture state when hands are separated
        gestureActive = false;
    }
}

/**
 * @brief Detects if both hands are positioned close together
 *
 * Checks if the distance between the palms of both hands is below
 * a threshold to trigger the position cycling gesture.
 *
 * @return True if hands are detected as being together, false otherwise
 */
bool areHandsTogether() {
    if (!leap) return false;

    leap->update();
    const LEAP_TRACKING_EVENT* frame = leap->getCurFrame();

    // At least two hands for this gesture
    if (frame->nHands < 2) return false;

    const LEAP_HAND& leftHand = frame->pHands[0];
    const LEAP_HAND& rightHand = frame->pHands[1];

    // Get the positions of both hands' palms
    glm::vec3 leftPalmPos(leftHand.palm.position.x, leftHand.palm.position.y, leftHand.palm.position.z);
    glm::vec3 rightPalmPos(rightHand.palm.position.x, rightHand.palm.position.y, rightHand.palm.position.z);

    // Convert from mm to meters
    leftPalmPos *= 0.001f;
    rightPalmPos *= 0.001f;

    // Calculate distance between palms
    float distance = glm::distance(leftPalmPos, rightPalmPos);

    // If palms are within 10cm of each other, consider it a "hands together" gesture
    return distance < 0.1f;
}

/**
 * @brief Initializes the list of predefined viewpoint positions
 *
 * Creates transformation matrices for different viewpoints around the chess board
 * that the user can cycle through using hand gestures.
 */
void initPredefinedPositions() {
    glm::mat4 position1 = glm::translate(glm::mat4(1.0f), glm::vec3(-1.4f, -0.f, -0.6f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 position2 = glm::translate(glm::mat4(1.0f), glm::vec3(-0.2f, 0.0f, -0.6f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 position3 = glm::translate(glm::mat4(1.0f), glm::vec3(-2.3f, 0.5f, -1.2f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(200.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 position4 = glm::translate(glm::mat4(1.0f), glm::vec3(3.5f, 1.0f, 5.f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(10.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    predefinedPositions = { position1, position2, position3, position4 };
}

/**
 * @brief Updates the visual bounding boxes for all chess pieces
 *
 * Creates or updates the line meshes that represent the bounding boxes
 * for all chess pieces when visualization is enabled.
 *
 * @param eng Reference to the engine instance
 */
void updateBoundingBoxes(Eng::Base& eng) {
    // Clear previous bounding box meshes
    for (auto& mesh : boundingBoxMeshes) {
        auto parent = mesh->getParent();
        if (parent) {
            // Remove from parent's children list
            auto& children = *parent->getChildren();
            children.erase(std::remove(children.begin(), children.end(), mesh), children.end());
        }
    }
    boundingBoxMeshes.clear();

    // If visualization is disabled, return
    if (!showBoundingBoxes) {
        return;
    }

    for (const auto& piece : selectablePieces) {
        // Get the piece's transformation matrix
        glm::mat4 M = piece.node->getFinalMatrix();

        // Calculate world space bounding box
        glm::vec3 bbMinL = piece.boundingBoxMin;
        glm::vec3 bbMaxL = piece.boundingBoxMax;
        glm::vec3 bbMinW(+FLT_MAX), bbMaxW(-FLT_MAX);

        for (int i = 0; i < 8; ++i) {
            glm::vec3 cornerL(
                (i & 1) ? bbMaxL.x : bbMinL.x,
                (i & 2) ? bbMaxL.y : bbMinL.y,
                (i & 4) ? bbMaxL.z : bbMinL.z);

            glm::vec3 cornerW = glm::vec3(M * glm::vec4(cornerL, 1.0f));
            bbMinW = glm::min(bbMinW, cornerW);
            bbMaxW = glm::max(bbMaxW, cornerW);
        }

        // Create world-space bounding box (green or blue if selected)
        glm::vec3 color = piece.isSelected ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
        createBoundingBoxLines(eng, bbMinW, bbMaxW, color);
    }
}


/**
 * @brief Creates line meshes representing a bounding box
 *
 * Generates 12 line segments to represent the edges of a bounding box
 * defined by its minimum and maximum coordinates.
 *
 * @param eng Reference to the engine instance
 * @param min The minimum coordinates of the bounding box
 * @param max The maximum coordinates of the bounding box
 * @param color The RGB color of the bounding box lines
 */
void createBoundingBoxLines(Eng::Base& eng, const glm::vec3& min, const glm::vec3& max, const glm::vec3& color) {
    // Create 12 edges of the box as line segments
    std::vector<std::pair<glm::vec3, glm::vec3>> edges = {
        // Bottom face
        {glm::vec3(min.x, min.y, min.z), glm::vec3(max.x, min.y, min.z)},
        {glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, min.y, max.z)},
        {glm::vec3(max.x, min.y, max.z), glm::vec3(min.x, min.y, max.z)},
        {glm::vec3(min.x, min.y, max.z), glm::vec3(min.x, min.y, min.z)},

        // Top face
        {glm::vec3(min.x, max.y, min.z), glm::vec3(max.x, max.y, min.z)},
        {glm::vec3(max.x, max.y, min.z), glm::vec3(max.x, max.y, max.z)},
        {glm::vec3(max.x, max.y, max.z), glm::vec3(min.x, max.y, max.z)},
        {glm::vec3(min.x, max.y, max.z), glm::vec3(min.x, max.y, min.z)},

        // Connecting edges
        {glm::vec3(min.x, min.y, min.z), glm::vec3(min.x, max.y, min.z)},
        {glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, max.y, min.z)},
        {glm::vec3(max.x, min.y, max.z), glm::vec3(max.x, max.y, max.z)},
        {glm::vec3(min.x, min.y, max.z), glm::vec3(min.x, max.y, max.z)}
    };

    // Create and add each line segment
    for (const auto& edge : edges) {
        auto lineMesh = createLineMesh(edge.first, edge.second, color);
        if (lineMesh) {
            eng.getRootNode()->addChild(lineMesh);
            lineMesh->setParent(eng.getRootNode().get());
            boundingBoxMeshes.push_back(lineMesh);
        }
    }
}
/**
 * @brief Creates a line mesh between two points
 *
 * Generates a thin cylinder mesh to represent a line between the specified
 * points with the given color.
 *
 * @param start The starting position of the line
 * @param end The ending position of the line
 * @param color The RGB color of the line
 * @return A shared pointer to the created mesh
 */
std::shared_ptr<Eng::Mesh> createLineMesh(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color) {
    // Create a thin cylinder to represent a line
    const float lineRadius = 0.003f; // Thin radius
    const int segments = 8;          // Low polygon count for efficiency

    // Calculate direction and length
    glm::vec3 direction = end - start;
    float length = glm::length(direction);

    if (length < 0.0001f) {
        // Avoid zero-length lines
        return nullptr;
    }

    // Create normalized direction
    direction = direction / length;

    // Create a rotation matrix to orient the cylinder along the line direction
    // First, find the rotation axis and angle from the z-axis to the line direction
    glm::vec3 zAxis(0.0f, 0.0f, 1.0f);
    glm::vec3 rotationAxis = glm::cross(zAxis, direction);

    float rotationAngle = acos(glm::dot(zAxis, direction));
    // Create vertices for a cylinder
    std::vector<Eng::Vertex> vertices;
    std::vector<unsigned int> indices;

    // If the direction is nearly parallel to the z-axis, use a different rotation
    if (glm::length(rotationAxis) < 0.001f) {
        rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
        rotationAngle = (direction.z > 0.0f) ? 0.0f : glm::pi<float>();
    }
    else {
        rotationAxis = glm::normalize(rotationAxis);
    }

    // Create the cylinder matrix
    glm::mat4 cylinderMatrix = glm::translate(glm::mat4(1.0f), start) *
        glm::rotate(glm::mat4(1.0f), rotationAngle, rotationAxis) *
        glm::scale(glm::mat4(1.0f), glm::vec3(lineRadius, lineRadius, length));

    // Generate vertices for the cylinder
    for (int i = 0; i <= segments; i++) {
        float angle = (float)i / segments * 2.0f * glm::pi<float>();
        float x = cos(angle);
        float y = sin(angle);

        // Bottom vertex
        Eng::Vertex v1;
        v1.setPosition(glm::vec3(x, y, 0.0f));
        v1.setNormal(glm::vec3(x, y, 0.0f));
        v1.setTexCoords(glm::vec2(0.0f));
        vertices.push_back(v1);

        // Top vertex
        Eng::Vertex v2;
        v2.setPosition(glm::vec3(x, y, 1.0f));
        v2.setNormal(glm::vec3(x, y, 0.0f));
        v2.setTexCoords(glm::vec2(1.0f));
        vertices.push_back(v2);
    }

    // Generate indices for the cylinder
    for (int i = 0; i < segments; i++) {
        int idx = i * 2;
        indices.push_back(idx);
        indices.push_back(idx + 1);
        indices.push_back(idx + 2);

        indices.push_back(idx + 1);
        indices.push_back(idx + 3);
        indices.push_back(idx + 2);
    }

    // Create a material with the specified color
    auto material = std::make_shared<Eng::Material>(color, 1.0f, 0.0f, glm::vec3(0));

    // Create the mesh using the Builder pattern
    auto& builder = Eng::Builder::getInstance();
    auto mesh = builder.setName("LineMesh")
        .addVertices(vertices)
        .addIndices(indices)
        .setMaterial(material)
        .setLocalMatrix(cylinderMatrix)
        .build();

    return mesh;
}

/**
 * @brief Recursively finds all chess pieces in the scene graph
 *
 * Searches through all nodes in the scene and identifies chess pieces
 * based on their naming convention ("B_" for black, "W_" for white).
 *
 * @param node The current node to check and traverse
 */
void findChessPieces(std::shared_ptr<Eng::Node> node) {
    if (!node) return;
    
    std::string name = node->getName();
    // finds all meshes with name starting with B_ (black pieces) or W_
    if ((name.length() >= 2 && (name.substr(0, 2) == "B_" || name.substr(0, 2) == "W_"))) {
        auto mesh = std::dynamic_pointer_cast<Eng::Mesh>(node);
        if (mesh) {
            SelectablePiece piece;
            piece.node = node;
            piece.mesh = mesh;
            piece.boundingBoxMin = mesh->getBoundingBoxMin();
            piece.boundingBoxMax = mesh->getBoundingBoxMax();
            piece.originalMaterial = mesh->getMaterial();
            piece.originalMatrix = node->getLocalMatrix();
            selectablePieces.push_back(piece);
        }
    }
    
    // recursively check for children
    for (auto& child : *node->getChildren()) {
        findChessPieces(child);
    }
}
/**
 * @brief Checks if a point is inside a chess piece's bounding box
 *
 * Transforms the local bounding box to world space and tests if the
 * specified point falls within the box, including a small tolerance margin.
 *
 * @param point The world-space point to test
 * @param piece The chess piece with its bounding box information
 * @return True if the point is inside the bounding box, false otherwise
 */
bool isPointInBoundingBox(const glm::vec3& point, const SelectablePiece& piece) {
    // get current final matrix
    glm::mat4 M = piece.node->getFinalMatrix();

    // convert bounding from local to global
    glm::vec3 bbMinL = piece.boundingBoxMin;
    glm::vec3 bbMaxL = piece.boundingBoxMax;

    glm::vec3 bbMinW(+FLT_MAX), bbMaxW(-FLT_MAX);
    for (int i = 0; i < 8; ++i) {
        glm::vec3 cornerL(
            (i & 1) ? bbMaxL.x : bbMinL.x,
            (i & 2) ? bbMaxL.y : bbMinL.y,
            (i & 4) ? bbMaxL.z : bbMinL.z);

        glm::vec3 cornerW = glm::vec3(M * glm::vec4(cornerL, 1.0f));
        bbMinW = glm::min(bbMinW, cornerW);
        bbMaxW = glm::max(bbMaxW, cornerW);
    }

    //ITA margine
    const float EPS = 0.05f; 
    bbMinW -= glm::vec3(EPS);
    bbMaxW += glm::vec3(EPS);

    //inclusion test
    return (
        point.x >= bbMinW.x && point.x <= bbMaxW.x &&
        point.y >= bbMinW.y && point.y <= bbMaxW.y &&
        point.z >= bbMinW.z && point.z <= bbMaxW.z
        );
}

/**
 * @brief Initializes the chess piece selection system
 *
 * Finds all chess pieces in the scene, sets up callbacks for selection handling,
 * and registers key bindings for bounding box visualization.
 *
 * @param eng Reference to the engine instance
 */
void initChessPieceSelection(Eng::Base& eng) {
    // Find all selectable pieces in the scene
    findChessPieces(eng.getRootNode());

    // Register the callback for selection handling
    auto& callbackManager = Eng::CallbackManager::getInstance();
    callbackManager.registerRenderCallback("chessPieceSelection", []() {
        updateChessPieceSelection();
        });

    // Register key binding for bounding box visualization
    callbackManager.registerKeyBinding('v', "Toggle bounding box visualization", [](unsigned char key, int x, int y) {
        showBoundingBoxes = !showBoundingBoxes;

        // Clear or initialize bounding boxes when toggled
        auto& eng = Eng::Base::getInstance();
        if (!showBoundingBoxes) {
            // Clear all bounding box meshes when turning off
            boundingBoxMeshes.clear();
        }
        else {
            // Initial update when turning on
            updateBoundingBoxes(eng);
        }
        });

    // Register a render callback to update bounding boxes each frame when enabled
    callbackManager.registerRenderCallback("boundingBoxUpdate", []() {
        if (showBoundingBoxes) {
            auto& eng = Eng::Base::getInstance();
            updateBoundingBoxes(eng);
        }
        });
}
/**
 * @brief Updates the selection state of chess pieces
 *
 * Checks for pinch gestures from Leap Motion and updates the selection
 * or movement of chess pieces accordingly.
 */
void updateChessPieceSelection() {
    if (!leap) return;

    leap->update();
    const LEAP_TRACKING_EVENT* frame = leap->getCurFrame();

   // if we have at least one hand
    if (frame->nHands == 0) {
        if (selectedPiece) {
            selectedPiece = nullptr;
            isPinching = false;
        }
        return;
    }

    // select first hand detected
    const LEAP_HAND& hand = frame->pHands[0];
    float pinchStrength = hand.pinch_strength;

    // middel point between thumb and index
    const LEAP_DIGIT& thumb = hand.digits[0];
    const LEAP_DIGIT& index = hand.digits[1];

    glm::vec3 thumbTip(thumb.distal.next_joint.x, thumb.distal.next_joint.y, thumb.distal.next_joint.z);
    glm::vec3 indexTip(index.distal.next_joint.x, index.distal.next_joint.y, index.distal.next_joint.z);

    //from mm to meters
    const float LEAP_TO_WORLD = 0.001f;
    thumbTip *= LEAP_TO_WORLD;
    indexTip *= LEAP_TO_WORLD;

    glm::vec3 pinchLocal = glm::vec3(index.distal.next_joint.x,
        index.distal.next_joint.y,
        index.distal.next_joint.z) * 0.001f;


    // world matrix
    glm::mat4 handsToWorld = handsNode->getFinalMatrix();
    glm::vec3 pinchWorld = glm::vec3(handsToWorld * glm::vec4(pinchLocal, 1.0f));
    glm::vec3 pinchPoint = pinchWorld;

    if (selectablePieces.size() > 0) {
        auto& piece = selectablePieces[0];
        glm::vec3 piecePos = glm::vec3(piece.node->getFinalMatrix()[3]);
    }

    // start pinch
    if (pinchStrength > pinchThreshold && !isPinching) {
        isPinching = true;

        // find closest obj
        float closestDistance = std::numeric_limits<float>::max();
        SelectablePiece* closestPiece = nullptr;

        for (auto& piece : selectablePieces) {
            // test if is within box coords
            if (isPointInBoundingBox(pinchPoint, piece)) {

                /// calculate distance
                glm::vec3 piecePos = glm::vec3(piece.node->getFinalMatrix()[3]);
                float dist = glm::distance(pinchPoint, piecePos);

                if (dist < closestDistance) {
                    closestDistance = dist;
                    closestPiece = &piece;
                }
            }
        }

        // if we arent in a bounding box, we select another object within a max distance threshold
        if (!closestPiece) {
            const float MAX_SELECTION_DISTANCE = 0.001f;

            for (auto& piece : selectablePieces) {
                glm::vec3 piecePos = glm::vec3(piece.node->getFinalMatrix()[3]);
                float distance = glm::distance(pinchPoint, piecePos);

                if (distance < closestDistance && distance < MAX_SELECTION_DISTANCE) {
                    closestDistance = distance;
                    closestPiece = &piece;
                }
            }
        }

        if (closestPiece) {
            // select piece
            closestPiece->isSelected = true;
            selectedPiece = closestPiece->node;

            // save original material
            if (!closestPiece->originalMaterial && closestPiece->mesh) {
                closestPiece->originalMaterial = closestPiece->mesh->getMaterial();
            }

            // change albedo to glow 
            if (closestPiece->mesh && closestPiece->originalMaterial) {
                
                auto highlightMaterial = std::make_shared<Eng::HolographicMaterial>(
                    glm::vec3(1.f, 0.f, 0.f),
                    1.0f,
                    1.0f,
                    0.f
                );
                highlightMaterial->setSecondaryColor(glm::vec3(1.f, 0.f, .0f));
                closestPiece->mesh->setMaterial(highlightMaterial);
            }

            // calculate offset for updated position
            glm::vec3 objectPos = glm::vec3(closestPiece->node->getFinalMatrix()[3]);
            grabOffset = objectPos - pinchPoint;

        }
    }
    // release object
    else if (pinchStrength <= pinchThreshold && isPinching) {
        isPinching = false;

        if (selectedPiece) {
            // restore material
            for (auto& piece : selectablePieces) {
                if (piece.node == selectedPiece && piece.isSelected) {
                    piece.isSelected = false;
                    if (piece.mesh && piece.originalMaterial) {
                        piece.mesh->setMaterial(piece.originalMaterial);
                    }
                    break;
                }
            }

            selectedPiece = nullptr;
        }
    }
    // update position while pinching
    else if (isPinching && selectedPiece) {
        // calculate new position
        glm::vec3 newPos = pinchPoint + grabOffset;

        // find piece in list
        for (auto& piece : selectablePieces) {
            if (piece.node == selectedPiece) {
                // get parent node
                Eng::Node* parent = piece.node->getParent();
                if (parent) {
                    // get local
                    glm::mat4 parentMatrix = parent->getFinalMatrix();
                    glm::mat4 invParentMatrix = glm::inverse(parentMatrix);
                    glm::vec3 localPos = glm::vec3(invParentMatrix * glm::vec4(newPos, 1.0f));

                    // update only translation (scaling and rotation are fine)
                    glm::mat4 newMatrix = piece.node->getLocalMatrix();
                    newMatrix[3] = glm::vec4(localPos, 1.0f);
                    piece.node->setLocalMatrix(newMatrix);

                    // IMPORTANT!!!!! UPDATE ORIGINAL MATRIX TO REFLECT CHANGES IN POSITION!!!!
                    piece.originalMatrix = newMatrix;

                }
                break;
            }
        }
}
}


/**
 * @brief Sets up the Leap Motion controller for hand tracking
 *
 * Initializes the Leap Motion hardware, creates necessary scene nodes,
 * and sets up callback for hand visualization updates.
 *
 * @param eng Reference to the engine instance
 */
void setupLeapMotion(Eng::Base& eng) {
    // Initialize Leap Motion
    leap = new Leap();
    if (!leap->init()) {
        std::cout << "[ERROR] Unable to init Leap Motion" << std::endl;
        delete leap;
        leap = nullptr;
        return;
    }

    auto head = eng.getHeadNode();
    handsNode = std::make_shared<Eng::Node>();
    handsNode->setName("LeapMotionHands");

    const float HAND_DISTANCE = 0.3f;  // 1 m in front
    glm::mat4 forward = glm::translate(
        glm::mat4(1.0f),
        glm::vec3(0.0f, -0.1f, -HAND_DISTANCE)
    );
    handsNode->setLocalMatrix(forward);


    head->addChild(handsNode);
    handsNode->setParent(head.get());

    // Create shared sphere mesh only once
    static std::shared_ptr<Eng::Mesh> sphereMesh = nullptr;
    if (!sphereMesh) {
        sphereMesh = createSphereMesh(0.005f);
        auto mat = std::make_shared<Eng::Material>(glm::vec3(1, 0, 0), 1.0f, 0.2f, glm::vec3(0));
        sphereMesh->setMaterial(mat);
        sphereMesh->initBuffers();
    }

    if (!cylinderMesh) {
        cylinderMesh = createCylinderMesh(0.002f, 1.0f);
        auto mat = std::make_shared<Eng::Material>(glm::vec3(1), 1.0f, 0.0f, glm::vec3(0));
        cylinderMesh->setMaterial(mat);
        cylinderMesh->initBuffers();
    }

    // Pre-create mesh instances for all joints
    if (jointMeshes.empty()) {
        jointMeshes.reserve(MAX_HANDS * JOINTS_PER_HAND);
        for (int i = 0; i < MAX_HANDS * JOINTS_PER_HAND; ++i) {
            // Create a mesh that shares vertices/indices with sphereMesh
            auto jointMesh = std::make_shared<Eng::Mesh>();

            // Share the same vertex and index data
            jointMesh->setVertices(sphereMesh->getVertices());
            jointMesh->setIndices(sphereMesh->getIndices());
            jointMesh->setMaterial(sphereMesh->getMaterial());

            // Initialize buffers for this instance
            jointMesh->initBuffers();

            // Add to the scene graph
            handsNode->addChild(jointMesh);
            jointMesh->setParent(handsNode.get());

            jointMeshes.push_back(jointMesh);
        }
    }

    // Create one Node-per-bone
    const int bonesPerHand = 2 + 5 * 4;
    if (boneNodes.empty()) {
        boneNodes.reserve(MAX_HANDS * bonesPerHand);
        for (int i = 0; i < MAX_HANDS * bonesPerHand; ++i) {
            auto boneNode = std::make_shared<Eng::Node>();
            auto meshInst = std::make_shared<Eng::Mesh>();
            meshInst->setVertices(cylinderMesh->getVertices());
            meshInst->setIndices(cylinderMesh->getIndices());
            meshInst->setMaterial(cylinderMesh->getMaterial());
            meshInst->initBuffers();
            boneNode->addChild(meshInst);
            meshInst->setParent(boneNode.get());

            handsNode->addChild(boneNode);
            boneNode->setParent(handsNode.get());
            boneNodes.push_back(boneNode);
        }
    }
}

/**
 * @brief Updates the visual representation of hands based on Leap Motion data
 *
 * Processes the current frame from Leap Motion and updates the position
 * and orientation of all joint meshes and bone connections.
 */
void updateLeapHands() {
    if (!leap || !handsNode || !leapVisualizationEnabled) return;

    leap->update();
    const LEAP_TRACKING_EVENT* frame = leap->getCurFrame();
    const float LEAP_TO_WORLD = 0.001f;  // mm → m
    size_t jointIndex = 0;

    bool hasActiveHands = (frame->nHands > 0);

    // Position all joint spheres
    for (unsigned h = 0; h < frame->nHands && h < MAX_HANDS; ++h) {
        const LEAP_HAND& hand = frame->pHands[h];
        glm::vec3 handColor = (h == 0) ? glm::vec3(0.2f, 0.8f, 0.2f) : glm::vec3(0.2f, 0.2f, 0.8f);

        auto setJoint = [&](const glm::vec3& pos) {
            if (jointIndex >= jointMeshes.size()) return;
            jointMeshes[jointIndex]
                ->setMaterial(std::make_shared<Eng::Material>(handColor, 1.0f, 0.2f, glm::vec3(0)));
            jointMeshes[jointIndex++]
                ->setLocalMatrix(glm::translate(glm::mat4(1.0f), pos));
            };

        // Elbow, Wrist, Palm 
        glm::vec3 elbowPos = glm::vec3(hand.arm.prev_joint.x, hand.arm.prev_joint.y, hand.arm.prev_joint.z) * LEAP_TO_WORLD;
        glm::vec3 wristPos = glm::vec3(hand.arm.next_joint.x, hand.arm.next_joint.y, hand.arm.next_joint.z) * LEAP_TO_WORLD;
        glm::vec3 palmPos = glm::vec3(hand.palm.position.x, hand.palm.position.y, hand.palm.position.z) * LEAP_TO_WORLD;

        if (glm::length(elbowPos) > 0.001f) setJoint(elbowPos);
        else setJoint(glm::vec3(0.0f));

        if (glm::length(wristPos) > 0.001f) setJoint(wristPos);
        else setJoint(glm::vec3(0.0f));

        if (glm::length(palmPos) > 0.001f) setJoint(palmPos);
        else setJoint(glm::vec3(0.0f));

        // Finger bones
        for (unsigned d = 0; d < 5; ++d) {
            const LEAP_DIGIT& finger = hand.digits[d];
            for (unsigned b = 0; b < 4; ++b) {
                const LEAP_BONE& bone = finger.bones[b];
                glm::vec3 jointPos = glm::vec3(bone.next_joint.x, bone.next_joint.y, bone.next_joint.z) * LEAP_TO_WORLD;
                if (glm::length(jointPos) > 0.001f) {
                    setJoint(jointPos);
                }
                else {
                    setJoint(glm::vec3(0.0f));
                }
            }
        }
    }

    // Hide any extra spheres
    while (jointIndex < jointMeshes.size()) {
        jointMeshes[jointIndex++]
            ->setLocalMatrix(glm::scale(glm::mat4(1.0f), glm::vec3(0.0f)));
    }

    // Build flat list of joint positions
    std::vector<glm::vec3> J;
    J.reserve(jointMeshes.size());
    for (auto& jm : jointMeshes) {
        glm::mat4 M = jm->getLocalMatrix();
        J.push_back(glm::vec3(M[3]));  // extract translation
    }

    std::vector<std::pair<int, int>> bonePairs;

    // Hide bones while hands are not visible
    if (!hasActiveHands) {
        for (auto& node : boneNodes) {
            node->setLocalMatrix(glm::scale(glm::mat4(1.0f), glm::vec3(0.0f)));
        }
        return;
    }

    for (int h = 0; h < MAX_HANDS && h < frame->nHands; ++h) {
        int base = h * JOINTS_PER_HAND;
        for (int f = 0; f < 5; ++f) {
            int fb = base + 3 + f * 4;  // Base index per questa dita
            for (int b = 0; b < 3; ++b) {
                if (glm::length(J[fb + b]) > 0.001f && glm::length(J[fb + b + 1]) > 0.001f) {
                    bonePairs.emplace_back(fb + b, fb + b + 1);
                }
            }
        }
    }

    // Stretch & orient each cylinder‐bone node to match its joint pair
    for (size_t i = 0; i < bonePairs.size() && i < boneNodes.size(); ++i) {
        auto [a, b] = bonePairs[i];
        glm::vec3 A = J[a], B = J[b];
        glm::vec3 dir = B - A;
        float len = glm::length(dir);
        auto node = boneNodes[i];

        if (len < 1e-5f) {
            node->setLocalMatrix(glm::scale(glm::mat4(1.0f), glm::vec3(0.0f)));
            continue;
        }

        // Translate to the midpoint
        glm::vec3 mid = 0.5f * (A + B);
        glm::mat4 T = glm::translate(glm::mat4(1.0f), mid);

        // Compute the axis & angle from +Z to our bone direction
        glm::vec3 up = glm::vec3(0, 0, 1);
        glm::vec3 ndir = glm::normalize(dir);

        // Avoid problems if ndir is parallel to up vector
        if (glm::abs(glm::dot(up, ndir)) > 0.999f) {
            if (glm::dot(up, ndir) > 0) {
                node->setLocalMatrix(glm::translate(glm::mat4(1.0f), mid) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, len)));
            }
            else {
                node->setLocalMatrix(glm::translate(glm::mat4(1.0f), mid) *
                    glm::rotate(glm::mat4(1.0f), glm::pi<float>(), glm::vec3(1, 0, 0)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, len)));
            }
            continue;
        }

        glm::vec3 axis = glm::cross(up, ndir);
        float cosA = glm::clamp(glm::dot(up, ndir), -1.0f, 1.0f);
        float angle = acos(cosA);

        // Build a rotation matrix around that axis
        glm::mat4 R = glm::rotate(glm::mat4(1.0f), angle, axis);

        // Scale so our unit‐height cylinder stretches to 'len'
        glm::mat4 S = glm::scale(glm::mat4(1.0f),
            glm::vec3(1.0f, 1.0f, len));

        // Combine: T * R * S
        node->setLocalMatrix(T * R * S);
    }

    // Hide extra bones
    for (size_t i = bonePairs.size(); i < boneNodes.size(); ++i) {
        boneNodes[i]->setLocalMatrix(glm::scale(glm::mat4(1.0f), glm::vec3(0.0f)));
    }
}

/**
 * @brief Creates a sphere mesh for joint visualization
 *
 * Generates a sphere with the specified radius for representing hand joints
 * in the Leap Motion visualization.
 *
 * @param radius The radius of the sphere in meters
 * @return A shared pointer to the created mesh
 */
std::shared_ptr<Eng::Mesh> createSphereMesh(float radius) {
    std::vector<Eng::Vertex> vertices;
    std::vector<unsigned int> indices;

    int gradation = 10;

    // Create sphere vertices
    for (int lat = 0; lat <= gradation; lat++) {
        float theta = lat * glm::pi<float>() / gradation;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);

        for (int lon = 0; lon <= gradation; lon++) {
            float phi = lon * 2 * glm::pi<float>() / gradation;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);

            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;

            Eng::Vertex vertex;
            vertex.setPosition(glm::vec3(x, y, z) * radius);
            vertex.setNormal(glm::normalize(glm::vec3(x, y, z)));
            vertex.setTexCoords(glm::vec2((float)lon / gradation, (float)lat / gradation));

            vertices.push_back(vertex);
        }
    }

    // Create indices
    for (int lat = 0; lat < gradation; lat++) {
        for (int lon = 0; lon < gradation; lon++) {
            int first = (lat * (gradation + 1)) + lon;
            int second = first + gradation + 1;

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }

    auto& builder = Eng::Builder::getInstance();
    return builder
        .setName(std::string("SphereMesh"))
        .addVertices(vertices)
        .addIndices(indices)
        .build();
}

/**
 * @brief Creates a cylinder mesh for bone visualization
 *
 * Generates a cylinder with the specified radius and height for representing
 * bone connections between joints in the Leap Motion visualization.
 *
 * @param radius The radius of the cylinder in meters
 * @param height The height of the cylinder in meters
 * @return A shared pointer to the created mesh
 */
std::shared_ptr<Eng::Mesh> createCylinderMesh(float radius, float height) {
    // build a vertical cylinder (centered at origin, from z=0 to z=height)
    const int slices = 12;
    std::vector<Eng::Vertex> verts;
    std::vector<unsigned int> idx;

    for (int i = 0; i <= slices; ++i) {
        float angle = i * 2.0f * glm::pi<float>() / slices;
        float x = cos(angle) * radius;
        float y = sin(angle) * radius;
        // bottom circle
        Eng::Vertex vb; vb.setPosition({ x, y, 0.0f }); vb.setNormal({ x, y, 0 });
        // top circle
        Eng::Vertex vt; vt.setPosition({ x, y, height }); vt.setNormal({ x, y, 0 });
        verts.push_back(vb);
        verts.push_back(vt);
    }
    // build side quads as two triangles each slice
    for (int i = 0; i < slices; ++i) {
        unsigned int b0 = 2 * i, t0 = 2 * i + 1;
        unsigned int b1 = 2 * (i + 1), t1 = 2 * (i + 1) + 1;
        idx.insert(idx.end(), { b0, t0, b1,  t0, t1, b1 });
    }

    auto& B = Eng::Builder::getInstance();
    return B.setName("BoneCylinder")
        .addVertices(verts)
        .addIndices(idx)
        .build();
}

/**
 * @brief Sets up multiple cameras with different viewpoints
 *
 * Creates and configures several camera objects with different positions and orientations,
 * and registers key bindings for camera control and switching.
 *
 * @param eng Reference to the engine instance
 */
void setUpCameras(Eng::Base &eng) {
   static std::vector<std::shared_ptr<Eng::PerspectiveCamera> > cameras;
   auto &callbackManager = Eng::CallbackManager::getInstance();
   float initialAspect = eng.getWindowAspectRatio();


   // Camera 1
   auto camera1 = std::make_shared<Eng::PerspectiveCamera>(45.0f, initialAspect, 0.1f, 50.0f);
   glm::vec3 cameraPos1(-1.5f, 1.8f, -0.6f);
   glm::vec3 lookAtPoint1(-.6f, 0.2f, -0.6f);
   glm::vec3 upVector1(0.0f, 1.0f, 0.0f);
   camera1->setLocalMatrix(glm::lookAt(cameraPos1, lookAtPoint1, upVector1));
   camera1->setName("Main Camera");
   cameras.push_back(camera1);

   // Camera 2
   auto camera2 = std::make_shared<Eng::PerspectiveCamera>(45.0f, initialAspect, 0.1f, 1000000.0f);
   glm::vec3 cameraPos2(-.1f, 1.6f, -0.6f);
   glm::vec3 lookAtPoint2(-6.0f, 0.2f, -0.6f);
   glm::vec3 upVector2(0.0f, 1.0f, 0.0f);
   camera2->setLocalMatrix(glm::lookAt(cameraPos2, lookAtPoint2, upVector2));
   camera2->setName("Second Camera");
   cameras.push_back(camera2);

   // Camera 3
   auto camera3 = std::make_shared<Eng::PerspectiveCamera>(45.0f, initialAspect, 0.1f, 1000000.0f);
   glm::vec3 cameraPos3(-3.0f, 2.f, -3.f);
   glm::vec3 lookAtPoint3(2.0f, 3.f, 6.f);
   glm::vec3 upVector3(0.0f, 1.0f, 0.0f);
   camera3->setLocalMatrix(glm::lookAt(cameraPos3, lookAtPoint3, upVector3));
   camera3->setName("Third Camera");
   cameras.push_back(camera3);

   // Camera 4
   auto camera4 = std::make_shared<Eng::PerspectiveCamera>(45.0f, initialAspect, 0.1f, 1000000.0f);
   glm::vec3 cameraPos4(5.0f, 3.f, 9.f);
   glm::vec3 lookAtPoint4(0.0f, 3.f, 0.f);
   glm::vec3 upVector4(0.0f, 1.0f, 0.0f);
   camera4->setLocalMatrix(glm::lookAt(cameraPos4, lookAtPoint4, upVector4));
   camera4->setName("Fourth Camera");
   cameras.push_back(camera4);

   // Set initial active camera
   eng.SetActiveCamera(cameras.front());

   constexpr float moveSpeed = 0.1f;
   constexpr float rotationSpeed = 0.05f;

   //register camera controls
   callbackManager.registerKeyBinding('w', "Move camera forward",
                                       [&](unsigned char key, int x, int y) {
                                           auto camera = eng.getActiveCamera();
                                           if (!camera) return;

                                           glm::mat4 currentMatrix = camera->getLocalMatrix();
                                           glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, moveSpeed));
                                           glm::mat4 updatedMatrix = translation * currentMatrix;

                                           camera->setLocalMatrix(updatedMatrix);
                                       });


   callbackManager.registerKeyBinding('s', "Move camera backward",
                                      [&](unsigned char key, int x, int y) {
                                         auto camera = eng.getActiveCamera();
                                         if (!camera) return;
                                         glm::mat4 currentMatrix = camera->getLocalMatrix();
                                         camera->setLocalMatrix(
                                            glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -moveSpeed)) *
                                            currentMatrix
                                         );
                                      });

   callbackManager.registerKeyBinding('a', "Rotate camera left / Move left",
                                      [&](unsigned char key, int x, int y) {
                                         auto camera = eng.getActiveCamera();
                                         if (!camera) return;
                                         glm::mat4 currentMatrix = camera->getLocalMatrix();
                                         const auto currentPosition = glm::vec3(currentMatrix[3]);

                                         if (camera == cameras[0]) {
                                            //is main camera
                                            glm::mat4 toOrigin = glm::translate(glm::mat4(1.0f), -currentPosition);
                                            glm::mat4 rotation = glm::rotate(
                                               glm::mat4(1.0f), rotationSpeed, glm::vec3(0.0f, 1.0f, 1.0f));
                                            glm::mat4 backToPos = glm::translate(glm::mat4(1.0f), currentPosition);
                                            glm::mat4 transformMatrix = backToPos * rotation * toOrigin;
                                            camera->setLocalMatrix(transformMatrix * currentMatrix);
                                         } else {
                                            glm::mat4 transformMatrix = glm::translate(
                                               glm::mat4(1.0f), glm::vec3(moveSpeed, 0.0f, 0.0f));
                                            camera->setLocalMatrix(transformMatrix * currentMatrix);
                                         }
                                      });

   callbackManager.registerKeyBinding('d', "Rotate camera right / Move right",
                                      [&](unsigned char key, int x, int y) {
                                         auto camera = eng.getActiveCamera();
                                         if (!camera) return;
                                         glm::mat4 currentMatrix = camera->getLocalMatrix();
                                         const auto currentPosition = glm::vec3(currentMatrix[3]);

                                         if (camera == cameras[0]) {
                                            //is main camera
                                            glm::mat4 toOrigin = glm::translate(glm::mat4(1.0f), -currentPosition);
                                            glm::mat4 rotation = glm::rotate(
                                               glm::mat4(1.0f), -rotationSpeed, glm::vec3(0.0f, 1.0f, 1.0f));
                                            glm::mat4 backToPos = glm::translate(glm::mat4(1.0f), currentPosition);
                                            glm::mat4 transformMatrix = backToPos * rotation * toOrigin;
                                            camera->setLocalMatrix(transformMatrix * currentMatrix);
                                         } else {
                                            glm::mat4 transformMatrix = glm::translate(
                                               glm::mat4(1.0f), glm::vec3(-moveSpeed, 0.0f, 0.0f));
                                            camera->setLocalMatrix(transformMatrix * currentMatrix);
                                         }
                                      });

   callbackManager.registerKeyBinding('c', "Switch camera",
                                      [&eng](unsigned char key, int x, int y) {
                                         static size_t currentCameraIndex = 0;
                                         currentCameraIndex = (currentCameraIndex + 1) % cameras.size();
                                         const auto &nextCamera = cameras[currentCameraIndex];
                                         if (auto perspCamera = std::dynamic_pointer_cast<Eng::PerspectiveCamera>(
                                            nextCamera)) {
                                            perspCamera->setAspect(eng.getWindowAspectRatio());
                                         }
                                         eng.SetActiveCamera(nextCamera);
                                      });
}
