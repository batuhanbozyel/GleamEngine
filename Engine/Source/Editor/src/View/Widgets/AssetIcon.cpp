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
		return AssetIcon{ .text = "Mesh", .color = Gleam::Color(0.2f, 0.5f, 0.95f, 1.0f) }; // Blue
	}
	if (type == textureType)
	{
		return AssetIcon{ .text = "Texture", .color = Gleam::Color(0.95f, 0.3f, 0.7f, 1.0f) }; // Pink/Magenta
	}
	if (type == materialType)
	{
		return AssetIcon{ .text = "Material", .color = Gleam::Color(0.15f, 0.65f, 0.1f, 1.0f) }; // Green
	}
	if (type == materialInstanceType)
	{
		return AssetIcon{ .text = "Material\nInstance", .color = Gleam::Color(0.5f, 0.95f, 0.4f, 1.0f) }; // Light Green
	}
	if (type == prefabType)
	{
		return AssetIcon{ .text = "Prefab", .color = Gleam::Color(0.9f, 0.55f, 0.2f, 1.0f) }; // Orange
	}
	if (type == worldType)
	{
		return AssetIcon{ .text = "World", .color = Gleam::Color(0.7f, 0.3f, 0.85f, 1.0f) }; // Purple
	}
	return AssetIcon();
}
