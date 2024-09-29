//
//  MaterialSystem.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 9.07.2023.
//

#pragma once
#include "Core/Subsystem.h"
#include "Material.h"

namespace Gleam {

class MaterialSystem final : public GameInstanceSubsystem
{
public:

	virtual void Initialize(Application* app) override;

	virtual void Shutdown() override;

	Material* GetMaterial(const AssetReference& ref);

private:

	HashMap<AssetReference, Scope<Material>> mMaterials;

};

} // namespace Gleam
