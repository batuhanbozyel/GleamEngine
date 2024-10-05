#include "gpch.h"
#include "Prefab.h"
#include "World.h"

#include "Serialization/JSONInternal.h"
#include "Serialization/JSONSerializer.h"
#include "Serialization/EntitySerializer.h"

using namespace Gleam;

void Prefab::Serialize(const EntityManager& entityManager, FileStream& stream) const
{
	rapidjson::Document document(rapidjson::kObjectType);
	rapidjson::Node root(document, document.GetAllocator());

	JSONSerializer jsonSerializer(stream);
	jsonSerializer.Serialize<Prefab>(*this, root);

	EntitySerializer entitySerializer(stream);
	entitySerializer.Serialize(entityManager, root);
}

TArray<EntityHandle> Prefab::Deserialize(EntityManager& entityManager, FileStream& stream)
{
	rapidjson::Document root(rapidjson::kObjectType);
	rapidjson::IStreamWrapper ss(stream);
	root.ParseStream(ss);

	JSONSerializer jsonSerializer(stream);
	*this = jsonSerializer.Deserialize<Prefab>(rapidjson::ConstNode(root));

	EntitySerializer entitySerializer(stream);
	return entitySerializer.Deserialize(rapidjson::ConstNode(root), entityManager);
}