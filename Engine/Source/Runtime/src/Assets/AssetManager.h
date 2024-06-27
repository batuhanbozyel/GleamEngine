#pragma once
#include "Asset.h"
#include "AssetReference.h"
#include "Core/Subsystem.h"

namespace Gleam {

struct Asset;

class AssetManager final : public Subsystem
{
public:
    
    virtual void Initialize() override;

    virtual void Shutdown() override;
    
    const Asset& GetAsset(const AssetReference& asset) const;

private:
    
    HashMap<AssetReference, Asset> mAssets;

};

} // namespace Gleam
