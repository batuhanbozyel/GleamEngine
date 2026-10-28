//
//  ScriptReflection.cpp
//  Editor
//

#include "ScriptReflection.h"
#include "ProjectSolution.h"

#include "World/EntityManager.h"
#include "Serialization/JSONSerializer.h"

#include <EASTL/algorithm.h>

using namespace GEditor;

struct StashedComponent
{
	Gleam::EntityManager* entityManager = nullptr;
	Gleam::EntityHandle entity = Gleam::InvalidEntity;
	bool singleton = false;
	Gleam::Guid typeGuid;
	Gleam::TString data;
};

ScriptReflection::ScriptReflection(const Gleam::Project& project)
	: mSolution(new ProjectSolution(project))
{

}

void ScriptReflection::Shutdown(Gleam::Engine* engine)
{
	mLifecycles.UnregisterAll();
	delete mSolution;
}

bool ScriptReflection::Refresh(Gleam::TArrayView<Gleam::EntityManager* const> entityManagers)
{
	const auto result = mSolution->BuildReflection();
	if (result.success == false)
	{
		GLEAM_CORE_ERROR("Reflection generation failed:\n{0}", result.output);
		return false;
	}

	const auto databaseTime = Gleam::Filesystem::LastWriteTime(mSolution->GetReflectionDatabase());
	if (databaseTime != mDatabaseTime)
	{
		Reload(entityManagers);
		mDatabaseTime = databaseTime;
	}
	return true;
}

void ScriptReflection::Reload(Gleam::TArrayView<Gleam::EntityManager* const> entityManagers)
{
	const auto& scriptTypes = mLifecycles.GetTypes();
	auto isScriptType = [&scriptTypes](const Gleam::Reflection::ClassDescription& classDesc)
	{
		return eastl::find(scriptTypes.begin(), scriptTypes.end(), classDesc.TypeHash()) != scriptTypes.end();
	};

	// Script components are written with the old layout and read back with the new one, fields are matched by guid
	Gleam::TArray<StashedComponent> stash;
	Gleam::JSONSerializer serializer;
	for (auto entityManager : entityManagers)
	{
		const auto& constManager = *entityManager;
		auto stashComponents = [&](Gleam::EntityHandle entity, bool singleton)
		{
			return [&, entity, singleton](const void* component, const Gleam::Reflection::ClassDescription& classDesc)
			{
				if (isScriptType(classDesc))
				{
					stash.push_back({ entityManager, entity, singleton, classDesc.Guid(), serializer.Serialize(component, classDesc) });
				}
			};
		};
		constManager.ForEach([&](Gleam::EntityHandle entity)
		{
			constManager.Visit(entity, stashComponents(entity, false));
		});
		constManager.VisitSingletons(stashComponents(Gleam::InvalidEntity, true));

		for (const auto typeHash : scriptTypes)
		{
			entityManager->ResetComponentStorage(typeHash);
		}
	}
	mLifecycles.UnregisterAll();

	auto database = Gleam::Reflection::IDatabase::GetInstance();
	database->Shutdown();
	database->Initialize(mSolution->GetReflectionDatabase());
	mLifecycles.RegisterAll();

	for (const auto& stashed : stash)
	{
		// Components of deleted classes are dropped
		const auto classDesc = database->GetClass(stashed.typeGuid);
		if (classDesc == nullptr)
		{
			continue;
		}

		auto component = stashed.singleton ? stashed.entityManager->AddSingleton(classDesc->TypeHash()) : stashed.entityManager->AddComponent(stashed.entity, classDesc->TypeHash());
		serializer.Deserialize(*classDesc, component, stashed.data);
	}
}
