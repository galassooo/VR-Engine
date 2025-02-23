#include "engine.h"
#include <iostream>
#include <functional>
#include <random>
#include <array>
/**
 * @brief Sets up the chess piece movement logic.
 *
 * This function initializes the chessboard, manages piece selection,
 * movement, and captures, and provides an undo functionality for moves.
 *
 * @param eng Reference to the engine instance.
 */
void setupChessMovement(Eng::Base &eng);

/**
 * @brief Sets up the mirror effect on the chessboard.
 *
 * This function makes the squares transparent and adds a reflective effect
 * to the chess pieces.
 *
 * @param engine Reference to the engine instance.
 */
void setupMirrorEffect(Eng::Base &engine);

/**
 * @brief Sets up controls for manipulating the light source in the scene.
 *
 * This function allows the user to move the light source and change its color.
 *
 * @param eng Reference to the engine instance.
 */
void setupLightControls(Eng::Base &eng);

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
   eng.loadScene("C:\\Users\\kevin\\Desktop\\SemesterProject\\addons\\bin\\output.ovo");

   setupMirrorEffect(eng);
   setUpCameras(eng);
   setupLightControls(eng);
   setupChessMovement(eng);

   eng.run();
   eng.free();

   return 0;
}

#include <deque>


/**
 * @brief Structure to store the state of a chess move.
 */
struct Move {
   glm::ivec2 fromPosition; /**< The starting position of the move. */
   glm::ivec2 toPosition;   /**< The destination position of the move. */
   std::shared_ptr<Eng::Node> movedPiece; /**< Pointer to the moved piece. */
   std::vector<std::shared_ptr<Eng::Node>> movedPieceChildren; /**< Reflections of the moved piece. */
   std::shared_ptr<Eng::Node> capturedPiece; /**< Pointer to the captured piece, nullptr if no capture. */
   std::vector<std::shared_ptr<Eng::Node>> capturedPieceChildren; /**< Reflections of the captured piece. */
};

/**
 * @brief Vector to store initial state of the board.
 *
 * It is used in the rest functionality.
 */
static std::vector<Move> initialState;

/**
 * @brief Deque to store the history of moves.
 *
 * Maintains the last 10 moves for undo functionality.
 */
static std::deque<Move> moveHistory;

/**
 * @brief Deque to store the history of undo moves.
 *
 * Maintains the last 10 moves for redo functionality.
 */
static std::deque<Move> redoHistory;

/**
 * @brief Maximum number of moves to store in the history.
 */
constexpr size_t MAX_HISTORY = 10;

void setupChessMovement(Eng::Base &eng) {
   auto &callbackManager = Eng::CallbackManager::getInstance();
   auto root = eng.getRootNode();

   // Matrix to store chess squares
   static std::array<std::array<std::shared_ptr<Eng::Node>, 8>, 8> chessBoard;
   static std::array<std::array<glm::vec3, 8>, 8> originalColors;
   static glm::ivec2 selectedPosition(0, 0); // Current position in the matrix
   static std::shared_ptr<Eng::Node> selectedPiece = nullptr;
   static bool isSelectionMode = true; // true = selecting piece, false = selecting destination

   // Find the board node
   std::shared_ptr<Eng::Node> boardNode = nullptr;
   std::function<void(const std::shared_ptr<Eng::Node> &)> findBoard =
         [&findBoard, &boardNode](const std::shared_ptr<Eng::Node> &node) {
      if (!node) return;
      if (node->getName() == "Board") {
         boardNode = node;
         return;
      }
      for (const auto &child: *node->getChildren()) {
         findBoard(child);
      }
   };

   findBoard(root);

   if (!boardNode) {
      std::cout << "Board not found!" << std::endl;
      return;
   }

   // Initialize the chess board matrix
   for (auto &child: *boardNode->getChildren()) {
      std::string name = child->getName();
      if (name.length() == 2 && name[0] >= 'A' && name[0] <= 'H' &&
          name[1] >= '1' && name[1] <= '8') {
         int col = name[0] - 'A';
         int row = name[1] - '1';
         chessBoard[row][col] = child;
         glm::vec3 originalColor = std::dynamic_pointer_cast<Eng::Mesh>(child)->getMaterial()->getAlbedo();
         originalColors[row][col] = originalColor;
      }
   }

   // Helper function to update square colors
   auto updateSquareColors = [&]() {
      for (int i = 0; i < 8; i++) {
         for (int j = 0; j < 8; j++) {
            auto square = chessBoard[i][j];
            if (auto squareMesh = std::dynamic_pointer_cast<Eng::Mesh>(square)) {
               glm::vec3 color(originalColors[i][j]); // Default color
               float alpha = 0.4f; // Default alpha

               if (i == selectedPosition.y && j == selectedPosition.x) {
                  if (isSelectionMode) {
                     color = glm::vec3(1.0f, 1.0f, 0.0f); // Yellow for selection
                     alpha = 1.0f;
                  } else if (!squareMesh->getChildren()->empty() &&
                             squareMesh->getChildren()->front()->getName() != selectedPiece->getName()) {
                     color = glm::vec3(1.0f, 0.0f, 0.0f); // Red for eat
                     alpha = 1.0f;
                  } else {
                     color = glm::vec3(0.0f, 1.0f, 0.0f); // Green for destination
                     alpha = 1.0f;
                  }
               } else if (squareMesh == selectedPiece) {
                  color = glm::vec3(0.0f, 1.0f, 0.0f); // Green for selected piece
                  alpha = 1.0f;
               }

               auto newMaterial = std::make_shared<Eng::Material>(color, alpha, 0.0f);
               squareMesh->setMaterial(newMaterial);
            }
         }
      }
   };

   // Register special key callback for arrow keys
   callbackManager.setSpecialCallback([&](int key, int x, int y) {
      switch (key) {
         case 100: // Left arrow
            selectedPosition.x = std::max(0, selectedPosition.x - 1);
            break;
         case 102: // Right arrow
            selectedPosition.x = std::min(7, selectedPosition.x + 1);
            break;
         case 101: // Up arrow
            selectedPosition.y = std::min(7, selectedPosition.y + 1);
            break;
         case 103: // Down arrow
            selectedPosition.y = std::max(0, selectedPosition.y - 1);
            break;
      }
      updateSquareColors();
   });

   // Register keyboard callback for Enter key
   callbackManager.registerKeyBinding('\r', "Select/Move Chess Piece", [&](unsigned char key, int x, int y) {
      auto currentSquare = chessBoard[selectedPosition.y][selectedPosition.x];

      if (isSelectionMode) {
         // Check if there's a piece to select
         if (!currentSquare->getChildren()->empty()) {
            selectedPiece = currentSquare->getChildren()->front();
            isSelectionMode = false;
         }
      } else {
         if (selectedPiece) {
            auto& targetSquare = chessBoard[selectedPosition.y][selectedPosition.x];

            // Save the state of the move in the deque
            if (moveHistory.size() >= MAX_HISTORY) {
               moveHistory.pop_front(); // Remove the oldest move
            }
            Move move = {
               glm::ivec2(selectedPiece->getParent()->getName()[0] - 'A', selectedPiece->getParent()->getName()[1] - '1'),
               glm::ivec2(selectedPosition.x, selectedPosition.y),
               selectedPiece,
               *selectedPiece->getChildren(), // Save the reflections of the moved piece
               targetSquare->getChildren()->empty() ? nullptr : targetSquare->getChildren()->front(),
               targetSquare->getChildren()->empty() ? std::vector<std::shared_ptr<Eng::Node>>() : *targetSquare->getChildren()->front()->getChildren() // Reflections of the captured piece
            };
            moveHistory.push_back(move);

            if (!targetSquare->getChildren()->empty()) {
               auto existingPiece = targetSquare->getChildren()->front();
               existingPiece->getChildren()->clear();
               targetSquare->getChildren()->clear();
               existingPiece->setParent(nullptr);
            }

            if (auto oldParent = selectedPiece->getParent()) {
               oldParent->getChildren()->clear();
            }

            targetSquare->addChild(selectedPiece);
            selectedPiece->setParent(targetSquare.get());

            selectedPiece = nullptr;
            isSelectionMode = true;
         }
      }
      updateSquareColors();
   });

   // Register the 'U' key for undoing a move
   callbackManager.registerKeyBinding('u', "Undo Move", [&](unsigned char key, int x, int y) {
      if (moveHistory.empty()) {
         return;
      }

      // Retrieve the last move
      Move lastMove = moveHistory.back();
      moveHistory.pop_back();
      redoHistory.push_back(lastMove); // Push to redo stack

      // Restore the previous state
      auto fromSquare = chessBoard[lastMove.fromPosition.y][lastMove.fromPosition.x];
      auto toSquare = chessBoard[lastMove.toPosition.y][lastMove.toPosition.x];

      if (auto oldParent = lastMove.movedPiece->getParent()) {
         oldParent->getChildren()->clear();
      }

      fromSquare->addChild(lastMove.movedPiece);
      lastMove.movedPiece->setParent(fromSquare.get());

      // Restore the reflections of the moved piece
      auto& movedChildren = *lastMove.movedPiece->getChildren();
      movedChildren.clear();
      movedChildren.insert(movedChildren.end(), lastMove.movedPieceChildren.begin(), lastMove.movedPieceChildren.end());

      if (lastMove.capturedPiece) {
         toSquare->addChild(lastMove.capturedPiece);
         lastMove.capturedPiece->setParent(toSquare.get());

         // Restore the reflections of the captured piece
         auto& capturedChildren = *lastMove.capturedPiece->getChildren();
         capturedChildren.clear();
         capturedChildren.insert(capturedChildren.end(), lastMove.capturedPieceChildren.begin(), lastMove.capturedPieceChildren.end());
      }

      updateSquareColors();
   });

   // Redo functionality
   callbackManager.registerKeyBinding('o', "Redo Move", [&](unsigned char key, int x, int y) {
       if (redoHistory.empty()) {
           return; // No moves to redo
       }

       // Retrieve the last undone move
       Move lastRedo = redoHistory.back();
       redoHistory.pop_back();
       moveHistory.push_back(lastRedo); // Push back to undo stack

       // Apply the move logic
       auto fromSquare = chessBoard[lastRedo.fromPosition.y][lastRedo.fromPosition.x];
       auto toSquare = chessBoard[lastRedo.toPosition.y][lastRedo.toPosition.x];

       // Clear any existing piece at the destination
       if (!toSquare->getChildren()->empty()) {
           auto existingPiece = toSquare->getChildren()->front();
           existingPiece->getChildren()->clear();
           toSquare->getChildren()->clear();
           existingPiece->setParent(nullptr);
       }

       // Move the piece from source to destination
       if (auto oldParent = lastRedo.movedPiece->getParent()) {
           oldParent->getChildren()->clear();
       }

       toSquare->addChild(lastRedo.movedPiece);
       lastRedo.movedPiece->setParent(toSquare.get());

       // Restore the moved piece's reflections
       auto& movedChildren = *lastRedo.movedPiece->getChildren();
       movedChildren.clear();
       movedChildren.insert(movedChildren.end(), lastRedo.movedPieceChildren.begin(), lastRedo.movedPieceChildren.end());

       updateSquareColors();
       });

   // Reset functionality
   callbackManager.registerKeyBinding('p', "Reset Board", [&](unsigned char key, int x, int y) {
       // Clear current board state
       for (auto& row : chessBoard) {
           for (auto& square : row) {
               if (!square->getChildren()->empty()) {
                   auto piece = square->getChildren()->front();
                   piece->getChildren()->clear();
                   square->getChildren()->clear();
               }
           }
       }

       // Restore the initial state
       for (const auto& move : initialState) {
           auto square = chessBoard[move.fromPosition.y][move.fromPosition.x];
           square->addChild(move.movedPiece);
           move.movedPiece->setParent(square.get());

           auto& pieceChildren = *move.movedPiece->getChildren();
           pieceChildren.clear();
           pieceChildren.insert(pieceChildren.end(), move.movedPieceChildren.begin(), move.movedPieceChildren.end());
       }

       // Clear undo and redo histories
       moveHistory.clear();
       redoHistory.clear();

       // Update visuals
       updateSquareColors();
       });

   // Capture the initial state of the board
   if (initialState.empty()) { // Only save once
       for (int row = 0; row < 8; ++row) {
           for (int col = 0; col < 8; ++col) {
               auto square = chessBoard[row][col];
               if (!square->getChildren()->empty()) {
                   auto piece = square->getChildren()->front();
                   Move initialMove = {
                       glm::ivec2(col, row),
                       glm::ivec2(col, row),
                       piece,
                       *piece->getChildren(),
                       nullptr,
                       {} // No captured pieces in the initial state
                   };
                   initialState.push_back(initialMove);
               }
           }
       }
   }

   // Initial color update
   updateSquareColors();
}


void setupLightControls(Eng::Base &eng) {
   auto &callbackManager = Eng::CallbackManager::getInstance();
   auto root = eng.getRootNode();

   // Find and store the reference to the light
   static std::shared_ptr<Eng::PointLight> plantLight;
   std::function<void(const std::shared_ptr<Eng::Node> &)> findLight =
         [&findLight](const std::shared_ptr<Eng::Node> &node) {
      if (!node) return;
      if (node->getName() == "PlantLight") {
         plantLight = std::dynamic_pointer_cast<Eng::PointLight>(node);
      }
      for (auto &child: *node->getChildren()) {
         findLight(child);
      }
   };

   findLight(root);

   if (!plantLight) {
      std::cout << "PlantLight not found in scene!" << std::endl;
      return;
   }

   constexpr float moveSpeed = 0.4f;

   //register light controls with their descriptions
   callbackManager.registerKeyBinding('y', "Move light forward",
                                      [=](unsigned char key, int x, int y) {
                                         if (!plantLight) return;
                                         glm::mat4 currentMatrix = plantLight->getLocalMatrix();
                                         plantLight->setLocalMatrix(
                                            glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -moveSpeed)) *
                                            currentMatrix
                                         );
                                      });

   callbackManager.registerKeyBinding('h', "Move light backward",
                                      [=](unsigned char key, int x, int y) {
                                         if (!plantLight) return;
                                         glm::mat4 currentMatrix = plantLight->getLocalMatrix();
                                         plantLight->setLocalMatrix(
                                            glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, moveSpeed)) *
                                            currentMatrix
                                         );
                                      });

   callbackManager.registerKeyBinding('g', "Move light left",
                                      [=](unsigned char key, int x, int y) {
                                         if (!plantLight) return;
                                         glm::mat4 currentMatrix = plantLight->getLocalMatrix();
                                         plantLight->setLocalMatrix(
                                            glm::translate(glm::mat4(1.0f), glm::vec3(-moveSpeed, 0.0f, 0.0f)) *
                                            currentMatrix
                                         );
                                      });

   callbackManager.registerKeyBinding('j', "Move light right",
                                      [=](unsigned char key, int x, int y) {
                                         if (!plantLight) return;
                                         glm::mat4 currentMatrix = plantLight->getLocalMatrix();
                                         plantLight->setLocalMatrix(
                                            glm::translate(glm::mat4(1.0f), glm::vec3(moveSpeed, 0.0f, 0.0f)) *
                                            currentMatrix
                                         );
                                      });
   // Randomize light color when 'R' is pressed
   callbackManager.registerKeyBinding('r', "Randomize light color",
                                      [=](unsigned char key, int x, int y) {
                                         if (!plantLight) return;

                                         // Random number generator for RGB values
                                         std::random_device rd;
                                         std::mt19937 gen(rd());
                                         std::uniform_real_distribution<float> dist(0.0f, 1.0f);

                                         // Generate random RGB values
                                         glm::vec3 randomColor(dist(gen), dist(gen), dist(gen));

                                         // Set the light color
                                         plantLight->setColor(randomColor);
                                      });
}


void setUpCameras(Eng::Base &eng) {
   static std::vector<std::shared_ptr<Eng::PerspectiveCamera> > cameras;
   auto &callbackManager = Eng::CallbackManager::getInstance();
   float initialAspect = eng.getWindowAspectRatio();

   // Camera 1
   auto camera1 = std::make_shared<Eng::PerspectiveCamera>(45.0f, initialAspect, 0.1f, 1000000.0f);
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
