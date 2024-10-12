#pragma once
#include "Entity.h"
#include "Assets/AssetReference.h"

namespace Gleam {

class EntityManager;

struct Prefab
{
	TString name;
	uint32_t entityCount;
	//AssetReference parent; // TODO: how to handle nested prefabs?

	void Serialize(const EntityManager& entityManager, FileStream& stream) const;

	TArray<EntityHandle> Deserialize(EntityManager& entityManager, FileStream& stream);
};

} // namespace Gleam

GLEAM_TYPE(Gleam::Prefab, Guid("CAFCF979-D525-48D5-81CD-76731218F4DA"))
	GLEAM_FIELD(name, Serializable())
	GLEAM_FIELD(entityCount, Serializable())
GLEAM_END
