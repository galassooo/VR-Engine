#include "engine.h"

Eng::MultipassRenderer& Eng::MultipassRenderer::getInstance() {
    static MultipassRenderer instance;
    return instance;
}

void Eng::MultipassRenderer::render(Eng::List& renderList) {
	// Implement the rendering logic here
	// This is a placeholder for the actual rendering code
	std::cout << "Rendering with MultipassRenderer" << std::endl;
}