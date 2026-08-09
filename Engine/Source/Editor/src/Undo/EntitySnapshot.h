//
//  EntitySnapshot.h
//  Editor
//

#pragma once
#include "Core/GUID.h"
#include "World/Entity.h"
#include "Container/Array.h"
#include "Container/String.h"

namespace Gleam {
class EntityManager;
} // namespace Gleam

namespace GEditor {

class EntitySnapshot
{
public:

	void Capture(const Gleam::EntityManager& entityManager, const Gleam::TArray<Gleam::EntityHandle>& entities);

	Gleam::TArray<Gleam::EntityHandle> Restore(Gleam::EntityManager& entityManager) const;

	Gleam::TArray<Gleam::EntityHandle> Destroy(Gleam::EntityManager& entityManager) const;

private:

	Gleam::TArray<Gleam::EntityHandle> Resolve(const Gleam::EntityManager& entityManager) const;

	Gleam::TString mSerializedObject;

	Gleam::TArray<Gleam::Guid> mEntities;

};

} // namespace GEditor
