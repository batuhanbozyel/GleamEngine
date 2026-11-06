//
//  EWorldManager.cpp
//  Editor
//

#include "EWorldManager.h"
#include "EAssets/AssetWriter.h"
#include "Scripting/ProjectSolution.h"
#include "Scripting/ScriptReflection.h"

#include "Core/Globals.h"
#include "Core/Application.h"
#include "Core/Engine.h"
#include "Core/Process.h"
#include "Core/ProcessLink.h"
#include "Core/CommandLine.h"
#include "Networking/IPCChannel.h"
#include "Assets/AssetManager.h"
#include "World/World.h"

#include "Serialization/JSONInternal.h"
#include "Serialization/EntitySerializer.h"

using namespace GEditor;

static constexpr uint32_t kQuitTimeout = 2000;

EWorldManager::EWorldManager(Gleam::World* world)
	: mEditWorld(world)
{

}

EWorldManager::~EWorldManager() = default;

void EWorldManager::Shutdown(Gleam::Application* app)
{
	if (mGame)
	{
		EndPlay(app);
	}
}

void EWorldManager::Tick(Gleam::Application* app)
{
	if (mGame and mGame->IsRunning() == false)
	{
		if (mGame->GetExitCode() != 0)
		{
			GLEAM_WARN("Game exited with code {0}", mGame->GetExitCode());
		}
		ReleasePlaySession();
	}

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
			else if (mPlayState == PlayState::Paused and mLink->Send(static_cast<uint32_t>(Gleam::ProcessLinkMessage::Resume)))
			{
				mPlayState = PlayState::Playing;
			}
			break;
		}
		case Request::Pause:
		{
			if (mPlayState == PlayState::Playing and mLink->Send(static_cast<uint32_t>(Gleam::ProcessLinkMessage::Pause)))
			{
				mPlayState = PlayState::Paused;
			}
			break;
		}
		case Request::Step:
		{
			if (mPlayState == PlayState::Paused)
			{
				mLink->Send(static_cast<uint32_t>(Gleam::ProcessLinkMessage::Step));
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
		case Request::Save:
		{
			// A failed script reflection build leaves the world unsaved
			Gleam::EntityManager* entityManagers[] = { &mEditWorld->GetEntityManager() };
			if (mPlayState == PlayState::Edit and Gleam::Globals::Engine->GetSubsystem<ScriptReflection>()->Refresh(entityManagers))
			{
				Save();
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

void EWorldManager::RequestSave()
{
	mRequest = Request::Save;
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
	auto& solution = Gleam::Globals::Engine->GetSubsystem<ScriptReflection>()->GetSolution();
	const auto build = solution.BuildGame();
	const auto processId = Gleam::Process::CurrentId();
	if (build.success)
	{
		mLink = Gleam::IPCChannel::Listen(Gleam::ProcessLink::ChannelName(processId));
	}

	if (mLink)
	{
		const auto sessionDirectory = Gleam::Globals::ProjectDirectory / "Intermediate" / "PlaySession";
		Gleam::Filesystem::CreateDirectories(sessionDirectory);

		Gleam::TString snapshotName = mEditWorld->GetReference().guid.ToString();
		snapshotName.append_convert(Gleam::World::Extension().data(), Gleam::World::Extension().size());
		mSnapshot = sessionDirectory / snapshotName;
		SaveAs(mSnapshot);

		mGame = Gleam::Process::Launch(solution.GetGameExecutable(),
		{
			"-project=" + Gleam::Globals::CLI->GetParam<Gleam::TString>("project", Gleam::TString()),
			"-link-process=" + Gleam::TString(std::to_string(processId)),
			"-world=" + mSnapshot.String(),
			"-log-file=" + (sessionDirectory / "Game.log").String()
		}, Gleam::Globals::StartupDirectory);
	}

	if (mGame)
	{
		mPlayState = PlayState::Playing;
	}
	else
	{
		if (build.success == false)
		{
			GLEAM_ERROR("Play is cancelled, the game build failed:\n{0}", build.output);
		}
		else
		{
			GLEAM_ERROR("Play is cancelled, the game could not be launched: {0}", solution.GetGameExecutable().String());
		}
		ReleasePlaySession();
	}
}

void EWorldManager::EndPlay(Gleam::Application* app)
{
	if (mLink->Send(static_cast<uint32_t>(Gleam::ProcessLinkMessage::Quit)) == false or mGame->WaitForExit(kQuitTimeout) == false)
	{
		mGame->Terminate();
	}
	ReleasePlaySession();
}

void EWorldManager::ReleasePlaySession()
{
	delete mGame;
	mGame = nullptr;
	delete mLink;
	mLink = nullptr;
	if (mSnapshot.Empty() == false)
	{
		Gleam::Filesystem::Remove(mSnapshot);
		mSnapshot = Gleam::Path();
	}
	mPlayState = PlayState::Edit;
}
