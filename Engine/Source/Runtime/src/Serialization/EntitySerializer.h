#pragma once
#include "World/EntityManager.h"

namespace rapidjson {
struct Node;
struct ConstNode;
} // namespace rapidjson

namespace Gleam {

class EntitySerializer final
{
public:
    
    void Serialize(const EntityManager& entityManager, rapidjson::Node& root);
    
	TArray<EntityHandle> Deserialize(const rapidjson::ConstNode& root, EntityManager& entityManager);
    
};

} // namespace Gleam
