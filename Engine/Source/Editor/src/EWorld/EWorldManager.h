//
//  EWorldManager.h
//  Editor
//

#pragma once
#include "Core/Subsystem.h"
#include "Container/Pointer.h"
#include "IO/Path.h"

namespace Gleam {
class World;
} // namespace Gleam

namespace GEditor {

enum class PlayState
{
	Edit,
	Playing,
	Paused
};

class EWorldManager final : public Gleam::TickableGameInstanceSubsystem
{
public:

	EWorldManager(Gleam::World* world);

	~EWorldManager();

	virtual void Shutdown(Gleam::Application* app) override;

	virtual void Tick(Gleam::Application* app) override;

	void Save();

	void SaveAs(const Gleam::Path& file);

	void RequestPlay();

	void RequestPause();

	void RequestStep();

	void RequestStop();

	PlayState GetPlayState() const
	{
		return mPlayState;
	}

	bool IsSimulating() const
	{
		return mPlayState != PlayState::Edit;
	}

	Gleam::World* GetEditWorld() const
	{
		return mEditWorld;
	}

	Gleam::World* GetPlayWorld() const
	{
		return mPlayWorld.get();
	}

private:

	enum class Request
	{
		None,
		Play,
		Pause,
		Step,
		Stop
	};

	void BeginPlay(Gleam::Application* app);

	void EndPlay(Gleam::Application* app);

	void SetPhysicsEnabled(bool enabled);

	Gleam::World* mEditWorld = nullptr;

	Gleam::Scope<Gleam::World> mPlayWorld;

	PlayState mPlayState = PlayState::Edit;

	Request mRequest = Request::None;

};

} // namespace GEditor
