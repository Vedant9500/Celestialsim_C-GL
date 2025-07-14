
#include "physics/BarnesHut.h"
#include "core/Body.h"
#include <algorithm>
#include <vector>
#include <iostream>

namespace nbody {

// NOTE: Constants are now correctly sourced from BarnesHut.h

BarnesHutTree::BarnesHutTree() = default;

void BarnesHutTree::BuildTree(const std::vector<std::unique_ptr<Body>>& bodies) {
    if (bodies.empty()) {
        m_root.reset();
        return;
    }
    
    m_stats = TreeStats();
    
    glm::vec2 center;
    float size;
    CalculateBounds(bodies, center, size);
    
    m_root = std::make_unique<QuadTreeNode>();
    m_root->center = center;
    m_root->size = size;
    
    for (const auto& body : bodies) {
        // Ensure body is within the root bounds before inserting
        if (m_root->Contains(body->GetPosition())) {
            InsertBody(m_root.get(), body.get());
        }
    }
    
    UpdateMassAndCenter(m_root.get());
    
    CountNodes(m_root.get(), m_stats);
}

glm::vec2 BarnesHutTree::CalculateForce(const Body& body, float theta, float G) const {
    if (!m_root) {
        return glm::vec2(0.0f);
    }
    m_stats.forceCalculations = 0;
    return CalculateForceIterative(body, theta, G);
}

void BarnesHutTree::InsertBody(QuadTreeNode* node, Body* body) {
    // Safety check: ensure body is within node bounds
    if (!node->Contains(body->GetPosition())) {
        return;
    }
    
    if (node->isLeaf) {
        if (node->body == nullptr) {
            // Empty leaf, place the body here.
            node->body = body;
            return;
        }
        
        // Leaf is occupied, so we must subdivide.
        // Edge case: If existing body and new body are at the same position,
        // it can cause infinite recursion. We handle this by just returning.
        glm::vec2 delta = node->body->GetPosition() - body->GetPosition();
        if (glm::dot(delta, delta) < 1e-12f) {
            return;
        }

        Body* existingBody = node->body;
        node->body = nullptr;
        node->isLeaf = false; // Mark as internal node
        Subdivide(node);
        
        // Re-insert both bodies into the correct new child quadrants.
        int existingQuadrant = node->GetQuadrant(existingBody->GetPosition());
        InsertBody(node->children[existingQuadrant].get(), existingBody);
        
        int newQuadrant = node->GetQuadrant(body->GetPosition());
        InsertBody(node->children[newQuadrant].get(), body);

    } else { // Node is internal
        // Insert the body into the correct child quadrant.
        int quadrant = node->GetQuadrant(body->GetPosition());
        if (node->children[quadrant]) {
            InsertBody(node->children[quadrant].get(), body);
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

    while (!stack.empty()) {
        const QuadTreeNode* node = stack.back();
        stack.pop_back();

        if (!node || node->totalMass <= 0.0f) {
            continue;
        }

        glm::vec2 r = node->centerOfMass - body.GetPosition();
        float distanceSq = glm::dot(r, r);
        float nodeWidthSq = node->size * node->size;
        
        // CORRECTED LOGIC: The core Barnes-Hut approximation test (s/d < theta)
        // This is equivalent to s^2 / d^2 < theta^2, or s^2 < theta^2 * d^2
        if (nodeWidthSq < theta * theta * distanceSq) {
            // Node is far enough away, treat it as a single point mass.
            // This now correctly handles both far-away leaves and internal nodes.
        } else if (node->isLeaf) {
            // Node is a leaf, but it's too close. We must do a direct calculation.
            // We must also ensure it's not the body calculating force on itself.
            if (node->body == &body) {
                continue; // Skip self-interaction
            }
        } else {
            // Node is an internal node and it's too close.
            // Push its children onto the stack to investigate further.
            for (int i = 0; i < 4; ++i) {
                if (node->children[i]) {
                    stack.push_back(node->children[i].get());
                }
            }
            continue; // Skip the force calculation for this internal node, as we use its children.
        }

        // --- Perform Force Calculation ---
        // This block is reached if:
        // 1. The node was far enough away (approximated).
        // 2. The node was a close leaf (direct calculation, not self).
        
        // Add softening to prevent extreme forces when particles are very close.
        float effectiveDistSq = distanceSq + SOFTENING_LENGTH * SOFTENING_LENGTH;
        
        if (effectiveDistSq > 1e-12f) { // Avoid division by zero
            // REF-style force calculation: F = G * m * r_vec / (dist^2 + softening^2)^1.5
            float invDist = 1.0f / std::pow(effectiveDistSq, 1.5f);
            totalForce += r * (G * node->totalMass * invDist);
            m_stats.forceCalculations++;
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
    
    glm::vec2 minPos = bodies[0]->GetPosition();
    glm::vec2 maxPos = bodies[0]->GetPosition();
    
    for (size_t i = 1; i < bodies.size(); ++i) {
        const glm::vec2& pos = bodies[i]->GetPosition();
        minPos.x = std::min(minPos.x, pos.x);
        minPos.y = std::min(minPos.y, pos.y);
        maxPos.x = std::max(maxPos.x, pos.x);
        maxPos.y = std::max(maxPos.y, pos.y);
    }
    
    center = (minPos + maxPos) * 0.5f;
    float sizeX = maxPos.x - minPos.x;
    float sizeY = maxPos.y - minPos.y;
    size = std::max(sizeX, sizeY);
    
    size *= 1.05f; // Add 5% padding to prevent bodies from falling on the edge
    size = std::max(size, MIN_NODE_SIZE);
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