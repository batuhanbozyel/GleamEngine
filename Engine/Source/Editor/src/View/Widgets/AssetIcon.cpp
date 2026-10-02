#include "AssetIcon.h"

#include "Renderer/MeshDescriptor.h"
#include "Renderer/TextureDescriptor.h"
#include "Renderer/Material/MaterialDescriptor.h"

#include "World/Prefab.h"
#include "World/World.h"

#include <Runtime.Reflection.generated.h>

using namespace GEditor;

AssetIcon GEditor::GetAssetIcon(const Gleam::Guid& type)
{
	static const auto meshType = Gleam::Reflection::GetClass<Gleam::MeshDescriptor>().Guid();
	static const auto textureType = Gleam::Reflection::GetClass<Gleam::Texture2DDescriptor>().Guid();
	static const auto materialType = Gleam::Reflection::GetClass<Gleam::MaterialDescriptor>().Guid();
	static const auto materialInstanceType = Gleam::Reflection::GetClass<Gleam::MaterialInstanceDescriptor>().Guid();
	static const auto prefabType = Gleam::Reflection::GetClass<Gleam::Prefab>().Guid();
	static const auto worldType = Gleam::Reflection::GetClass<Gleam::World>().Guid();

	if (type == meshType)
	{
		return AssetIcon{ .text = "MESH", .color = Gleam::Theme::Info, .category = AssetCategory::Mesh };
	}
	else if (type == textureType)
	{
		return AssetIcon{ .text = "TEXTURE", .color = Gleam::Theme::Pink, .category = AssetCategory::Texture };
	}
	else if (type == materialType)
	{
		return AssetIcon{ .text = "MATERIAL", .color = Gleam::Theme::Success, .category = AssetCategory::Material };
	}
	else if (type == materialInstanceType)
	{
		return AssetIcon{ .text = "MATERIAL\nINSTANCE", .color = Gleam::Theme::Teal, .category = AssetCategory::MaterialInstance };
	}
	else if (type == prefabType)
	{
		return AssetIcon{ .text = "PREFAB", .color = Gleam::Theme::Warning, .category = AssetCategory::Prefab };
	}
	else if (type == worldType)
	{
		return AssetIcon{ .text = "WORLD", .color = Gleam::Theme::Purple, .category = AssetCategory::World };
	}
	else
	{
		return AssetIcon();
	}
}
