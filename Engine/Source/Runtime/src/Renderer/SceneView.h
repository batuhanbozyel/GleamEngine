#pragma once
#include "Math/Rect.h"
#include "Container/Hash.h"
#include "World/Entity.h"
#include "Assets/AssetReference.h"
#include "Renderer/RendererList.h"
#include "Renderer/Shaders/ShaderTypes.h"

#include <entt/core/type_info.hpp>
#include <EASTL/any.h>

namespace Gleam {

class World;
class GPUAllocator;
class GraphicsDevice;
class RenderPipeline;
class RenderSurface;
class RenderTexture;

template<typename T>
concept SceneViewHistoryType = requires(T history, GraphicsDevice* device, GPUAllocator* allocator)
{
	history.Release(device, allocator);
};

class SceneView final
{
	using ReleaseFunc = void(*)(eastl::any& data, GraphicsDevice* device, GPUAllocator* allocator);

	struct HistoryEntry
	{
		eastl::any data;
		ReleaseFunc release = nullptr;
	};

public:

	SceneView(const World* world, EntityHandle camera, const RenderSurface* surface)
		: world(world)
		, camera(camera)
		, mSurface(surface)
	{

	}

	// The view's region of its output in pixels, from the camera's viewport rect
	Rect GetRect() const;

	Size GetResolution() const;

	template<SceneViewHistoryType T>
	T& GetHistory()
	{
		const uint32_t key = entt::type_hash<T>().value();
		auto it = mHistories.find(key);
		if (it == mHistories.end())
		{
			HistoryEntry entry;
			entry.data.template emplace<T>();
			entry.release = [](eastl::any& data, GraphicsDevice* device, GPUAllocator* allocator)
			{
				eastl::any_cast<T&>(data).Release(device, allocator);
			};
			it = mHistories.emplace(key, eastl::move(entry)).first;
		}
		return eastl::any_cast<T&>(it->second.data);
	}

	void AddRenderer(IRenderer* renderer, RenderStage stage)
	{
		mRenderers.Insert(renderer, stage);
	}

	void RemoveRenderer(IRenderer* renderer)
	{
		mRenderers.Remove(renderer);
	}

	const RendererList& GetRenderers() const
	{
		return mRenderers;
	}

	void ReleaseHistory(GraphicsDevice* device, GPUAllocator* allocator)
	{
		for (auto& [_, entry] : mHistories)
		{
			entry.release(entry.data, device, allocator);
		}
		mHistories.clear();
		historyValid = false;
	}

	const World* world = nullptr;
	EntityHandle camera = InvalidEntity;
	AssetReference pipelineReference;
	RenderPipeline* pipeline = nullptr;
	AssetReference targetReference;
	RenderTexture* target = nullptr;
	float4x4 prevViewMatrix = {};
	float4x4 prevViewProjectionMatrix = {};
	bool historyValid = false;

private:

	// Output of views without a target texture
	const RenderSurface* mSurface = nullptr;

	HashMap<uint32_t, HistoryEntry> mHistories;

	RendererList mRenderers;

};

} // namespace Gleam
