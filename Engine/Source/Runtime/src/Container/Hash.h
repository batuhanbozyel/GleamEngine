#pragma once
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

namespace Gleam {

template<class Key, class Value, class Hasher = std::hash<Key>, class Comparator = std::equal_to<Key>>
using Map = std::map<Key, Value, Hasher, Comparator>;
    
template<class Key, class Hasher = std::hash<Key>, class Comparator = std::equal_to<Key>>
using Set = std::set<Key, Hasher, Comparator>;

template<class Key, class Value, class Hasher = std::hash<Key>, class Comparator = std::equal_to<Key>>
using HashMap = std::unordered_map<Key, Value, Hasher, Comparator>;

template<class Key, class Hasher = std::hash<Key>, class Comparator = std::equal_to<Key>>
using HashSet = std::unordered_set<Key, Hasher, Comparator>;

} // namespace Gleam
