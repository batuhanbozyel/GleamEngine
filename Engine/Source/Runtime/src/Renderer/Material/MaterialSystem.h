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

class MaterialSystem final : public Subsystem
{
public:

	virtual void Initialize() override;

	virtual void Shutdown() override;

	Material* GetMaterial(const AssetReference& ref) const;

private:

	HashMap<AssetReference, Material*> mMaterials;

};

} // namespace Gleam
