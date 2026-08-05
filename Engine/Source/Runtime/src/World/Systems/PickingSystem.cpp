#include "gpch.h"
#include "PickingSystem.h"

#include "Core/Engine.h"
#include "Core/Globals.h"
#include "Core/ConfigSystem.h"

#include "Renderer/Swapchain.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/RendererConfig.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/RenderPipeline.h"
#include "Renderer/Renderers/PickingRenderer.h"

using namespace Gleam;

void PickingSystem::Initialize(World* world)
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();

	CreateReadbackBuffer(GetFramesInFlight());

	mRenderer = new PickingRenderer(this);
	mRenderer->OnCreate(renderSystem->GetRenderContext());
	renderSystem->GetRenderPipeline(RenderPath::Default)->AddSharedRenderer(mRenderer);
	renderSystem->GetRenderPipeline(RenderPath::PathTracing)->AddSharedRenderer(mRenderer);

	// Reconfiguring the swapchain recreates the per frame command buffers, so anything already in
	// flight loses its completion guarantee. The ring itself is rebuilt in ResolveReadback, where the
	// swapchain is guaranteed to have been reconfigured already.
	auto configSystem = Globals::Engine->GetSubsystem<ConfigSystem>();
	mConfigHandle = configSystem->Subscribe<RendererConfig>([this](const RendererConfig& config)
	{
		for (auto& pending : mPendingPicks)
		{
			pending.encoded = false;
		}
	});
}

void PickingSystem::Shutdown(World* world)
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();

	auto configSystem = Globals::Engine->GetSubsystem<ConfigSystem>();
	configSystem->Unsubscribe<RendererConfig>(mConfigHandle);

	auto context = renderSystem->GetRenderContext();
	renderSystem->GetRenderPipeline(RenderPath::Default)->RemoveSharedRenderer(mRenderer);
	renderSystem->GetRenderPipeline(RenderPath::PathTracing)->RemoveSharedRenderer(mRenderer);
	mRenderer->OnDestroy(context);
	delete mRenderer;
	mRenderer = nullptr;

	if (mReadbackBuffer.IsValid())
	{
		renderSystem->GetDevice()->Dispose(renderSystem->GetAllocator(), mReadbackBuffer, BarrierStage::None);
	}
}

PickingRequestID PickingSystem::RequestPick(const PickingRequest& request)
{
	QueuedRequest queued;
	queued.requestID = mNextRequestID++;
	queued.request = request;
	mRequestQueue.push(queued);
	return queued.requestID;
}

PickingCallbackHandle PickingSystem::AddCallback(PickingCallback&& callback)
{
	Subscriber subscriber;
	subscriber.handle = mNextCallbackHandle++;
	subscriber.callback = eastl::move(callback);
	mCallbacks.push_back(eastl::move(subscriber));
	return mCallbacks.back().handle;
}

void PickingSystem::RemoveCallback(PickingCallbackHandle handle)
{
	for (auto it = mCallbacks.begin(); it != mCallbacks.end(); ++it)
	{
		if (it->handle == handle)
		{
			mCallbacks.erase(it);
			break;
		}
	}
}

void PickingSystem::CreateReadbackBuffer(uint32_t framesInFlight)
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	auto device = renderSystem->GetDevice();

	if (mReadbackBuffer.IsValid())
	{
		device->Dispose(renderSystem->GetAllocator(), mReadbackBuffer, BarrierStage::None);
	}

	BufferDescriptor bufferDesc;
	bufferDesc.name = "PickingReadbackBuffer";
	bufferDesc.memoryType = MemoryType::Readback;
	bufferDesc.size = MaskBytes * framesInFlight;
	mReadbackBuffer = device->CreateBuffer(renderSystem->GetAllocator(), bufferDesc);

	mPendingPicks.clear();
	mPendingPicks.resize(framesInFlight);
}

void PickingSystem::ResolveReadback(uint32_t frameIndex)
{
	const uint32_t framesInFlight = GetFramesInFlight();
	if (mPendingPicks.size() != framesInFlight)
	{
		CreateReadbackBuffer(framesInFlight);
	}

	Resolve(frameIndex);

	if (mCompletedResults.empty())
	{
		return;
	}

	// Callbacks may request another pick or add and remove callbacks, so hand them copies to walk
	auto results = eastl::move(mCompletedResults);
	mCompletedResults.clear();

	auto callbacks = mCallbacks;
	for (const auto& result : results)
	{
		for (const auto& subscriber : callbacks)
		{
			std::invoke(subscriber.callback, result);
		}
	}
}

void PickingSystem::Resolve(uint32_t frameIndex)
{
	auto& pending = mPendingPicks[frameIndex];
	if (pending.encoded == false)
	{
		return;
	}
	pending.encoded = false;

	PickingResult result;
	result.requestID = pending.requestID;
	result.request = pending.request;

	auto instanceCount = static_cast<uint32_t>(pending.instanceToEntity.size());
	uint32_t wordCount = Math::DivideRoundingUp(instanceCount, 32u);
	auto mask = static_cast<const uint32_t*>(mReadbackBuffer.GetContents()) + frameIndex * PICKING_MASK_UINTS;

	for (uint32_t word = 0; word < wordCount; ++word)
	{
		uint32_t bits = mask[word];
		for (uint32_t bit = 0; bits != 0; ++bit, bits >>= 1)
		{
			const uint32_t instanceID = word * 32u + bit;
			// A swapchain reconfigure can leave a stale readback behind, so never trust the bit index
			if ((bits & 1u) && instanceID < instanceCount)
			{
				result.entities.push_back(pending.instanceToEntity[instanceID]);
			}
		}
	}

	// One entity contributes an instance per submesh, so the same handle can come back several times
	eastl::sort(result.entities.begin(), result.entities.end());
	result.entities.erase(eastl::unique(result.entities.begin(), result.entities.end()), result.entities.end());

	mCompletedResults.emplace_back(eastl::move(result));
}

uint32_t PickingSystem::GetFramesInFlight() const
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	return static_cast<const Swapchain*>(renderSystem->GetSurface())->GetFramesInFlight();
}

PickingSystem::PendingPick* PickingSystem::AcquirePendingPick(uint32_t slot)
{
	if (mRequestQueue.empty())
	{
		return nullptr;
	}

	auto& pending = mPendingPicks[slot];
	const auto& queued = mRequestQueue.front();
	pending.requestID = queued.requestID;
	pending.request = queued.request;
	pending.encoded = false;
	mRequestQueue.pop();
	return &pending;
}

void PickingSystem::CompleteWithoutReadback(const PendingPick& pending)
{
	PickingResult result;
	result.requestID = pending.requestID;
	result.request = pending.request;
	mCompletedResults.emplace_back(eastl::move(result));
}
