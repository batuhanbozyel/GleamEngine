#include "gpch.h"
#include "AssetManager.h"
#include "Core/Engine.h"
#include "Core/Globals.h"
#include "IO/FileWatcher.h"

#include "Renderer/Mesh.h"
#include "Renderer/Texture.h"
#include "Renderer/Material/Material.h"
#include "Renderer/Material/MaterialInstance.h"

using namespace Gleam;

void AssetManager::Initialize(Application* app)
{
	RegisterMetaAsset<Mesh, MeshDescriptor>();
	RegisterMetaAsset<Texture, TextureDescriptor>();
	RegisterMetaAsset<Material, MaterialDescriptor>();
	RegisterMetaAsset<MaterialInstance, MaterialInstanceDescriptor>();
	
	Filesystem::ForEach(Globals::ProjectContentDirectory, [this](const auto& entry)
	{
		if (entry.extension() == ".asset")
		{
			EmplaceAssetPath(entry);
		}
	}, true);

    auto fileWatcher = Globals::Engine->GetSubsystem<FileWatcher>();
    fileWatcher->AddWatch(Globals::ProjectContentDirectory, [this](const Filesystem::Path& path, FileWatchEvent event)
    {
        if (path.extension() != Asset::Extension())
        {
            return;
        }
        
		auto relPath = Filesystem::Relative(path, Globals::ProjectContentDirectory);
		std::lock_guard<std::mutex> lock(mMutex);
        switch (event)
        {
            case FileWatchEvent::Added:
            {
				EmplaceAssetPath(relPath);
                break;
            }
            case FileWatchEvent::Removed:
            {
                auto it = std::find_if(mAssetPaths.begin(), mAssetPaths.end(), [&](auto pair)
                {
					return pair.second == relPath;
                });
                
                if (it != mAssetPaths.end())
                {
					mAssetPaths.erase(it);
                }
                break;
            }
			case FileWatchEvent::Modified:
			{
				auto it = std::find_if(mAssetPaths.begin(), mAssetPaths.end(), [&](auto pair)
				{
					return pair.second == relPath;
				});

				if (it == mAssetPaths.end())
				{
					EmplaceAssetPath(relPath);
				}
				break;
			}
            default: break;
        }
    });
}

void AssetManager::Shutdown()
{
	mAssetPaths.clear();
}

void AssetManager::EmplaceAssetPath(const Filesystem::Path& path)
{
	Guid guid = path.stem().string();
	auto relPath = path.is_relative() ? path : Filesystem::Relative(path, Globals::ProjectContentDirectory);
	
	if (guid != Guid::InvalidGuid())
	{
		AssetReference assetRef = { .guid = guid };
		mAssetPaths[assetRef] = relPath;
	}
}

const Filesystem::Path& AssetManager::GetAssetPath(const AssetReference& ref) const
{
	auto it = mAssetPaths.find(ref);
	if (it != mAssetPaths.end())
	{
		return it->second;
	}

	GLEAM_CORE_ERROR("Asset could not located for GUID: {0}", ref.guid.ToString());
	GLEAM_ASSERT(false);
	static Filesystem::Path invalidPath;
	return invalidPath;
}
