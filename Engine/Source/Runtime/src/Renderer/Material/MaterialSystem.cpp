#include "gpch.h"
#include "MaterialSystem.h"
#include "Core/Globals.h"
#include "Core/Application.h"
#include "Assets/AssetManager.h"

using namespace Gleam;

void MaterialSystem::Initialize(Application* app)
{
	Filesystem::ForEach(Globals::ProjectContentDirectory, [&, this](const auto& entry)
	{
		if (entry.extension() == ".asset")
		{
			auto file = Filesystem::Open(entry, FileType::Text);
			auto accessor = Filesystem::ReadAccessor(entry);
			auto serializer = JSONSerializer(file.GetStream());
			auto header = serializer.ParseHeader();

			if (header.guid == Reflection::GetClass<MaterialDescriptor>().Guid())
			{
				auto guid = Guid(entry.stem().string());
				AssetReference ref = { .guid = guid };
				auto descriptor = app->GetSubsystem<AssetManager>()->Get<MaterialDescriptor>(ref);
				mMaterials.emplace_hint(mMaterials.end(), ref, CreateScope<Material>(descriptor));
			}
		}
	}, true);
}

void MaterialSystem::Shutdown()
{
	for (auto& [ref, material] : mMaterials)
	{
		material->Dispose();
	}
	mMaterials.clear();
}

Material* MaterialSystem::GetMaterial(const AssetReference& ref)
{
	auto it = mMaterials.find(ref);
	if (it == mMaterials.end())
	{
		auto descriptor = Globals::GameInstance->GetSubsystem<AssetManager>()->Get<MaterialDescriptor>(ref);
		it = mMaterials.emplace_hint(mMaterials.end(), ref, CreateScope<Material>(descriptor));
	}
	return it->second.get();
}
