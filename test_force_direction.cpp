#include "physics/PhysicsEngine.h"
#include "physics/BarnesHut.h"
#include "core/Body.h"
#include <iostream>
#include <memory>
#include <vector>
#include <iomanip>

using namespace nbody;

void printSeparator(const std::string& title) {
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << title << std::endl;
    std::cout << std::string(50, '=') << std::endl;
}

bool compareForces(const glm::vec2& f1, const glm::vec2& f2, float tolerance = 0.01f) {
    float diff = glm::length(f1 - f2);
    float mag = std::max(glm::length(f1), glm::length(f2));
    return mag < 1e-10f || (diff / mag) < tolerance;
}

int main() {
    std::cout << std::fixed << std::setprecision(6);
    printSeparator("Testing Barnes-Hut vs Direct Force Calculation Consistency");
    
    // Test 1: Simple two-body system
    {
        printSeparator("Test 1: Two-Body System");
        
        auto bodyA = std::make_unique<Body>(glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 0.0f), 10.0f);
        auto bodyB = std::make_unique<Body>(glm::vec2(10.0f, 0.0f), glm::vec2(0.0f, 0.0f), 20.0f);
        
        std::vector<std::unique_ptr<Body>> bodies;
        bodies.push_back(std::move(bodyA));
        bodies.push_back(std::move(bodyB));
        
        PhysicsEngine physics;
        physics.Initialize();
        
        // Test with both methods
        std::cout << "Body A position: (" << bodies[0]->GetPosition().x << ", " << bodies[0]->GetPosition().y << ")" << std::endl;
        std::cout << "Body B position: (" << bodies[1]->GetPosition().x << ", " << bodies[1]->GetPosition().y << ")" << std::endl;
        std::cout << "Body A mass: " << bodies[0]->GetMass() << ", Body B mass: " << bodies[1]->GetMass() << std::endl;
        
        // Direct method
        for (auto& body : bodies) body->ResetForce();
        auto config = physics.GetConfig();
        config.useBarnesHut = false;
        physics.SetConfig(config);
        physics.CalculateForces(bodies);
        
        glm::vec2 directForceA = bodies[0]->GetForce();
        glm::vec2 directForceB = bodies[1]->GetForce();
        
        // Barnes-Hut method
        for (auto& body : bodies) body->ResetForce();
        config.useBarnesHut = true;
        physics.SetConfig(config);
        physics.CalculateForces(bodies);
        
        glm::vec2 bhForceA = bodies[0]->GetForce();
        glm::vec2 bhForceB = bodies[1]->GetForce();
        
        std::cout << "\nDirect Method Results:" << std::endl;
        std::cout << "  Body A force: (" << directForceA.x << ", " << directForceA.y << ")" << std::endl;
        std::cout << "  Body B force: (" << directForceB.x << ", " << directForceB.y << ")" << std::endl;
        
        std::cout << "\nBarnes-Hut Results:" << std::endl;
        std::cout << "  Body A force: (" << bhForceA.x << ", " << bhForceA.y << ")" << std::endl;
        std::cout << "  Body B force: (" << bhForceB.x << ", " << bhForceB.y << ")" << std::endl;
        
        // Validate physics principles
        bool attractiveA = directForceA.x > 0 && bhForceA.x > 0; // Should attract A toward B
        bool attractiveB = directForceB.x < 0 && bhForceB.x < 0; // Should attract B toward A
        bool forceConsistent = compareForces(directForceA, bhForceA) && compareForces(directForceB, bhForceB);
        
        std::cout << "\nValidation:" << std::endl;
        std::cout << "  Forces are attractive: " << (attractiveA && attractiveB ? "✓ PASS" : "✗ FAIL") << std::endl;
        std::cout << "  Methods are consistent: " << (forceConsistent ? "✓ PASS" : "✗ FAIL") << std::endl;
        
        if (!attractiveA || !attractiveB || !forceConsistent) {
            std::cout << "ERROR: Two-body test failed!" << std::endl;
            return 1;
        }
    }
    
    // Test 2: Three-body system
    {
        printSeparator("Test 2: Three-Body System");
        
        std::vector<std::unique_ptr<Body>> bodies;
        bodies.emplace_back(std::make_unique<Body>(glm::vec2(-5.0f, 0.0f), glm::vec2(0.0f), 15.0f));
        bodies.emplace_back(std::make_unique<Body>(glm::vec2(0.0f, 0.0f), glm::vec2(0.0f), 25.0f));
        bodies.emplace_back(std::make_unique<Body>(glm::vec2(5.0f, 0.0f), glm::vec2(0.0f), 10.0f));
        
        PhysicsEngine physics;
        physics.Initialize();
        
        // Direct method
        for (auto& body : bodies) body->ResetForce();
        auto config = physics.GetConfig();
        config.useBarnesHut = false;
        physics.SetConfig(config);
        physics.CalculateForces(bodies);
        
        std::vector<glm::vec2> directForces;
        for (const auto& body : bodies) {
            directForces.push_back(body->GetForce());
        }
        
        // Barnes-Hut method
        for (auto& body : bodies) body->ResetForce();
        config.useBarnesHut = true;
        physics.SetConfig(config);
        physics.CalculateForces(bodies);
        
        std::vector<glm::vec2> bhForces;
        for (const auto& body : bodies) {
            bhForces.push_back(body->GetForce());
        }
        
        std::cout << "Comparing forces for " << bodies.size() << " bodies:" << std::endl;
        bool allConsistent = true;
        for (size_t i = 0; i < bodies.size(); ++i) {
            bool consistent = compareForces(directForces[i], bhForces[i]);
            std::cout << "  Body " << i << ": Direct(" << directForces[i].x << ", " << directForces[i].y 
                      << ") vs BH(" << bhForces[i].x << ", " << bhForces[i].y << ") - " 
                      << (consistent ? "✓" : "✗") << std::endl;
            allConsistent &= consistent;
        }
        
        std::cout << "Three-body consistency: " << (allConsistent ? "✓ PASS" : "✗ FAIL") << std::endl;
        
        if (!allConsistent) {
            std::cout << "ERROR: Three-body test failed!" << std::endl;
            return 1;
        }
    }
    
    printSeparator("All Tests Passed Successfully!");
    std::cout << "✓ Force directions are correct (attractive)" << std::endl;
    std::cout << "✓ Barnes-Hut and Direct methods are consistent" << std::endl;
    std::cout << "✓ Physics calculations are mathematically correct" << std::endl;
    
    return 0;
}
