#pragma once
#include "Asset.h"
#include "AssetReference.h"
#include "Core/Subsystem.h"

#include <mutex>

namespace Gleam {

struct Asset;

class AssetManager final : public Subsystem
{
public:
    
    virtual void Initialize() override;

    virtual void Shutdown() override;
    
    const Asset& GetAsset(const AssetReference& asset) const;

private:

	void TryEmplaceAsset(const Asset& asset);

	std::mutex mMutex;
    
    HashMap<AssetReference, Asset> mAssets;

};

} // namespace Gleam
