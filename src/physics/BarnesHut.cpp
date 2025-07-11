#include "physics/BarnesHut.h"
#include "core/Body.h"
#include <algorithm>
#include <limits>

namespace nbody {

// Constants
static constexpr float MIN_DISTANCE = 1e-6f;
static constexpr float SOFTENING_LENGTH = 0.1f;

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
    
    // Insert all bodies
    for (const auto& body : bodies) {
        InsertBody(m_root.get(), body.get());
    }
    
    // Update mass and center of mass for all nodes
    UpdateMassAndCenter(m_root.get());
    
    // Count nodes for statistics
    CountNodes(m_root.get(), m_stats);
}

glm::vec2 BarnesHutTree::CalculateForce(const Body& body, float theta, float G) const {
    if (!m_root) {
        return glm::vec2(0.0f);
    }
    
    return CalculateForceRecursive(m_root.get(), body, theta, G);
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
    if (!node || node->totalMass == 0.0f) {
        return glm::vec2(0.0f);
    }
    
    glm::vec2 r = node->centerOfMass - body.GetPosition();
    float distance = glm::length(r);
    
    if (distance < MIN_DISTANCE) {
        return glm::vec2(0.0f);
    }
    
    // Check if we can use this node directly (far enough away)
    if (node->isLeaf || (node->size / distance) < theta) {
        // Use this node as a single body
        float distanceSq = distance * distance + SOFTENING_LENGTH * SOFTENING_LENGTH;
        float forceMagnitude = G * body.GetMass() * node->totalMass / distanceSq;
        
        glm::vec2 forceDirection = r / distance;
        
        m_stats.forceCalculations++;
        return forceMagnitude * forceDirection;
    } else {
        // Node is too close - recurse into children
        glm::vec2 totalForce(0.0f);
        
        for (int i = 0; i < 4; ++i) {
            if (node->children[i]) {
                totalForce += CalculateForceRecursive(node->children[i].get(), body, theta, G);
            }
        }
        
        return totalForce;
    }
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
    size = std::max(size, MIN_NODE_SIZE);
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
