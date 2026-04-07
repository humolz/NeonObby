#pragma once

#include "ecs/Entity.h"
#include "ecs/ComponentPool.h"
#include <vector>
#include <typeindex>
#include <unordered_map>
#include <memory>

class IComponentPoolBase {
public:
    virtual ~IComponentPoolBase() = default;
    virtual void remove(uint32_t entityId) = 0;
    virtual bool has(uint32_t entityId) const = 0;
    virtual size_t size() const = 0;
};

template<typename T>
class ComponentPoolWrapper : public IComponentPoolBase {
public:
    ComponentPool<T> pool;
    void remove(uint32_t entityId) override { pool.remove(entityId); }
    bool has(uint32_t entityId) const override { return pool.has(entityId); }
    size_t size() const override { return pool.size(); }
};

class World {
public:
    Entity create() {
        uint32_t id;
        if (!m_freeIds.empty()) {
            id = m_freeIds.back();
            m_freeIds.pop_back();
        } else {
            id = m_nextId++;
            m_generations.push_back(0);
        }
        Entity e(id, m_generations[id]);
        m_alive.push_back(e);
        return e;
    }

    void destroy(Entity e) {
        if (!alive(e)) return;
        for (auto& [_, pool] : m_pools) {
            pool->remove(e.id);
        }
        m_generations[e.id]++;
        m_freeIds.push_back(e.id);
        m_alive.erase(
            std::remove(m_alive.begin(), m_alive.end(), e),
            m_alive.end()
        );
    }

    bool alive(Entity e) const {
        return e.id < m_generations.size() && m_generations[e.id] == e.gen;
    }

    template<typename T>
    T& add(Entity e, T component = {}) {
        return getPool<T>().add(e.id, std::move(component));
    }

    template<typename T>
    void remove(Entity e) {
        getPool<T>().remove(e.id);
    }

    template<typename T>
    T& get(Entity e) {
        return getPool<T>().get(e.id);
    }

    template<typename T>
    const T& get(Entity e) const {
        return getPool<T>().get(e.id);
    }

    template<typename T>
    bool has(Entity e) const {
        auto it = m_pools.find(std::type_index(typeid(T)));
        if (it == m_pools.end()) return false;
        return it->second->has(e.id);
    }

    // Iterate entities having all specified component types.
    // Callback: void(Entity, T1&, T2&, ...)
    template<typename First, typename... Rest, typename Func>
    void each(Func&& func) {
        auto& iterPool = getPool<First>();
        for (size_t i = 0; i < iterPool.size(); i++) {
            uint32_t eid = iterPool.entityAt(i);
            if (hasAll<First, Rest...>(eid)) {
                Entity e(eid, m_generations[eid]);
                func(e, getPool<First>().get(eid), getPool<Rest>().get(eid)...);
            }
        }
    }

    size_t entityCount() const { return m_alive.size(); }
    const std::vector<Entity>& entities() const { return m_alive; }

private:
    uint32_t m_nextId = 1;
    std::vector<uint32_t> m_generations{0};
    std::vector<uint32_t> m_freeIds;
    std::vector<Entity> m_alive;
    std::unordered_map<std::type_index, std::unique_ptr<IComponentPoolBase>> m_pools;

    template<typename T>
    ComponentPool<T>& getPool() {
        auto key = std::type_index(typeid(T));
        auto it = m_pools.find(key);
        if (it == m_pools.end()) {
            auto wrapper = std::make_unique<ComponentPoolWrapper<T>>();
            auto* ptr = &wrapper->pool;
            m_pools[key] = std::move(wrapper);
            return *ptr;
        }
        return static_cast<ComponentPoolWrapper<T>*>(it->second.get())->pool;
    }

    template<typename T>
    const ComponentPool<T>& getPool() const {
        auto key = std::type_index(typeid(T));
        return static_cast<const ComponentPoolWrapper<T>*>(m_pools.at(key).get())->pool;
    }

    template<typename... Ts>
    bool hasAll(uint32_t eid) const {
        auto check = [&](auto key) {
            auto it = m_pools.find(key);
            return it != m_pools.end() && it->second->has(eid);
        };
        return (check(std::type_index(typeid(Ts))) && ...);
    }
};
