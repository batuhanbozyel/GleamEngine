#pragma once
#include "EAssetManager.h"
#include "IO/Path.h"
#include "Reflection/Reflection.h"

namespace GEditor {

class AssetImportRequest
{
public:

	AssetImportRequest(const Gleam::Path& source)
		: mSource(source)
	{

	}

	virtual ~AssetImportRequest() = default;

	virtual void* Settings() = 0;

	virtual const void* Settings() const = 0;

	virtual const Gleam::Reflection::ClassDescription& SettingsClass() const = 0;

	// Callers guarantee both requests share the same SettingsClass
	virtual void CopySettings(const AssetImportRequest& other) = 0;

	virtual bool Import(EAssetManager* assetManager, const Gleam::Path& outputDirectory) = 0;

	const Gleam::Path& Source() const
	{
		return mSource;
	}

private:

	Gleam::Path mSource;

};

template<typename Package>
class TAssetImportRequest final : public AssetImportRequest
{
public:

	using ImportSettings = typename Package::ImportSettings;

	using AssetImportRequest::AssetImportRequest;

	virtual void* Settings() override
	{
		return &mSettings;
	}

	virtual const void* Settings() const override
	{
		return &mSettings;
	}

	virtual const Gleam::Reflection::ClassDescription& SettingsClass() const override
	{
		return Gleam::Reflection::GetClass<ImportSettings>();
	}

	virtual void CopySettings(const AssetImportRequest& other) override
	{
		mSettings = *static_cast<const ImportSettings*>(other.Settings());
	}

	virtual bool Import(EAssetManager* assetManager, const Gleam::Path& outputDirectory) override
	{
		auto registry = AssetRegistry(Source().Parent());
		auto package = Package(assetManager, &registry);
		if (package.Import(Source(), mSettings))
		{
			assetManager->Import(outputDirectory, package);
			return true;
		}
		return false;
	}

private:

	ImportSettings mSettings;

};

} // namespace GEditor
