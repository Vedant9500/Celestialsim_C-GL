#include "physics/BarnesHut.h"
#include "core/Body.h"
#include <algorithm>
#include <limits>
#include <iostream> // For debug output

namespace nbody {

// Constants
static constexpr float MIN_DISTANCE = 1e-6f;
static constexpr float SOFTENING_LENGTH = 0.1f;
static constexpr float MIN_NODE_SIZE = 1.0f;

BarnesHutTree::BarnesHutTree() = default;

void BarnesHutTree::BuildTree(const std::vector<std::unique_ptr<Body>>& bodies) {
    if (bodies.empty()) {
        m_root.reset();
        return;
    }
    
    // Reset statistics
    m_stats = TreeStats();
    
    // Calculate bounding box
    glm::vec2 center;
    float size;
    CalculateBounds(bodies, center, size);
    
    // Create root node
    m_root = std::make_unique<QuadTreeNode>();
    m_root->center = center;
    m_root->size = size;
    m_root->isLeaf = true;
    
    // Debug output
    std::cout << "Building Barnes-Hut tree for " << bodies.size() << " bodies" << std::endl;
    std::cout << "Root bounds: center(" << center.x << ", " << center.y << "), size=" << size << std::endl;
    
    // Insert all bodies
    for (const auto& body : bodies) {
        InsertBody(m_root.get(), body.get());
    }
    
    // Update mass and center of mass for all nodes
    UpdateMassAndCenter(m_root.get());
    
    // Count nodes for statistics
    CountNodes(m_root.get(), m_stats);
    
    std::cout << "Tree built: " << m_stats.totalNodes << " nodes, " << m_stats.leafNodes << " leaves, max depth " << m_stats.maxDepth << std::endl;
    std::cout << "Root total mass: " << m_root->totalMass << std::endl;
}

glm::vec2 BarnesHutTree::CalculateForce(const Body& body, float theta, float G) const {
    if (!m_root) {
        return glm::vec2(0.0f);
    }
    
    // Reset stats for this calculation
    m_stats.forceCalculations = 0;
    
    // Use REF-style iterative force calculation instead of recursive
    return CalculateForceIterative(body, theta, G);
}

void BarnesHutTree::InsertBody(QuadTreeNode* node, Body* body) {
    if (!node->Contains(body->GetPosition())) {
        return; // Body is outside this node
    }
    
    if (node->isLeaf) {
        if (!node->body) {
            // Empty leaf - insert body here
            node->body = body;
        } else {
            // Leaf already contains a body - subdivide
            Body* existingBody = node->body;
            node->body = nullptr;
            node->isLeaf = false;
            
            Subdivide(node);
            
            // Re-insert both bodies
            InsertBody(node, existingBody);
            InsertBody(node, body);
        }
    } else {
        // Internal node - insert into appropriate child
        int quadrant = node->GetQuadrant(body->GetPosition());
        InsertBody(node->children[quadrant].get(), body);
    }
}

void BarnesHutTree::Subdivide(QuadTreeNode* node) {
    float childSize = node->size * 0.5f;
    
    for (int i = 0; i < 4; ++i) {
        node->children[i] = std::make_unique<QuadTreeNode>();
        node->children[i]->center = node->GetChildCenter(i);
        node->children[i]->size = childSize;
        node->children[i]->isLeaf = true;
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
        node->totalMass = 0.0f;
        node->centerOfMass = glm::vec2(0.0f);
        
        for (int i = 0; i < 4; ++i) {
            if (node->children[i]) {
                UpdateMassAndCenter(node->children[i].get());
                
                float childMass = node->children[i]->totalMass;
                if (childMass > 0.0f) {
                    glm::vec2 weightedPos = node->children[i]->centerOfMass * childMass;
                    node->centerOfMass = (node->centerOfMass * node->totalMass + weightedPos) / (node->totalMass + childMass);
                    node->totalMass += childMass;
                }
            }
        }
    }
}

glm::vec2 BarnesHutTree::CalculateForceRecursive(const QuadTreeNode* node, const Body& body, float theta, float G) const {
    if (!node || node->totalMass <= 0.0f) {
        return glm::vec2(0.0f);
    }
    
    // Calculate vector from body to node center of mass
    glm::vec2 r = node->centerOfMass - body.GetPosition();
    float distanceSq = glm::dot(r, r);  // Squared distance
    
    // Skip if too close (likely self-interaction)
    if (distanceSq < MIN_DISTANCE * MIN_DISTANCE) {
        return glm::vec2(0.0f);
    }
    
    // REF-style approximation criteria: s² < theta * d²
    float sizeSq = node->size * node->size;
    
    // If this is a leaf node OR the node is far enough (satisfies approximation criteria)
    if (node->isLeaf || sizeSq < (theta * distanceSq)) {
        // For leaf nodes, ensure we're not calculating force on self
        if (node->isLeaf && node->body == &body) {
            return glm::vec2(0.0f);
        }
        
        // Calculate gravitational force using REF formula
        float softeningSq = SOFTENING_LENGTH * SOFTENING_LENGTH;
        float effectiveDistSq = distanceSq + softeningSq;
        float invDist = 1.0f / std::pow(effectiveDistSq, 1.5f);
        
        // Force = (G * mass) * r * invDist (where r is the vector, not normalized)
        float forceMagnitude = G * node->totalMass * invDist;
        
        // Apply force capping for stability
        static constexpr float MAX_FORCE = 10000.0f;
        if (forceMagnitude > MAX_FORCE) {
            forceMagnitude = MAX_FORCE;
        }
        
        glm::vec2 force = r * forceMagnitude;
        
        m_stats.forceCalculations++;
        return force;
    } else {
        // Node is too close - must recurse into children
        glm::vec2 totalForce(0.0f);
        
        for (int i = 0; i < 4; ++i) {
            if (node->children[i] && node->children[i]->totalMass > 0.0f) {
                totalForce += CalculateForceRecursive(node->children[i].get(), body, theta, G);
            }
        }
        
        return totalForce;
    }
}

// REF-inspired iterative force calculation - more stable than recursive
glm::vec2 BarnesHutTree::CalculateForceIterative(const Body& body, float theta, float G) const {
    glm::vec2 totalForce(0.0f);
    std::vector<const QuadTreeNode*> stack;
    
    if (!m_root || m_root->totalMass <= 0.0f) return totalForce;
    
    stack.push_back(m_root.get());
    
    while (!stack.empty()) {
        const QuadTreeNode* node = stack.back();
        stack.pop_back();
        
        if (!node || node->totalMass <= 0.0f) continue;
        
        // Calculate distance to node's center of mass
        glm::vec2 r = node->centerOfMass - body.GetPosition();
        float distance = glm::length(r);
        
        if (distance < MIN_DISTANCE) continue;
        
        // REF-style approximation criteria: s² < theta * d²
        // Note: Our 'size' is half-size (radius), REF uses full size
        float fullSize = node->size * 2.0f;  // Convert to full size like REF
        float sizeSq = fullSize * fullSize;
        float distanceSq = distance * distance;
        
        // REF uses theta²: if theta=0.5, they compare with 0.25*d²
        float thetaSq = theta * theta;
        
        // If node is leaf OR satisfies approximation criteria
        if (node->isLeaf || sizeSq < (thetaSq * distanceSq)) {
            // Check for self-interaction in leaf nodes
            if (node->isLeaf && node->body == &body) {
                continue; // Skip self-interaction
            }
            
            // Calculate force using REF formula
            float softeningSq = SOFTENING_LENGTH * SOFTENING_LENGTH;
            float effectiveDistSq = distanceSq + softeningSq;
            float invDist = 1.0f / std::pow(effectiveDistSq, 1.5f);
            
            glm::vec2 force = r * (G * node->totalMass * invDist);
            
            // Cap force magnitude to prevent instability
            float forceMagnitude = glm::length(force);
            static constexpr float MAX_FORCE = 10000.0f;
            if (forceMagnitude > MAX_FORCE) {
                force = glm::normalize(force) * MAX_FORCE;
            }
            
            totalForce += force;
            m_stats.forceCalculations++;
        } else {
            // Node is too close - add children to stack
            for (int i = 0; i < 4; ++i) {
                if (node->children[i] && node->children[i]->totalMass > 0.0f) {
                    stack.push_back(node->children[i].get());
                }
            }
        }
    }
    
    return totalForce;
}

void BarnesHutTree::CalculateBounds(const std::vector<std::unique_ptr<Body>>& bodies, glm::vec2& center, float& size) const {
    if (bodies.empty()) {
        center = glm::vec2(0.0f);
        size = 1.0f;
        return;
    }
    
    // Find bounding box
    glm::vec2 minPos = bodies[0]->GetPosition();
    glm::vec2 maxPos = bodies[0]->GetPosition();
    
    for (const auto& body : bodies) {
        glm::vec2 pos = body->GetPosition();
        minPos.x = std::min(minPos.x, pos.x);
        minPos.y = std::min(minPos.y, pos.y);
        maxPos.x = std::max(maxPos.x, pos.x);
        maxPos.y = std::max(maxPos.y, pos.y);
    }
    
    // Calculate center and size
    center = (minPos + maxPos) * 0.5f;
    glm::vec2 extent = maxPos - minPos;
    size = std::max(extent.x, extent.y);
    
    // Add some padding to ensure all bodies fit
    size *= 1.1f;
    
    // Ensure minimum size
    static constexpr float LOCAL_MIN_NODE_SIZE = 1.0f;
    size = std::max(size, LOCAL_MIN_NODE_SIZE);
}

void BarnesHutTree::CountNodes(const QuadTreeNode* node, TreeStats& stats, int depth) const {
    if (!node) return;
    
    stats.totalNodes++;
    stats.maxDepth = std::max(stats.maxDepth, depth);
    
    if (node->isLeaf) {
        stats.leafNodes++;
    } else {
        for (int i = 0; i < 4; ++i) {
            if (node->children[i]) {
                CountNodes(node->children[i].get(), stats, depth + 1);
            }
        }
    }
}

} // namespace nbody
