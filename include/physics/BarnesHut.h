#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <array>

namespace nbody {

class Body;

/**
 * @brief Spatial partitioning node for Barnes-Hut algorithm
 */
struct QuadTreeNode {
    glm::vec2 center{0.0f};
    float size = 0.0f;
    
    // Physical properties
    float totalMass = 0.0f;
    glm::vec2 centerOfMass{0.0f};
    
    // Tree structure
    std::array<std::unique_ptr<QuadTreeNode>, 4> children;
    Body* body = nullptr;
    bool isLeaf = true;
    
    // Bounds checking
    bool Contains(const glm::vec2& point) const {
        float halfSize = size * 0.5f;
        return (point.x >= center.x - halfSize && point.x <= center.x + halfSize &&
                point.y >= center.y - halfSize && point.y <= center.y + halfSize);
    }
    
    // Get quadrant index for a point
    int GetQuadrant(const glm::vec2& point) const {
        int index = 0;
        if (point.x > center.x) index |= 1;
        if (point.y > center.y) index |= 2;
        return index;
    }
    
    // Get child center for quadrant
    glm::vec2 GetChildCenter(int quadrant) const {
        float quarter = size * 0.25f;
        float x = center.x + ((quadrant & 1) ? quarter : -quarter);
        float y = center.y + ((quadrant & 2) ? quarter : -quarter);
        return glm::vec2(x, y);
    }
};

/**
 * @brief Barnes-Hut tree for O(N log N) force calculations
 */
class BarnesHutTree {
public:
    BarnesHutTree();
    ~BarnesHutTree() = default;

    /**
     * @brief Build the tree from a collection of bodies
     * @param bodies Vector of bodies to build tree from
     */
    void BuildTree(const std::vector<std::unique_ptr<Body>>& bodies);

    /**
     * @brief Calculate force on a body using Barnes-Hut approximation
     * @param body Target body
     * @param theta Approximation parameter (0.5 is typical)
     * @return Force vector
     */
    glm::vec2 CalculateForce(const Body& body, float theta, float G) const;

    /**
     * @brief Get tree statistics
     */
    struct TreeStats {
        int totalNodes = 0;
        int leafNodes = 0;
        int maxDepth = 0;
        int forceCalculations = 0;
    };
    
    const TreeStats& GetStats() const { return m_stats; }
    
    /**
     * @brief Get root node for visualization
     */
    const QuadTreeNode* GetRoot() const { return m_root.get(); }
    
private:
    std::unique_ptr<QuadTreeNode> m_root;
    mutable TreeStats m_stats;
    
    // Tree building
    void InsertBody(QuadTreeNode* node, Body* body);
    void Subdivide(QuadTreeNode* node);
    void UpdateMassAndCenter(QuadTreeNode* node);
    
    // Force calculation
    glm::vec2 CalculateForceRecursive(const QuadTreeNode* node, const Body& body, 
                                     float theta, float G) const;
    
    // Utility
    void CalculateBounds(const std::vector<std::unique_ptr<Body>>& bodies,
                        glm::vec2& center, float& size) const;
    
    void CountNodes(const QuadTreeNode* node, TreeStats& stats, int depth = 0) const;
    
    // Constants
    static constexpr float SOFTENING_LENGTH = 0.01f;
    static constexpr float MIN_NODE_SIZE = 0.001f;
};

} // namespace nbody
