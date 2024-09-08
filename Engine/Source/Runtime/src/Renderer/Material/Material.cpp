//
//  Material.cpp
//  GleamEngine
//
//  Created by Batuhan Bozyel on 3.04.2023.
//

#include "gpch.h"
#include "Material.h"

#include "Core/Engine.h"
#include "Core/Globals.h"
#include "Renderer/RenderSystem.h"

using namespace Gleam;

Material::Material(const MaterialDescriptor& descriptor)
    : IMaterial(descriptor.properties)
    , mName(descriptor.name)
{
    static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
    // TODO: Allocate GPU buffer
}

MaterialInstance Material::CreateInstance()
{
    // TODO: Suballocate material instance data from material buffer
    return MaterialInstance(this, mInstanceCount++);
}

void Material::Dispose()
{
	// TOOD: dispose material buffer
}

const Buffer& Material::GetBuffer() const
{
    return mBuffer;
}

const TString& Material::GetName() const
{
    return mName;
}

uint32_t Material::GetPipelineHash() const
{
	return mPipelineStateHash;
}
