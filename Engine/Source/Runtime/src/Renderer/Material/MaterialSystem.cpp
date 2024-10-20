#include "gpch.h"
#include "MaterialSystem.h"
#include "Core/Globals.h"
#include "Core/Application.h"
#include "Assets/AssetManager.h"

using namespace Gleam;

void MaterialSystem::Initialize(Application* app)
{
	
}

void MaterialSystem::Shutdown()
{
	for (auto& [ref, material] : mMaterials)
	{
		material.Release();
	}
	mMaterials.clear();
}

Material& MaterialSystem::GetMaterial(const AssetReference& ref)
{
	auto it = mMaterials.find(ref);
	if (it == mMaterials.end())
	{
		auto descriptor = Globals::GameInstance->GetSubsystem<AssetManager>()->LoadDescriptor<MaterialDescriptor>(ref);
		it = mMaterials.emplace_hint(mMaterials.end(),
									 std::piecewise_construct,
									 std::forward_as_tuple(ref),
									 std::forward_as_tuple(descriptor));
	}
	return it->second;
}
