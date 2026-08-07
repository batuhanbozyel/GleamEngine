#pragma once
#include "World/WorldSubsystem.h"
#include "World/Entity.h"
#include "Core/ConfigSystem.h"
#include "Container/Queue.h"
#include "Renderer/Buffer.h"
#include "Renderer/Shaders/ShaderTypes.h"

#include <functional>

namespace Gleam {

class PickingRenderer;

struct PickingRequest
{
	uint32_t x = 0;
	uint32_t y = 0;
	uint32_t width = 1;
	uint32_t height = 1;
};

using PickingRequestID = uint32_t;
static constexpr PickingRequestID InvalidPickingRequest = 0;

struct PickingResult
{
	PickingRequestID requestID = InvalidPickingRequest;
	PickingRequest request = {};
	TArray<EntityHandle> entities;
};

using PickingCallback = std::function<void(const PickingResult&)>;
using PickingCallbackHandle = uint32_t;
static constexpr PickingCallbackHandle InvalidPickingCallback = 0;

class PickingSystem final : public WorldSubsystem
{
	friend class PickingRenderer;
	friend class RenderSystem;
public:

	virtual void Initialize(World* world) override;

	virtual void Shutdown(World* world) override;

	PickingRequestID RequestPick(const PickingRequest& request);

	PickingCallbackHandle AddCallback(PickingCallback&& callback);

	void RemoveCallback(PickingCallbackHandle handle);

private:

	struct PendingPick
	{
		PickingRequestID requestID = InvalidPickingRequest;
		PickingRequest request = {};
		TArray<EntityHandle> instanceToEntity;
		bool encoded = false;
	};

	struct QueuedRequest
	{
		PickingRequestID requestID = InvalidPickingRequest;
		PickingRequest request = {};
	};

	struct Subscriber
	{
		PickingCallbackHandle handle = InvalidPickingCallback;
		PickingCallback callback;
	};

	static constexpr size_t MaskBytes = INSTANCE_MASK_UINTS * sizeof(uint32_t);

	void ResolveReadback(uint32_t frameIndex);

	void CreateReadbackBuffer(uint32_t framesInFlight);

	void Resolve(uint32_t frameIndex);

	uint32_t GetFramesInFlight() const;

	PendingPick* AcquirePendingPick(uint32_t slot);

	void CompleteWithoutReadback(const PendingPick& pending);

	const Buffer& GetReadbackBuffer() const { return mReadbackBuffer; }

	Buffer mReadbackBuffer = {};
	PickingRenderer* mRenderer = nullptr;
	ConfigCallbackHandle mConfigHandle = 0;

	TArray<PendingPick> mPendingPicks;
	Queue<QueuedRequest> mRequestQueue;
	TArray<Subscriber> mCallbacks;
	TArray<PickingResult> mCompletedResults;

	PickingRequestID mNextRequestID = 1;
	PickingCallbackHandle mNextCallbackHandle = 1;

};

} // namespace Gleam
