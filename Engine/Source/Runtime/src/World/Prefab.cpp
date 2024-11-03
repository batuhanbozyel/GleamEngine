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

	JSONSerializer jsonSerializer;
	jsonSerializer.Serialize<Prefab>(*this, root);

	EntitySerializer entitySerializer;
	entitySerializer.Serialize(entityManager, root);

	rapidjson::OStreamWrapper ss(stream);
	rapidjson::PrettyWriter writer(ss);
	writer.SetFormatOptions(rapidjson::PrettyFormatOptions::kFormatSingleLineArray);
	writer.SetMaxDecimalPlaces(6);
	writer.SetIndent('\t', 1);
	root.object.Accept(writer);
}

TArray<EntityHandle> Prefab::Deserialize(EntityManager& entityManager, FileStream& stream)
{
	rapidjson::Document root(rapidjson::kObjectType);
	rapidjson::IStreamWrapper ss(stream);
	root.ParseStream(ss);

	JSONSerializer jsonSerializer;
	*this = jsonSerializer.Deserialize<Prefab>(rapidjson::ConstNode(root));

	EntitySerializer entitySerializer;
	return entitySerializer.Deserialize(rapidjson::ConstNode(root), entityManager);
}