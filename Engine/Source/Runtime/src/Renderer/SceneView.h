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

	SceneView(const RenderSurface* surface, const World* world, EntityHandle camera);

	~SceneView();

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

	int32_t GetPriority() const
	{
		return mPriority;
	}

	const World* GetWorld() const
	{
		return mWorld;
	}

	const RenderPipeline* GetRenderPipeline() const
	{
		return mRenderPipeline;
	}

	const RenderTexture* GetRenderTexture() const
	{
		return mRenderTexture;
	}

	EntityHandle GetCameraEntity() const
	{
		return mCamera;
	}

	void ReleaseHistory(GraphicsDevice* device, GPUAllocator* allocator);

	bool ShouldRender() const;
	
	float4x4 prevViewMatrix = {};
	float4x4 prevViewProjectionMatrix = {};
	bool historyValid = false;

private:

	const World* mWorld = nullptr;
	const RenderSurface* mSurface = nullptr;

	RenderPipeline* mRenderPipeline = nullptr;
	RenderTexture* mRenderTexture = nullptr;

	int32_t mPriority = 0;
	EntityHandle mCamera = InvalidEntity;

	HashMap<uint32_t, HistoryEntry> mHistories;

	RendererList mRenderers;

};

} // namespace Gleam
