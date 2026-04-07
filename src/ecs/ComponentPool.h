#pragma once

#include <vector>
#include <cstdint>
#include <cassert>
#include <limits>

static constexpr uint32_t INVALID_INDEX = std::numeric_limits<uint32_t>::max();

template<typename T>
class ComponentPool {
public:
    bool has(uint32_t entityId) const {
        return entityId < m_sparse.size() && m_sparse[entityId] != INVALID_INDEX;
    }

    T& get(uint32_t entityId) {
        assert(has(entityId));
        return m_dense[m_sparse[entityId]];
    }

    const T& get(uint32_t entityId) const {
        assert(has(entityId));
        return m_dense[m_sparse[entityId]];
    }

    T& add(uint32_t entityId, T component = {}) {
        if (entityId >= m_sparse.size()) {
            m_sparse.resize(entityId + 1, INVALID_INDEX);
        }
        if (m_sparse[entityId] != INVALID_INDEX) {
            m_dense[m_sparse[entityId]] = std::move(component);
            return m_dense[m_sparse[entityId]];
        }
        uint32_t denseIdx = static_cast<uint32_t>(m_dense.size());
        m_sparse[entityId] = denseIdx;
        m_dense.push_back(std::move(component));
        m_entityIds.push_back(entityId);
        return m_dense.back();
    }

    void remove(uint32_t entityId) {
        if (!has(entityId)) return;

        uint32_t denseIdx = m_sparse[entityId];
        uint32_t lastIdx = static_cast<uint32_t>(m_dense.size()) - 1;

        if (denseIdx != lastIdx) {
            m_dense[denseIdx] = std::move(m_dense[lastIdx]);
            m_entityIds[denseIdx] = m_entityIds[lastIdx];
            m_sparse[m_entityIds[denseIdx]] = denseIdx;
        }

        m_dense.pop_back();
        m_entityIds.pop_back();
        m_sparse[entityId] = INVALID_INDEX;
    }

    size_t size() const { return m_dense.size(); }

    T* data() { return m_dense.data(); }
    const uint32_t* entities() const { return m_entityIds.data(); }

    // Iteration support
    auto begin() { return m_dense.begin(); }
    auto end() { return m_dense.end(); }
    auto begin() const { return m_dense.begin(); }
    auto end() const { return m_dense.end(); }

    uint32_t entityAt(size_t denseIndex) const { return m_entityIds[denseIndex]; }

private:
    std::vector<T> m_dense;
    std::vector<uint32_t> m_sparse;
    std::vector<uint32_t> m_entityIds;
};
