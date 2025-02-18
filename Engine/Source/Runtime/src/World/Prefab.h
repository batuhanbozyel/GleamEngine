#pragma once
#include "Entity.h"

namespace Gleam {

class EntityManager;

struct Prefab
{
	TString name;
	//AssetReference parent; // TODO: how to handle nested prefabs?

	void Serialize(const EntityManager& entityManager, FileStream& stream) const;

	EntityHandle Deserialize(EntityManager& entityManager, FileStream& stream);

	static constexpr TStringView Extension()
	{
		return ".prefab";
	}
};

} // namespace Gleam

GLEAM_TYPE(Gleam::Prefab, Guid("CAFCF979-D525-48D5-81CD-76731218F4DA"))
	GLEAM_FIELD(name, Serializable())
GLEAM_END
