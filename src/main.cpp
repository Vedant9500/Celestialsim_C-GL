#include "core/Application.h"
#include <iostream>
#include <cstdlib>

int main() {
    try {
        nbody::Application app;
        
        if (!app.Initialize()) {
            std::cerr << "Failed to initialize application" << std::endl;
            return EXIT_FAILURE;
        }
        
        app.Run();
        app.Shutdown();
        
        return EXIT_SUCCESS;
    }
    catch (const std::exception& e) {
        std::cerr << "Application error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (...) {
        std::cerr << "Unknown application error" << std::endl;
        return EXIT_FAILURE;
    }
}
