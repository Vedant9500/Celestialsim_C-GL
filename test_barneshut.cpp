#include "physics/BarnesHut.h"
#include "core/Body.h"
#include <iostream>
#include <memory>
#include <vector>

int main() {
    // Create a simple test with a few bodies
    std::vector<std::unique_ptr<nbody::Body>> bodies;
    
    // Create 3 bodies in a line to test basic functionality
    bodies.emplace_back(std::make_unique<nbody::Body>(glm::vec2(-1.0f, 0.0f), glm::vec2(0.0f), 1.0f));
    bodies.emplace_back(std::make_unique<nbody::Body>(glm::vec2(0.0f, 0.0f), glm::vec2(0.0f), 1.0f));
    bodies.emplace_back(std::make_unique<nbody::Body>(glm::vec2(1.0f, 0.0f), glm::vec2(0.0f), 1.0f));
    
    std::cout << "Testing Barnes-Hut with 3 bodies:" << std::endl;
    for (size_t i = 0; i < bodies.size(); i++) {
        std::cout << "Body " << i << ": pos=(" << bodies[i]->GetPosition().x << "," << bodies[i]->GetPosition().y 
                  << "), mass=" << bodies[i]->GetMass() << std::endl;
    }
    
    // Build tree
    nbody::BarnesHutTree tree;
    tree.BuildTree(bodies);
    
    // Calculate forces
    const float G = 1.0f;
    const float theta = 0.25f;
    
    std::cout << "\nForce calculations:" << std::endl;
    for (size_t i = 0; i < bodies.size(); i++) {
        glm::vec2 force = tree.CalculateForce(*bodies[i], theta, G);
        std::cout << "Body " << i << " force: (" << force.x << "," << force.y << ")" << std::endl;
    }
    
    auto stats = tree.GetStats();
    std::cout << "\nTree stats: " << stats.totalNodes << " nodes, " 
              << stats.leafNodes << " leaves, " << stats.forceCalculations << " force calculations" << std::endl;
    
    return 0;
}
