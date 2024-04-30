#pragma once
#include "World.h"
#include "WorldConfig.h"
#include "Core/Subsystem.h"
#include "Assets/AssetReference.h"

namespace Gleam {

class WorldManager final : public Subsystem
{
public:

	virtual void Initialize() override;

	virtual void Shutdown() override;

	void Configure(const WorldConfig& config);

	size_t LoadWorld(uint32_t buildIndex);

	void UnloadWorld(uint32_t index);

	World* GetWorld(uint32_t index);

	World* GetActiveWorld();

private:

	size_t mActiveWorld = 0;
	TArray<Scope<World>> mLoadedWorlds;
	TArray<AssetReference> mWorldsInBuild;

};


} // namespace Gleam
