#include "../Engine.h"

/**
 * @brief Tests the initialization of CallbackManager.
 */
void Eng::testCallbackManagerInitialization() {
    auto& manager = Eng::CallbackManager::getInstance();

    // First call to initialize should return true
    assert(manager.initialize() == true && "Initialization failed on the first call!");

    // Subsequent calls to initialize should return false
    assert(manager.initialize() == false && "Initialization should return false on subsequent calls!");

    // Ensure the manager is in a valid state
    assert(!manager.isHelpMenuVisible() && "Help menu visibility state is incorrect!");

    std::cout << "testCallbackManagerInitialization Passed!" << std::endl;
}

/**
 * @brief Tests the registration of key bindings in CallbackManager.
 */
void Eng::testKeyBindingRegistration() {
    auto& manager = Eng::CallbackManager::getInstance();

    manager.registerKeyBinding('t', "Test Key", [](unsigned char key, int x, int y) {
        std::cout << "Key T pressed at (" << x << ", " << y << ")" << std::endl;
        });

    // Validate the key binding
    assert(manager.getKeyBindings().count('t') == 1 && "Key 't' was not registered correctly!");
    assert(manager.getKeyBindings().at('t').desc == "Test Key" && "Key description is incorrect!");

    std::cout << "testKeyBindingRegistration Passed!" << std::endl;
}

/**
 * @brief Tests default key bindings and ensures they work as expected.
 */
void Eng::testDefaultKeyBindings() {
    auto& manager = Eng::CallbackManager::getInstance();

    // Check default key bindings
    assert(manager.getKeyBindings().count('x') == 1 && "Default binding 'x' missing!");
    assert(manager.getKeyBindings().count('f') == 1 && "Default binding 'f' missing!");
    assert(manager.getKeyBindings().count(27) == 1 && "Default binding 'ESC' missing!");

    std::cout << "testDefaultKeyBindings Passed!" << std::endl;
}

/**
 * @brief Tests the execution order of callbacks for the same key.
 */
void Eng::testCallbackExecutionOrder() {
    auto& manager = Eng::CallbackManager::getInstance();

    // Register multiple callbacks for the same key
    std::vector<std::string> executionOrder;

    manager.registerKeyBinding('o', "First Callback", [&executionOrder](unsigned char key, int x, int y) {
        executionOrder.push_back("First");
        });
    manager.registerKeyBinding('o', "Second Callback", [&executionOrder](unsigned char key, int x, int y) {
        executionOrder.push_back("Second");
        });

    // Simulate key press
    manager.getKeyBindings().at('o').func('o', 0, 0);

    // Validate execution order
    assert(executionOrder.size() == 1 && "Callback was executed more than once!");
    assert(executionOrder[0] == "Second" && "Latest callback was not executed!");

    std::cout << "testCallbackExecutionOrder Passed!" << std::endl;
}