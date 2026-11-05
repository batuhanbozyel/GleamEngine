#include "gpch.h"
#include "SceneView.h"
#include "RenderSurface.h"
#include "RenderTexture.h"
#include "RenderSystem.h"

#include "World/World.h"
#include "World/Components/Camera.h"

#include "Core/Engine.h"
#include "Core/Globals.h"
#include "Assets/AssetManager.h"

namespace Gleam {

SceneView::SceneView(const RenderSurface* surface, const World* world, EntityHandle camera)
	: mWorld(world)
	, mSurface(surface)
	, mCamera(camera)
{
	auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	auto assetManager = Globals::GameInstance->GetSubsystem<AssetManager>();

	const auto& cameraComponent = mWorld->GetEntityManager().GetComponent<Camera>(mCamera);
	mPriority = cameraComponent.priority;

	const auto& pipelineRef = cameraComponent.renderPipeline.guid != Guid::InvalidGuid() ? cameraComponent.renderPipeline : renderSystem->GetDefaultRenderPipeline();
	mRenderPipeline = assetManager->Load<RenderPipeline>(pipelineRef);

	if (cameraComponent.targetTexture.guid != Guid::InvalidGuid())
	{
		mRenderTexture = assetManager->Load<RenderTexture>(cameraComponent.targetTexture);
		mSurface = mRenderTexture;
	}
}

SceneView::~SceneView()
{
	auto assetManager = Globals::GameInstance->GetSubsystem<AssetManager>();
	assetManager->Release(mRenderPipeline->GetReference());

	if (mRenderTexture)
	{
		assetManager->Release(mRenderTexture->GetReference());
	}
}

Rect SceneView::GetRect() const
{
	const auto output = mSurface->GetSize();
	const auto& rect = mWorld->GetEntityManager().GetComponent<Camera>(mCamera).viewportRect;
	const float minX = Math::Round(Math::Clamp(rect.offset.x, 0.0f, 1.0f) * output.width);
	const float minY = Math::Round(Math::Clamp(rect.offset.y, 0.0f, 1.0f) * output.height);
	const float maxX = Math::Round(Math::Clamp(rect.offset.x + rect.size.width, 0.0f, 1.0f) * output.width);
	const float maxY = Math::Round(Math::Clamp(rect.offset.y + rect.size.height, 0.0f, 1.0f) * output.height);
	return Rect(Size(maxX - minX, maxY - minY), Float2(minX, minY));
}

Size SceneView::GetResolution() const
{
	return GetRect().size;
}

void SceneView::ReleaseHistory(GraphicsDevice* device, GPUAllocator* allocator)
{
	for (auto& [_, entry] : mHistories)
	{
		entry.release(entry.data, device, allocator);
	}
	mHistories.clear();
	historyValid = false;
}

bool SceneView::ShouldRender() const
{
	const auto resolution = GetResolution();
	return mWorld->GetEntityManager().GetComponent<Entity>(mCamera).IsActive() and resolution.width >= 1.0f and resolution.height >= 1.0f;
}

} // namespace Gleam
