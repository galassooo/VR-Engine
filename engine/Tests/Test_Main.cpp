#include "../engine.h"
#include <GL/freeglut.h>

int main() {
    try {
        // Suppress stderr during FreeGLUT initialization
        FILE* original_stderr = freopen("/dev/null", "w", stderr); // Redirect stderr to /dev/null

        // Initialize FreeGLUT
        int argc = 1;
        char* argv[1] = { (char*)"tests" };
        glutInit(&argc, argv);

        // Restore stderr
        if (original_stderr) {
            freopen("/dev/tty", "w", stderr); // Restore stderr to terminal
        }

        // Camera Tests
        Eng::testOrthographicCamera();
        Eng::testPerspectiveCamera();

        // Light Tests
        Eng::testDirectionalLight();
        Eng::testPointLight();
        Eng::testSpotLight();

        // List Tests
        Eng::testListOrdering();
        Eng::testListNodeManagement();
        Eng::testListElement();

        // Mesh Tests
        Eng::testMeshVerticesAndIndices();
        Eng::testMeshMaterial();

        // CallManager Tests
        Eng::testCallbackManagerInitialization();
        Eng::testKeyBindingRegistration();
        Eng::testDefaultKeyBindings();
        Eng::testCallbackExecutionOrder();

        std::cout << "All Tests Passed!" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Test Failed: " << e.what() << std::endl;
        return 1; // Non-zero return code indicates failure
    }
    catch (...) {
        std::cerr << "Unknown error occurred during testing!" << std::endl;
        return 1; // Non-zero return code indicates failure
    }

    return 0;
}