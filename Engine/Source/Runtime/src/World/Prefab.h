#pragma once
#include "Entity.h"
#include "IO/Filesystem.h"

namespace Gleam {

class EntityManager;

GSTRUCT(Prefab, "CAFCF979-D525-48D5-81CD-76731218F4DA", Serializable)
{
	GFIELD("E1B84680-4E33-4C68-9DDD-AD88FBCD3E6C", Serializable)
	TString name;
	//AssetReference parent; // TODO: how to handle nested prefabs?

	void Serialize(const EntityManager& entityManager, FileStream& stream) const;

	EntityHandle Deserialize(EntityManager& entityManager, FileStream& stream);

	static constexpr TWStringView Extension()
	{
		return L".prefab";
	}
};

} // namespace Gleam
