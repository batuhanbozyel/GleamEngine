//
//  EWorldManager.cpp
//  Editor
//

#include "EWorldManager.h"
#include "EAssets/AssetWriter.h"
#include "Physics/PhysicsVisualizationSystem.h"

#include "Core/Globals.h"
#include "Core/Application.h"
#include "Assets/AssetManager.h"
#include "Physics/PhysicsSystem.h"
#include "World/World.h"
#include "World/WorldManager.h"

#include "Serialization/JSONInternal.h"
#include "Serialization/EntitySerializer.h"

using namespace GEditor;

EWorldManager::EWorldManager(Gleam::World* world)
	: mEditWorld(world)
{

}

EWorldManager::~EWorldManager() = default;

void EWorldManager::Shutdown(Gleam::Application* app)
{
	if (mPlayWorld)
	{
		EndPlay(app);
	}
}

void EWorldManager::Tick(Gleam::Application* app)
{
	const auto request = mRequest;
	mRequest = Request::None;

	switch (request)
	{
		case Request::Play:
		{
			if (mPlayState == PlayState::Edit)
			{
				BeginPlay(app);
			}
			else if (mPlayState == PlayState::Paused)
			{
				SetPhysicsEnabled(true);
				mPlayState = PlayState::Playing;
			}
			break;
		}
		case Request::Pause:
		{
			if (mPlayState == PlayState::Playing)
			{
				SetPhysicsEnabled(false);
				mPlayState = PlayState::Paused;
			}
			break;
		}
		case Request::Step:
		{
			if (mPlayState == PlayState::Paused)
			{
				mPlayWorld->GetSystem<Gleam::PhysicsSystem>()->OnFixedUpdate(mPlayWorld->GetEntityManager());
			}
			break;
		}
		case Request::Stop:
		{
			if (mPlayState != PlayState::Edit)
			{
				EndPlay(app);
			}
			break;
		}
		default:
		{
			break;
		}
	}
}

void EWorldManager::Save()
{
	const auto& path = Gleam::Globals::GameInstance->GetSubsystem<Gleam::AssetManager>()->GetAssetPath(mEditWorld->GetReference());
	SaveAs(Gleam::Globals::ProjectContentDirectory / path);
}

void EWorldManager::SaveAs(const Gleam::Path& file)
{
	rapidjson::Document entities(rapidjson::kObjectType);
	rapidjson::Node entitiesNode(entities, entities.GetAllocator());

	Gleam::EntitySerializer serializer;
	serializer.Serialize(mEditWorld->GetEntityManager(), entitiesNode);

	JSONAssetWriter writer;
	writer.AddBlob<Gleam::Entity>(entitiesNode, Gleam::AssetPlatform::Common, Gleam::AssetBackend::Common);
	writer.Write(file, mEditWorld->GetHeader(), mEditWorld->GetDescriptor());
}

void EWorldManager::RequestPlay()
{
	mRequest = Request::Play;
}

void EWorldManager::RequestPause()
{
	mRequest = Request::Pause;
}

void EWorldManager::RequestStep()
{
	mRequest = Request::Step;
}

void EWorldManager::RequestStop()
{
	mRequest = Request::Stop;
}

void EWorldManager::BeginPlay(Gleam::Application* app)
{
	mPlayWorld = Gleam::CreateScope<Gleam::World>(Gleam::AssetReference{}, mEditWorld->GetHeader(), mEditWorld->GetDescriptor());
	mPlayWorld->GetEntityManager().CopyFrom(mEditWorld->GetEntityManager());

	auto visualization = mPlayWorld->AddSystem<PhysicsVisualizationSystem>();
	if (mEditWorld->HasSystem<PhysicsVisualizationSystem>())
	{
		visualization->SetSettings(mEditWorld->GetSystem<PhysicsVisualizationSystem>()->GetSettings());
	}

	app->GetSubsystem<Gleam::WorldManager>()->SetActiveWorld(mPlayWorld.get());
	mPlayState = PlayState::Playing;
}

void EWorldManager::EndPlay(Gleam::Application* app)
{
	app->GetSubsystem<Gleam::WorldManager>()->SetActiveWorld(mEditWorld);
	mPlayWorld.reset();
	mPlayState = PlayState::Edit;
}

void EWorldManager::SetPhysicsEnabled(bool enabled)
{
	mPlayWorld->GetSystem<Gleam::PhysicsSystem>()->Enabled = enabled;
}
