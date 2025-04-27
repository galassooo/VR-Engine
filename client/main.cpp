#include "engine.h"
#include <iostream>
#include <functional>
#include <random>
#include <array>

#include "leap.h"

// Extern to request the use of high performance GPUs when available (Nvidia or AMD)
extern "C" {
    _declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    _declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

// Skybox
std::vector<std::string> myCubemapFaces = {
    "../resources/right.png", // right
    "../resources/left.png", // left
    "../resources/top.png", // top
    "../resources/bottom.png", // bottom
    "../resources/front.png", // front
    "../resources/back.png"  // back
};

// Leap Motion
Leap* leap = nullptr;
std::shared_ptr<Eng::Node> handsNode = nullptr; // Root node for hands visualization
bool leapVisualizationEnabled = true;

static const int MAX_HANDS = 2;
static const int JOINTS_PER_HAND = 3 + (5 * 4);  // elbow, wrist, palm + 5 fingers * 4 bones
static std::vector<std::shared_ptr<Eng::Mesh>> jointMeshes;

static std::shared_ptr<Eng::Mesh> cylinderMesh = nullptr;
static std::vector<std::shared_ptr<Eng::Node>> boneNodes;

// Update hands
void updateLeapHands();

std::shared_ptr<Eng::Mesh> createSphereMesh(float radius = 5.0f);
std::shared_ptr<Eng::Mesh> createCylinderMesh(float radius = 0.002f, float height = 1.0f);

// LepMotion setup
void setupLeapMotion(Eng::Base& eng);

/**
 * @brief Sets up multiple cameras and their controls.
 *
 * This function initializes cameras at different positions and allows the
 * user to switch between them and control their movement and rotation.
 *
 * @param eng Reference to the engine instance.
 */
void setUpCameras(Eng::Base &eng);

/**
 * @brief Entry point of the client application.
 *
 * This function initializes the engine, loads the scene, and sets up various
 * functionalities including chess movement, mirror effects, light controls,
 * and camera controls.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Returns 0 on success, -1 on initialization failure.
 */
int main(int argc, char *argv[]) {
   std::cout << "Client application example, K. Quarenghi & M. Galasso (C) SUPSI" << std::endl;
   std::cout << std::endl;


   Eng::Base &eng = Eng::Base::getInstance();
   if (!eng.init()) {
      return -1;
   }

   eng.loadScene("..\\resources\\Chess.ovo");

   // Dopo aver caricato la scena in main.cpp
   // Setup Leap Motion with its own render callback
   setupLeapMotion(eng);

   eng.engEnable(ENG_STEREO_RENDERING);

   setUpCameras(eng);

   eng.registerSkybox(myCubemapFaces);

   eng.run();

   // Clean up Leap Motion before engine cleanup
   if (leap) {
       leap->free();
       delete leap;
       leap = nullptr;
   }

   eng.free();

   return 0;
}

#include <deque>

// Updated setupLeapMotion function with proper mesh reuse
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
        sphereMesh = createSphereMesh(0.005f);  // Increase size to 5.0f
        auto mat = std::make_shared<Eng::Material>(glm::vec3(1, 0, 0), 1.0f, 0.2f);
        sphereMesh->setMaterial(mat);
        sphereMesh->initBuffers();
    }

    if (!cylinderMesh) {
        cylinderMesh = createCylinderMesh(0.002f, 1.0f);
        auto mat = std::make_shared<Eng::Material>(glm::vec3(1), 1.0f, 0.0f);
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

    // 2) Create one Node-per-bone
    //    bones per hand = 2 (elbow→wrist, wrist→palm) + 5*4 finger bones = 22
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

    // Register update callback for Leap Motion
    auto& callbackManager = Eng::CallbackManager::getInstance();
    callbackManager.registerRenderCallback("leapMotionUpdate", []() {
        updateLeapHands();
        });

    // Register a key binding to toggle Leap Motion visualization
    callbackManager.registerKeyBinding('l', "Toggle Leap Motion Visualization", [](unsigned char key, int x, int y) {
        leapVisualizationEnabled = !leapVisualizationEnabled;
        if (handsNode) {
            if (!leapVisualizationEnabled) {
                // Hide all joint meshes
                for (auto& jointMesh : jointMeshes) {
                    jointMesh->setLocalMatrix(glm::scale(glm::mat4(1.0f), glm::vec3(0.0f)));
                }
            }
        }
        });

    std::cout << "Leap Motion initialized successfully. Press 'L' to toggle hand visualization." << std::endl;
}

void updateLeapHands() {
    if (!leap || !handsNode || !leapVisualizationEnabled) return;

    leap->update();
    const LEAP_TRACKING_EVENT* frame = leap->getCurFrame();
    const float LEAP_TO_WORLD = 0.001f;  // mm → m
    size_t jointIndex = 0;

    bool hasActiveHands = (frame->nHands > 0);

    // 1) Position all joint spheres
    for (unsigned h = 0; h < frame->nHands && h < MAX_HANDS; ++h) {
        const LEAP_HAND& hand = frame->pHands[h];
        glm::vec3 handColor = (h == 0) ? glm::vec3(0.2f, 0.8f, 0.2f) : glm::vec3(0.2f, 0.2f, 0.8f);

        auto setJoint = [&](const glm::vec3& pos) {
            if (jointIndex >= jointMeshes.size()) return;
            jointMeshes[jointIndex]
                ->setMaterial(std::make_shared<Eng::Material>(handColor, 1.0f, 0.2f));
            jointMeshes[jointIndex++]
                ->setLocalMatrix(glm::translate(glm::mat4(1.0f), pos));
            };

        // Elbow, Wrist, Palm - manteniamo questi punti visibili
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

    // 2) Build flat list of joint positions
    std::vector<glm::vec3> J;
    J.reserve(jointMeshes.size());
    for (auto& jm : jointMeshes) {
        glm::mat4 M = jm->getLocalMatrix();
        J.push_back(glm::vec3(M[3]));  // extract translation
    }

    // 3) Definire solo le connessioni tra le giunture delle dita
    std::vector<std::pair<int, int>> bonePairs;

    // Nascondi tutti gli ossi se non ci sono mani attive
    if (!hasActiveHands) {
        for (auto& node : boneNodes) {
            node->setLocalMatrix(glm::scale(glm::mat4(1.0f), glm::vec3(0.0f)));
        }
        return;
    }

    for (int h = 0; h < MAX_HANDS && h < frame->nHands; ++h) {
        int base = h * JOINTS_PER_HAND;

        // RIMOSSO collegamenti a gomito, polso e palmo
        // Manteniamo solo le dita

        for (int f = 0; f < 5; ++f) {
            int fb = base + 3 + f * 4;  // Base index per questa dita
            for (int b = 0; b < 3; ++b) {
                if (glm::length(J[fb + b]) > 0.001f && glm::length(J[fb + b + 1]) > 0.001f) {
                    bonePairs.emplace_back(fb + b, fb + b + 1);
                }
            }
        }
    }

    // 4) Stretch & orient each cylinder‐bone node to match its joint pair
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

        // 1) Translate to the midpoint
        glm::vec3 mid = 0.5f * (A + B);
        glm::mat4 T = glm::translate(glm::mat4(1.0f), mid);

        // 2) Compute the axis & angle from +Z to our bone direction
        glm::vec3 up = glm::vec3(0, 0, 1);
        glm::vec3 ndir = glm::normalize(dir);

        // Evita problemi se ndir è parallelo a up
        if (glm::abs(glm::dot(up, ndir)) > 0.999f) {
            // Usiamo un vettore diverso se sono paralleli
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

        // 3) Build a rotation matrix around that axis
        glm::mat4 R = glm::rotate(glm::mat4(1.0f), angle, axis);

        // 4) Scale so our unit‐height cylinder stretches to 'len'
        glm::mat4 S = glm::scale(glm::mat4(1.0f),
            glm::vec3(1.0f, 1.0f, len));

        // 5) Combine: T * R * S
        node->setLocalMatrix(T * R * S);
    }

    // Nascondi gli ossi rimanenti
    for (size_t i = bonePairs.size(); i < boneNodes.size(); ++i) {
        boneNodes[i]->setLocalMatrix(glm::scale(glm::mat4(1.0f), glm::vec3(0.0f)));
    }
}


// Create the sphere mesh (same as before but with optimized parameters)
std::shared_ptr<Eng::Mesh> createSphereMesh(float radius) {
    std::vector<Eng::Vertex> vertices;
    std::vector<unsigned int> indices;

    int gradation = 10;  // Same as your teacher's implementation

    // Create sphere vertices (same as teacher's implementation)
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
        // quad: b0,t0,b1  and  t0,t1,b1
        idx.insert(idx.end(), { b0, t0, b1,  t0, t1, b1 });
    }

    auto& B = Eng::Builder::getInstance();
    return B.setName("BoneCylinder")
        .addVertices(verts)
        .addIndices(idx)
        .build();
}


void setUpCameras(Eng::Base &eng) {
   static std::vector<std::shared_ptr<Eng::PerspectiveCamera> > cameras;
   auto &callbackManager = Eng::CallbackManager::getInstance();
   float initialAspect = eng.getWindowAspectRatio();


   // Camera 1
   auto camera1 = std::make_shared<Eng::PerspectiveCamera>(45.0f, initialAspect, 0.1f, 50.0f);
   glm::vec3 cameraPos1(0.0f, 4.f, 3.f);
   glm::vec3 lookAtPoint1(0.0f, 0.f, 0.f);
   glm::vec3 upVector1(0.0f, 1.0f, 0.0f);
   camera1->setLocalMatrix(glm::lookAt(cameraPos1, lookAtPoint1, upVector1));
   camera1->setName("Main Camera");
   cameras.push_back(camera1);

   // Camera 2
   auto camera2 = std::make_shared<Eng::PerspectiveCamera>(45.0f, initialAspect, 0.1f, 1000000.0f);
   glm::vec3 cameraPos2(-3.0f, 4.f, 7.f);
   glm::vec3 lookAtPoint2(0.0f, 0.f, 0.f);
   glm::vec3 upVector2(0.0f, 1.0f, 0.0f);
   camera2->setLocalMatrix(glm::lookAt(cameraPos2, lookAtPoint2, upVector2));
   camera2->setName("Second Camera");
   cameras.push_back(camera2);

   // Camera 3
   auto camera3 = std::make_shared<Eng::PerspectiveCamera>(45.0f, initialAspect, 0.1f, 1000000.0f);
   glm::vec3 cameraPos3(-0.0f, 3.f, 17.f);
   glm::vec3 lookAtPoint3(0.0f, 3.f, 0.f);
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

void setupMirrorEffect(Eng::Base &engine) {
   auto root = engine.getRootNode();
   if (!root) return;

   auto boardNode = [&root]() -> std::shared_ptr<Eng::Node> {
      std::function<std::shared_ptr<Eng::Node>(const std::shared_ptr<Eng::Node> &)> find =
            [&find](const std::shared_ptr<Eng::Node> &node) -> std::shared_ptr<Eng::Node> {
         if (!node) return nullptr;
         if (node->getName() == "Board") return node;
         for (auto &child: *node->getChildren()) {
            if (auto found = find(child)) return found;
         }
         return nullptr;
      };
      return find(root);
   }();

   if (!boardNode) {
      std::cout << "Board node not found!" << std::endl;
      return;
   }

   // Make squares transparent
   for (auto &square: *boardNode->getChildren()) {
      if (!square) continue;

      if (auto squareMesh = std::dynamic_pointer_cast<Eng::Mesh>(square)) {
         if (auto material = squareMesh->getMaterial()) {
            auto transparentMaterial = std::make_shared<Eng::Material>(
               material->getAlbedo(),
               0.4f,
               0.0f
            );
            squareMesh->setMaterial(transparentMaterial);
         }
      }

      // Create reflections for pieces
      for (auto &piece: *square->getChildren()) {
         if (!piece) continue;
         if (auto mesh = std::dynamic_pointer_cast<Eng::Mesh>(piece)) {
            std::string name = piece->getName();
            if (name.find('P') != std::string::npos ||
                name.find('R') != std::string::npos ||
                name.find('H') != std::string::npos ||
                name.find('B') != std::string::npos ||
                name.find('Q') != std::string::npos ||
                name.find('K') != std::string::npos) {
               auto reflectedMesh = std::make_shared<Eng::Mesh>();
               reflectedMesh->setName(name + "_reflection");
               reflectedMesh->setVertices(mesh->getVertices());
               reflectedMesh->setIndices(mesh->getIndices());
               reflectedMesh->setMaterial(mesh->getMaterial());

               glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, -1.0f, 1.0f));
               reflectedMesh->setLocalMatrix(scaleMatrix);

               mesh->addChild(reflectedMesh);
               reflectedMesh->setParent(mesh.get());
            }
         }
      }
   }
}
