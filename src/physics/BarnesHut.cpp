#include "physics/BarnesHut.h"
#include "core/Body.h"
#include <algorithm>
#include <vector>
#include <iostream>

namespace nbody {

// NOTE: Constants are now correctly sourced from BarnesHut.h

BarnesHutTree::BarnesHutTree() = default;

void BarnesHutTree::ReserveNodes(size_t expectedNodes) {
    // This is a placeholder for future optimization
    // Could implement a node pool here to reduce allocations
    (void)expectedNodes; // Suppress unused parameter warning
}

void BarnesHutTree::BuildTree(const std::vector<std::unique_ptr<Body>>& bodies) {
    if (bodies.empty()) {
        m_root.reset();
        return;
    }
    
    m_stats = TreeStats();
    
    // Get bounds efficiently
    glm::vec2 center;
    float size;
    CalculateBounds(bodies, center, size);
    
    // Debug output only in debug builds and less frequently to reduce console spam
    #ifdef _DEBUG
    static int frameCount = 0;
    frameCount++;
    if (frameCount % 300 == 0) { // Only print every 300 frames instead of 60
        std::cout << "Building Barnes-Hut tree for " << bodies.size() << " bodies" << std::endl;
        std::cout << "Tree bounds: center=(" << center.x << "," << center.y << "), size=" << size << std::endl;
    }
    #endif
    
    // Reset or create root node
    if (!m_root) {
        m_root = std::make_unique<QuadTreeNode>();
    }
    m_root->center = center;
    m_root->size = size;
    m_root->totalMass = 0.0f;
    m_root->centerOfMass = glm::vec2(0.0f);
    m_root->body = nullptr;
    m_root->isLeaf = true;
    // Clear children
    for (auto& child : m_root->children) {
        child.reset();
    }
    
    int bodiesInserted = 0;
    int bodiesOutsideBounds = 0;
    
    // Insert bodies into the tree
    for (const auto& body : bodies) {
        // Ensure body is within the root bounds before inserting
        if (m_root->Contains(body->GetPosition())) {
            InsertBody(m_root.get(), body.get());
            bodiesInserted++;
        } else {
            bodiesOutsideBounds++;
        }
    }
    
    // Print warnings only occasionally to avoid console spam
    #ifdef _DEBUG
    if (bodiesOutsideBounds > 0 && frameCount % 300 == 0) {
        std::cout << "Warning: " << bodiesOutsideBounds << " bodies outside tree bounds!" << std::endl;
    }
    #endif
    
    // Calculate center of mass for each node
    UpdateMassAndCenter(m_root.get());
    
    // Count nodes for stats (only occasionally to improve performance)
    #ifdef _DEBUG
    if (frameCount % 300 == 0) {
        CountNodes(m_root.get(), m_stats);
        std::cout << "Tree stats: " << m_stats.totalNodes << " nodes, " 
                  << m_stats.leafNodes << " leaves, max depth " << m_stats.maxDepth << std::endl;
    }
    #endif
}

glm::vec2 BarnesHutTree::CalculateForce(const Body& body, float theta, float G) const {
    if (!m_root) {
        return glm::vec2(0.0f);
    }
    
    // Don't reset force calculation counter here - let it accumulate for all bodies
    glm::vec2 force = CalculateForceIterative(body, theta, G);
    
    // Debug output for first few bodies (only in debug builds)
    #ifdef _DEBUG
    static int debugCount = 0;
    if (debugCount < 3) {
        int initialCount = m_stats.forceCalculations; // Only compute when needed for debug
        std::cout << "Force on body at (" << body.GetPosition().x << "," << body.GetPosition().y 
                  << ") = (" << force.x << "," << force.y << "), added " 
                  << (m_stats.forceCalculations - initialCount) << " calculations" << std::endl;
        debugCount++;
    }
    #endif
    
    return force;
}

void BarnesHutTree::InsertBody(QuadTreeNode* node, Body* body) {
    // Iterative implementation to avoid recursion overhead
    QuadTreeNode* current = node;
    
    while (true) {
        // Safety check: ensure body is within node bounds
        if (!current->Contains(body->GetPosition())) {
            return;
        }
        
        if (current->isLeaf) {
            if (current->body == nullptr) {
                // Empty leaf, place the body here.
                current->body = body;
                return;
            }
            
            // Leaf is occupied, so we must subdivide.
            // Edge case: If existing body and new body are at the same position,
            // handle gracefully by placing in same node (bodies very close together)
            glm::vec2 delta = current->body->GetPosition() - body->GetPosition();
            if (glm::dot(delta, delta) < 1e-12f) {
                return; // Bodies are essentially at same position
            }

            Body* existingBody = current->body;
            current->body = nullptr;
            current->isLeaf = false; // Mark as internal node
            Subdivide(current);
            
            // Insert existing body into correct child quadrant
            int existingQuadrant = current->GetQuadrant(existingBody->GetPosition());
            if (current->children[existingQuadrant]) {
                InsertBody(current->children[existingQuadrant].get(), existingBody);
            }
            
            // Continue loop to insert new body
            int newQuadrant = current->GetQuadrant(body->GetPosition());
            if (current->children[newQuadrant]) {
                current = current->children[newQuadrant].get();
                continue;
            } else {
                return;
            }

        } else { // Node is internal
            // Move to the correct child quadrant
            int quadrant = current->GetQuadrant(body->GetPosition());
            if (current->children[quadrant]) {
                current = current->children[quadrant].get();
                continue;
            } else {
                return;
            }
        }
    }
}

void BarnesHutTree::Subdivide(QuadTreeNode* node) {
    float childSize = node->size * 0.5f;
    for (int i = 0; i < 4; ++i) {
        node->children[i] = std::make_unique<QuadTreeNode>();
        node->children[i]->center = node->GetChildCenter(i);
        node->children[i]->size = childSize;
    }
}

void BarnesHutTree::UpdateMassAndCenter(QuadTreeNode* node) {
    if (!node) return;
    
    if (node->isLeaf) {
        if (node->body) {
            node->totalMass = node->body->GetMass();
            node->centerOfMass = node->body->GetPosition();
        } else {
            node->totalMass = 0.0f;
            node->centerOfMass = glm::vec2(0.0f);
        }
    } else {
        // CORRECTED: Calculate center of mass for an internal node.
        // Sum weighted positions and total mass, then perform a single division.
        node->totalMass = 0.0f;
        glm::vec2 weightedPositionSum(0.0f);

        for (int i = 0; i < 4; ++i) {
            if (node->children[i]) {
                UpdateMassAndCenter(node->children[i].get());
                
                float childMass = node->children[i]->totalMass;
                if (childMass > 0.0f) {
                    node->totalMass += childMass;
                    weightedPositionSum += node->children[i]->centerOfMass * childMass;
                }
            }
        }

        if (node->totalMass > 1e-9f) {
            node->centerOfMass = weightedPositionSum / node->totalMass;
        } else {
            node->centerOfMass = node->center; // Default to geometric center if massless
        }
    }
}

glm::vec2 BarnesHutTree::CalculateForceIterative(const Body& body, float theta, float G) const {
    glm::vec2 totalForce(0.0f);
    if (!m_root || m_root->totalMass <= 0.0f) {
        return totalForce;
    }

    std::vector<const QuadTreeNode*> stack;
    stack.push_back(m_root.get());
    
    int nodeVisits = 0;
    int nodesTotalMass = 0;

    while (!stack.empty()) {
        const QuadTreeNode* node = stack.back();
        stack.pop_back();
        nodeVisits++;

        if (!node || node->totalMass <= 0.0f) {
            continue;
        }

        nodesTotalMass++;

        // Vector FROM body's position TO node's center of mass
        // This is the direction the force should pull the body
        glm::vec2 bodyToNode = node->centerOfMass - body.GetPosition();
        float distanceSq = glm::dot(bodyToNode, bodyToNode);
        
        // Skip self-interactions
        if (node->isLeaf && node->body == &body) {
            continue;
        }
        
        // Barnes-Hut approximation: More aggressive approximation for better performance
        // Standard is s/d < theta, which equals s²/d² < theta²
        // Higher theta (0.5-1.0) makes simulation faster but less accurate
        float distance = std::sqrt(distanceSq);
        float sizeToDistRatio = node->size / (distance + 1e-10f);
        
        // If s/d is small enough, use approximation (Rust implementation uses theta=1.0)
        if (sizeToDistRatio < theta) {
            // Node is far enough away, use approximation
            if (distanceSq <= 0.0f) continue; // Avoid division by zero
            
            // Use consistent gravitational force calculation with optimized computation
            // Calculate force magnitude: F = G * mass / r² where r² includes softening
            float softenedDistSq = distanceSq + SOFTENING_LENGTH * SOFTENING_LENGTH;
            float forceMagnitude = G * node->totalMass / softenedDistSq;
            
            // Optimize: use existing distance calculation to avoid redundant sqrt
            totalForce += forceMagnitude * bodyToNode / distance;
            
            m_stats.forceCalculations++;
        } 
        else if (node->isLeaf) {
            // Too close for approximation, but it's a leaf node
            // Skip empty leaves
            if (!node->body) continue;
            
            if (distanceSq <= 0.0f) continue; // Avoid division by zero
            
            // Use consistent gravitational force calculation
            float softenedDistSq = distanceSq + SOFTENING_LENGTH * SOFTENING_LENGTH;
            float forceMagnitude = G * node->totalMass / softenedDistSq;
            
            // Optimize: reuse distance calculation
            totalForce += forceMagnitude * bodyToNode / distance;
            
            m_stats.forceCalculations++;
        } 
        else {
            // Internal node that's too close for approximation, descend to children
            // Push children in reverse order for better cache locality (closer nodes first)
            for (int i = 3; i >= 0; --i) {
                if (node->children[i]) {
                    stack.push_back(node->children[i].get());
                }
            }
        }
    }
    
    // Debug output for first few bodies (only in debug builds)
    #ifdef _DEBUG
    static int bodyCount = 0;
    if (bodyCount < 3) {
        std::cout << "Body " << bodyCount << ": visits=" << nodeVisits 
                  << ", nodes with mass=" << nodesTotalMass
                  << ", computations=" << m_stats.forceCalculations
                  << ", force=(" << totalForce.x << "," << totalForce.y << ")" << std::endl;
        bodyCount++;
    }
    #endif
    
    return totalForce;
}


void BarnesHutTree::CalculateBounds(const std::vector<std::unique_ptr<Body>>& bodies, glm::vec2& center, float& size) const {
    if (bodies.empty()) {
        center = glm::vec2(0.0f);
        size = 1.0f;
        return;
    }
    
    // Optimized bounds calculation - use first body as initial values
    const glm::vec2& firstPos = bodies[0]->GetPosition();
    glm::vec2 minPos(firstPos);
    glm::vec2 maxPos(firstPos);
    
    // Find the actual bounds of all bodies (start from index 1)
    for (size_t i = 1; i < bodies.size(); ++i) {
        const glm::vec2& pos = bodies[i]->GetPosition();
        minPos.x = std::min(minPos.x, pos.x);
        minPos.y = std::min(minPos.y, pos.y);
        maxPos.x = std::max(maxPos.x, pos.x);
        maxPos.y = std::max(maxPos.y, pos.y);
    }
    
    // Calculate center and size
    center = (minPos + maxPos) * 0.5f;
    
    // Ensure we have some minimum size even if all bodies are at the same position
    float sizeX = std::max(maxPos.x - minPos.x, 0.1f);
    float sizeY = std::max(maxPos.y - minPos.y, 0.1f);
    size = std::max(sizeX, sizeY);
    
    // Add smaller padding (20%) to ensure bodies don't fall outside the bounds
    // but keep the tree size reasonable for better approximation
    size *= 1.2f;
    
    // Ensure minimum size to avoid numerical issues
    size = std::max(size, MIN_NODE_SIZE);
    
    #ifdef _DEBUG
    std::cout << "Bounds: min=(" << minPos.x << "," << minPos.y 
              << "), max=(" << maxPos.x << "," << maxPos.y 
              << "), center=(" << center.x << "," << center.y 
              << "), size=" << size << std::endl;
    #endif
}

void BarnesHutTree::CountNodes(const QuadTreeNode* node, TreeStats& stats, int depth) const {
    if (!node) return;
    
    stats.totalNodes++;
    stats.maxDepth = std::max(stats.maxDepth, depth);
    
    if (node->isLeaf) {
        if (node->body) {
            stats.leafNodes++;
        }
    } else {
        for (int i = 0; i < 4; ++i) {
            if (node->children[i]) {
                CountNodes(node->children[i].get(), stats, depth + 1);
            }
        }
    }
}

} // namespace nbody