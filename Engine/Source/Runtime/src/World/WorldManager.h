#pragma once
#include "World.h"
#include "WorldConfig.h"
#include "Core/Subsystem.h"
#include "Assets/AssetReference.h"

namespace Gleam {

class WorldManager final : public GameInstanceSubsystem
{
public:

	virtual void Initialize(Application* app) override;

	virtual void Shutdown() override;

	void Configure(const WorldConfig& config);

	void OpenWorld(uint32_t buildIndex);

	void LoadWorld(uint32_t buildIndex);

	void SaveWorld();

	World* GetActiveWorld();

private:

	std::mutex mMutex;

	uint32_t mActiveWorld = 0;

	Application* mApp = nullptr;

	TArray<AssetReference> mWorldsInBuild;

	HashMap<AssetReference, Scope<World>> mLoadedWorlds;

	HashMap<AssetReference, Filesystem::Path> mWorldPaths;

};

} // namespace Gleam
