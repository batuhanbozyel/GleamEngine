#include "gpch.h"
#include "World.h"
#include "Systems/RenderSceneProxy.h"
#include "Serialization/JSONInternal.h"
#include "Serialization/EntitySerializer.h"

using namespace Gleam;

World::World(const TString& name)
	: mName(name)
{
	Time::Reset();
	AddSystem<RenderSceneProxy>();
}

void World::Update()
{
	Time::Step();

	for (auto subsystem : mTickableSubsystems)
	{
		subsystem->Tick();
	}
	
	bool fixedUpdate = Time::fixedTime <= (Time::elapsedTime - Time::fixedDeltaTime);
	if (fixedUpdate)
	{
		Time::FixedStep();
		for (auto system : mSystems)
		{
			if (system->Enabled)
			{
				system->OnFixedUpdate(mEntityManager);
			}
		}
	}
	
	for (auto system : mSystems)
	{
		if (system->Enabled)
		{
			system->OnUpdate(mEntityManager);
		}
	}
}

void World::Serialize(FileStream& stream)
{
	rapidjson::Document document(rapidjson::kObjectType);
	rapidjson::Node root(document, document.GetAllocator());
	root.AddMember("Name", rapidjson::StringRef(mName.c_str()));

	EntitySerializer serializer;
	serializer.Serialize(mEntityManager, root);

	rapidjson::OStreamWrapper ss(stream);
	rapidjson::PrettyWriter writer(ss);
	writer.SetFormatOptions(rapidjson::PrettyFormatOptions::kFormatSingleLineArray);
	writer.SetMaxDecimalPlaces(6);
	writer.SetIndent('\t', 1);
	root.object.Accept(writer);
}

void World::Deserialize(FileStream& stream)
{
	rapidjson::Document root(rapidjson::kObjectType);
	rapidjson::IStreamWrapper ss(stream);
	root.ParseStream(ss);

	mName = TString(root["Name"].GetString());

	EntitySerializer serializer;
	serializer.Deserialize(rapidjson::ConstNode(root), mEntityManager);
}
