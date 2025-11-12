#include "gpch.h"
#include "World.h"
#include "Systems/RenderSceneProxy.h"
#include "Serialization/JSONInternal.h"
#include "Serialization/JSONSerializer.h"
#include "Serialization/EntitySerializer.h"

using namespace Gleam;

World::World(const TString& name)
	: name(name)
{
	Timestep::Reset();
	AddSystem<RenderSceneProxy>();
}

World::~World()
{
	for (auto system : mSystems)
	{
		system->OnDestroy(mEntityManager);
	}
	mSystems.clear();
	mTickableSubsystems.clear();

	for (auto system : mSubsystems)
	{
		system->Shutdown();
	}
	mSubsystems.clear();
}

void World::Update()
{
	Timestep::Step();
	for (auto subsystem : mTickableSubsystems)
	{
		subsystem->Tick();
	}

	while (Timestep::InFixedTimeStep())
	{
		Timestep::FixedStep();
		for (auto system : mSystems)
		{
			if (system->Enabled)
			{
				system->OnFixedUpdate(mEntityManager);
			}
		}
	}
	Timestep::Update();
	
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
	
	JSONSerializer jsonSerializer;
	jsonSerializer.Serialize<World>(*this, root);

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
	
	JSONSerializer jsonSerializer;
	World world = jsonSerializer.Deserialize<World>(rapidjson::ConstNode(root));
	name = world.name;

	EntitySerializer serializer;
	serializer.Deserialize(rapidjson::ConstNode(root), mEntityManager);
}
