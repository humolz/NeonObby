#pragma once

#include <cstdint>
#include <functional>

struct Entity {
    uint32_t id  : 24;
    uint32_t gen : 8;

    Entity() : id(0), gen(0) {}
    Entity(uint32_t i, uint32_t g) : id(i), gen(g) {}

    bool operator==(const Entity& o) const { return id == o.id && gen == o.gen; }
    bool operator!=(const Entity& o) const { return !(*this == o); }

    bool valid() const { return id != 0 || gen != 0; }
    uint32_t packed() const { return (gen << 24) | id; }

    static Entity null() { return Entity(0, 0); }
};

template<>
struct std::hash<Entity> {
    size_t operator()(const Entity& e) const noexcept {
        return std::hash<uint32_t>()(e.packed());
    }
};
