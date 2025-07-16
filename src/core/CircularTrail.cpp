#include "core/CircularTrail.h"
#include <algorithm>
#include <stdexcept>

namespace nbody {

CircularTrail::CircularTrail() 
    : m_head(0), m_size(0), m_capacity(DEFAULT_CAPACITY) {
    m_points.resize(m_capacity);
}

CircularTrail::CircularTrail(int capacity) 
    : m_head(0), m_size(0), m_capacity(std::max(1, capacity)) {
    m_points.resize(m_capacity);
}

void CircularTrail::AddPoint(const glm::vec2& point) {
    // Store the point at the current head position
    m_points[m_head] = point;
    
    // Move head to next position (circular)
    m_head = (m_head + 1) % m_capacity;
    
    // Update size (capped at capacity)
    if (m_size < m_capacity) {
        m_size++;
    }
}

void CircularTrail::SetCapacity(int capacity) {
    int newCapacity = std::max(1, capacity);
    
    if (newCapacity == m_capacity) {
        return; // No change needed
    }
    
    if (newCapacity > m_capacity) {
        // Expanding: create new buffer and copy existing data
        std::vector<glm::vec2> newPoints(newCapacity);
        
        // Copy existing points in chronological order
        for (int i = 0; i < m_size; ++i) {
            newPoints[i] = GetPoint(i);
        }
        
        m_points = std::move(newPoints);
        m_head = m_size; // Next insertion point
        m_capacity = newCapacity;
    } else {
        // Shrinking: keep only the most recent points
        if (newCapacity < m_size) {
            // Need to discard some old points
            std::vector<glm::vec2> newPoints(newCapacity);
            
            // Copy the most recent points
            int startIndex = m_size - newCapacity;
            for (int i = 0; i < newCapacity; ++i) {
                newPoints[i] = GetPoint(startIndex + i);
            }
            
            m_points = std::move(newPoints);
            m_head = 0; // Next insertion will overwrite oldest
            m_size = newCapacity;
        } else {
            // Capacity reduction but still fits all points
            std::vector<glm::vec2> newPoints(newCapacity);
            
            // Copy all existing points
            for (int i = 0; i < m_size; ++i) {
                newPoints[i] = GetPoint(i);
            }
            
            m_points = std::move(newPoints);
            m_head = m_size;
        }
        
        m_capacity = newCapacity;
    }
}

void CircularTrail::Clear() {
    m_head = 0;
    m_size = 0;
}

const glm::vec2& CircularTrail::GetPoint(int index) const {
    if (index < 0 || index >= m_size) {
        throw std::out_of_range("Trail index out of range");
    }
    
    return m_points[LogicalToPhysical(index)];
}

std::vector<glm::vec2> CircularTrail::GetOrderedPoints() const {
    std::vector<glm::vec2> result;
    result.reserve(m_size);
    
    for (int i = 0; i < m_size; ++i) {
        result.push_back(GetPoint(i));
    }
    
    return result;
}

void CircularTrail::Reserve(int capacity) {
    if (capacity > m_capacity) {
        SetCapacity(capacity);
    }
}

int CircularTrail::LogicalToPhysical(int logicalIndex) const {
    // Calculate the starting position (oldest point)
    int startPos = (m_head - m_size + m_capacity) % m_capacity;
    
    // Add the logical offset
    return (startPos + logicalIndex) % m_capacity;
}

// Iterator implementation
CircularTrail::ConstIterator::ConstIterator(const CircularTrail* trail, int index)
    : m_trail(trail), m_currentIndex(index) {
}

const glm::vec2& CircularTrail::ConstIterator::operator*() const {
    return m_trail->GetPoint(m_currentIndex);
}

const glm::vec2* CircularTrail::ConstIterator::operator->() const {
    return &m_trail->GetPoint(m_currentIndex);
}

CircularTrail::ConstIterator& CircularTrail::ConstIterator::operator++() {
    ++m_currentIndex;
    return *this;
}

CircularTrail::ConstIterator CircularTrail::ConstIterator::operator++(int) {
    ConstIterator temp = *this;
    ++m_currentIndex;
    return temp;
}

bool CircularTrail::ConstIterator::operator==(const ConstIterator& other) const {
    return m_trail == other.m_trail && m_currentIndex == other.m_currentIndex;
}

bool CircularTrail::ConstIterator::operator!=(const ConstIterator& other) const {
    return !(*this == other);
}

CircularTrail::ConstIterator CircularTrail::begin() const {
    return ConstIterator(this, 0);
}

CircularTrail::ConstIterator CircularTrail::end() const {
    return ConstIterator(this, m_size);
}

} // namespace nbody
