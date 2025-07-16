#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace nbody {

/**
 * @brief Efficient circular buffer for particle trails
 * 
 * This class implements a circular buffer that provides O(1) insertion
 * and efficient memory usage for particle trail systems. Unlike std::vector
 * based trails, this doesn't require shifting elements or frequent reallocations.
 */
class CircularTrail {
public:
    /**
     * @brief Construct a new CircularTrail with default capacity
     */
    CircularTrail();
    
    /**
     * @brief Construct a new CircularTrail with specified capacity
     * @param capacity Maximum number of trail points
     */
    explicit CircularTrail(int capacity);
    
    /**
     * @brief Add a new point to the trail (O(1) operation)
     * @param point The point to add
     */
    void AddPoint(const glm::vec2& point);
    
    /**
     * @brief Set the maximum capacity of the trail
     * @param capacity New capacity (efficiently resizes if needed)
     */
    void SetCapacity(int capacity);
    
    /**
     * @brief Get the current capacity
     * @return Maximum number of points the trail can hold
     */
    int GetCapacity() const { return m_capacity; }
    
    /**
     * @brief Get the current number of points in the trail
     * @return Number of active trail points
     */
    int GetSize() const { return m_size; }
    
    /**
     * @brief Check if the trail is empty
     * @return True if no points are stored
     */
    bool IsEmpty() const { return m_size == 0; }
    
    /**
     * @brief Check if the trail is at full capacity
     * @return True if the trail cannot hold more points
     */
    bool IsFull() const { return m_size == m_capacity; }
    
    /**
     * @brief Clear all points from the trail
     */
    void Clear();
    
    /**
     * @brief Get a point at a specific index (0 = oldest, size-1 = newest)
     * @param index Index of the point to retrieve
     * @return The point at the specified index
     */
    const glm::vec2& GetPoint(int index) const;
    
    /**
     * @brief Get all trail points in chronological order for rendering
     * 
     * This method creates a temporary vector with points in the correct order.
     * For performance-critical rendering, prefer using the iterator interface.
     * 
     * @return Vector of points from oldest to newest
     */
    std::vector<glm::vec2> GetOrderedPoints() const;
    
    /**
     * @brief Reserve memory for the trail buffer
     * @param capacity Capacity to reserve
     */
    void Reserve(int capacity);

    // Iterator support for efficient rendering
    class ConstIterator {
    public:
        ConstIterator(const CircularTrail* trail, int index);
        
        const glm::vec2& operator*() const;
        const glm::vec2* operator->() const;
        ConstIterator& operator++();
        ConstIterator operator++(int);
        bool operator==(const ConstIterator& other) const;
        bool operator!=(const ConstIterator& other) const;
        
    private:
        const CircularTrail* m_trail;
        int m_currentIndex;
    };
    
    /**
     * @brief Get iterator to the beginning (oldest point)
     */
    ConstIterator begin() const;
    
    /**
     * @brief Get iterator to the end
     */
    ConstIterator end() const;

private:
    std::vector<glm::vec2> m_points;    // Circular buffer storage
    int m_head;                         // Index of the next insertion point
    int m_size;                         // Current number of points
    int m_capacity;                     // Maximum capacity
    
    static constexpr int DEFAULT_CAPACITY = 100;
    
    /**
     * @brief Convert logical index to physical buffer index
     * @param logicalIndex Index from 0 (oldest) to size-1 (newest)
     * @return Physical index in the buffer
     */
    int LogicalToPhysical(int logicalIndex) const;
};

} // namespace nbody
