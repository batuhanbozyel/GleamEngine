#pragma once
#include "AssetPackage.h"
#include "AssetRegistry.h"

#include "IO/Path.h"
#include "Core/Subsystem.h"
#include "Input/InputActionDescriptor.h"
#include "Container/Pointer.h"

namespace GEditor {

class EAssetManager final : public Gleam::GameInstanceSubsystem
{
public:

	EAssetManager(const Gleam::Path& directory);

	virtual void Initialize(Gleam::Application* app) override;

	virtual void Shutdown(Gleam::Application* app) override;

	void Import(const Gleam::Path& directory, const AssetPackage& package);

	const AssetItem& GetAsset(const Gleam::Guid& guid) const;

	const AssetItem* FindAsset(const Gleam::Guid& guid) const;

	const AssetItem& CreateInputActionAsset(const Gleam::Path& directory);

	const AssetItem* SaveInputActionAsset(const Gleam::Path& directory, const AssetItem& item, const Gleam::InputActionAssetDescriptor& descriptor);

	Gleam::Optional<Gleam::InputActionAssetDescriptor> LoadInputActionAsset(const Gleam::Path& file) const;

	bool IsValidInputActionAssetName(const Gleam::Path& directory, const AssetItem& item, Gleam::TStringView name) const;

	const AssetItem* RenameInputActionAsset(const Gleam::Path& file, const AssetItem& item, Gleam::TStringView name);

	bool RemoveInputActionAsset(const Gleam::Path& file, const AssetItem& item);

	template<typename T>
	const AssetItem& GetAsset(const Gleam::Path& path) const
	{
		return mRegistry.GetAsset<T>(path);
	}

private:

	void CompileMaterials(const Gleam::Path& sourceDirectory, const Gleam::Path& outputDirectory);

	void CompileMaterial(const Gleam::Path& source, const Gleam::Path& outputDirectory);

	AssetRegistry mRegistry;

	Gleam::Path mAssetDirectory;

};

} // namespace GEditor
