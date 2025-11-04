#pragma once
#include <EASTL/map.h>
#include <EASTL/set.h>
#include <EASTL/unordered_map.h>
#include <EASTL/unordered_set.h>

namespace Gleam {

template<class Key, class Value, class Hasher = eastl::hash<Key>, class Comparator = eastl::equal_to<Key>>
using Map = eastl::map<Key, Value, Hasher, Comparator>;

template<class Key, class Hasher = eastl::hash<Key>, class Comparator = eastl::equal_to<Key>>
using Set = eastl::set<Key, Hasher, Comparator>;

template<class Key, class Value, class Hasher = eastl::hash<Key>, class Comparator = eastl::equal_to<Key>>
using HashMap = eastl::unordered_map<Key, Value, Hasher, Comparator>;

template<class Key, class Hasher = eastl::hash<Key>, class Comparator = eastl::equal_to<Key>>
using HashSet = eastl::unordered_set<Key, Hasher, Comparator>;

template <typename T>
constexpr void hash_combine(size_t& seed, const T& value)
{
    seed ^= std::hash<T>()(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

struct EnumClassHash
{
    template <typename T>
    size_t operator()(T t) const
    {
        return static_cast<size_t>(t);
    }
};

} // namespace Gleam
